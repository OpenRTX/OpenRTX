#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "AudioSystem.h"
#include "AudioTrack.h"
#include "AudioRecord.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <interfaces/audio.h>

#define SAMPLE_RATE (16000)    // Sample rate in Hz

// Live audio streaming configuration
#define AUDIO_CHUNK_MS          (20)      // 20ms chunks for low latency
#define AUDIO_CHUNK_SAMPLES     (SAMPLE_RATE * AUDIO_CHUNK_MS / 1000)
#define AUDIO_CHUNK_SIZE        (AUDIO_CHUNK_SAMPLES * sizeof(int16_t))
#define AUDIO_BUFFER_CHUNKS     (8)       // Circular buffer with multiple chunks
#define AUDIO_THREAD_STACK_SIZE (8*1024)
#define AUDIO_THREAD_PRIORITY   (5)

#define PATH(x,y) ((x << 4) | y)

// Circular buffer for audio data exchange between threads
typedef struct {
    uint8_t buffer[AUDIO_CHUNK_SIZE];
    bool filled;
} AudioChunk;

typedef struct {
    AudioChunk chunks[AUDIO_BUFFER_CHUNKS];
    int write_idx;
    int read_idx;
    struct k_mutex mutex;
    struct k_sem data_ready;
    struct k_sem space_available;
} AudioRingBuffer;

#define C62_AUDIO_CHANNEL_MIC        CHANNEL_IN_LEFT
#define C62_AUDIO_CHANNEL_RX         CHANNEL_IN_RIGHT
#define C62_AUDIO_CHANNEL_SPK        CHANNEL_OUT_FRONT_LEFT
#define C62_AUDIO_CHANNEL_TX         CHANNEL_OUT_FRONT_RIGHT

#define AUDIO_INPUT_CHANNEL_COUNT 2

static const struct gpio_dt_spec speaker_enable = GPIO_DT_SPEC_GET(DT_PATH(gpio_controls, speaker_enable), gpios);

// Live audio streaming state
typedef struct {
    enum AudioSource source;
    enum AudioSink sink;
    bool active;
} AudioPath;

static AudioPath s_active_paths[3];  // Support up to 3 simultaneous paths
static int s_active_path_count = 0;
static struct k_mutex s_audio_mutex;

// Audio devices for each source/sink
static AudioRecord s_record;
static AudioTrack s_track_spk;
static AudioTrack s_track_rtx_output;

static bool s_audio_initialized = false;
static bool s_audio_streaming = false;

// Track which devices are currently active
static bool s_mic_active = false;
static bool s_rtx_input_active = false;
static bool s_spk_active = false;
static bool s_rtx_output_active = false;

const struct audioDevice outputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {NULL, 0, 0, SINK_SPK},
};

const struct audioDevice inputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {NULL, 0, 0, SINK_SPK},
};

// Ring buffer for audio data exchange (in PSRAM)
__attribute__((section(".psram_section"))) static AudioRingBuffer s_ring_buffer;

// Separate threads for input and output
static struct k_thread s_audio_input_thread;
static struct k_thread s_audio_output_thread;
K_THREAD_STACK_DEFINE(s_audio_input_stack, AUDIO_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(s_audio_output_stack, AUDIO_THREAD_STACK_SIZE);


#define FREQUENCY 440        // Frequency of the sine wave in Hz (A4)

/* Fill buffer with sine wave data */
static void fillSineWave(uint8_t *buffer, size_t size) {
    // Calculate the sine wave
    for (size_t i = 0; i < size / sizeof(int16_t); i++) {
        // Calculate sample value
        double sample = sin(2.0 * M_PI * FREQUENCY * (i / (double)SAMPLE_RATE));

        // Scale to 16-bit PCM range (-32768 to 32767)
        int16_t pcmValue = (int16_t)(sample * 32767);
        
        // Store the PCM value in the buffer
        buffer[2 * i] = (uint8_t)(pcmValue & 0xFF);          // LSB
        buffer[2 * i + 1] = (uint8_t)((pcmValue >> 8) & 0xFF); // MSB
    }
}

// Ring buffer helper functions
static void ring_buffer_init(AudioRingBuffer *rb)
{
	k_mutex_init(&rb->mutex);
	k_sem_init(&rb->data_ready, 0, AUDIO_BUFFER_CHUNKS);
	k_sem_init(&rb->space_available, AUDIO_BUFFER_CHUNKS, AUDIO_BUFFER_CHUNKS);
	
	rb->write_idx = 0;
	rb->read_idx = 0;
	
	for (int i = 0; i < AUDIO_BUFFER_CHUNKS; i++) {
		rb->chunks[i].filled = false;
	}
}

static bool ring_buffer_write(AudioRingBuffer *rb, const uint8_t *data, size_t size)
{
	if (size > AUDIO_CHUNK_SIZE) {
		return false;
	}

	// Wait for space to be available (with timeout to prevent deadlock)
	if (k_sem_take(&rb->space_available, K_MSEC(100)) != 0) {
		printk("Ring buffer full, dropping audio chunk\n");
		return false;
	}

	k_mutex_lock(&rb->mutex, K_FOREVER);
	
	// Copy data to the write buffer
	memcpy(rb->chunks[rb->write_idx].buffer, data, size);
	rb->chunks[rb->write_idx].filled = true;
	
	// Advance write index
	rb->write_idx = (rb->write_idx + 1) % AUDIO_BUFFER_CHUNKS;
	
	k_mutex_unlock(&rb->mutex);

	// Signal that data is ready
	k_sem_give(&rb->data_ready);
	
	return true;
}

static bool ring_buffer_read(AudioRingBuffer *rb, uint8_t *data, size_t *size)
{
	// Wait for data to be available
	if (k_sem_take(&rb->data_ready, K_MSEC(100)) != 0) {
		return false;
	}

	k_mutex_lock(&rb->mutex, K_FOREVER);
	
	// Copy data from the read buffer
	memcpy(data, rb->chunks[rb->read_idx].buffer, AUDIO_CHUNK_SIZE);
	*size = AUDIO_CHUNK_SIZE;
	rb->chunks[rb->read_idx].filled = false;
	
	// Advance read index
	rb->read_idx = (rb->read_idx + 1) % AUDIO_BUFFER_CHUNKS;
	
	k_mutex_unlock(&rb->mutex);

	// Signal that space is available
	k_sem_give(&rb->space_available);
	
	return true;
}

// Audio input thread - records audio and pushes to ring buffer
static void audio_input_thread(void *arg1, void *arg2, void *arg3)
{
	(void)arg1;
	(void)arg2;
	(void)arg3;

	printk("Audio input thread started\n");

	static uint8_t chunk_buffer[AUDIO_CHUNK_SIZE * AUDIO_INPUT_CHANNEL_COUNT /* channel input MIC + RTX */];

	while (s_audio_streaming) {
		k_mutex_lock(&s_audio_mutex, K_FOREVER);
		
		// Check if we have any active recording sources
		bool has_mic_source = false;
		bool has_rtx_source = false;

		for (int i = 0; i < s_active_path_count; i++) {
			if (!s_active_paths[i].active)
				continue;

			if (s_active_paths[i].source == SOURCE_MIC) {
				has_mic_source = true;
			} else if (s_active_paths[i].source == SOURCE_RTX) {
				has_rtx_source = true;
			}
		}

		k_mutex_unlock(&s_audio_mutex);

		ssize_t read_size = AudioRecord_read(&s_record, chunk_buffer, AUDIO_CHUNK_SIZE * AUDIO_INPUT_CHANNEL_COUNT);

		
		// Record audio from active source
		if (has_mic_source) {
			if (read_size > 0) {
				// Deinterleave MIC and RTX data
				for (size_t i = 0; i < AUDIO_CHUNK_SAMPLES; i++) {
					chunk_buffer[i * 2] = chunk_buffer[i * 4];	// MIC data (assuming MIC is on the first channel)
					chunk_buffer[i * 2 + 1] = chunk_buffer[i * 4 + 1];
				}
				read_size = AUDIO_CHUNK_SAMPLES * sizeof(int16_t); // Update read size
				// Push to ring buffer for output thread
				if (!ring_buffer_write(&s_ring_buffer, chunk_buffer, read_size)) {
					// Buffer overflow handled in ring_buffer_write
				}
			}
		} else if (has_rtx_source) {
			if (read_size > 0) {
				// Deinterleave MIC and RTX data
				for (size_t i = 0; i < AUDIO_CHUNK_SAMPLES; i++) {
					chunk_buffer[i * 2] = chunk_buffer[i * 4 + 2];	// MIC data (assuming MIC is on the first channel)
					chunk_buffer[i * 2 + 1] = chunk_buffer[i * 4 + 1 + 2];
				}
				read_size = AUDIO_CHUNK_SAMPLES * sizeof(int16_t); // Update read size
				// Push to ring buffer for output thread
				if (!ring_buffer_write(&s_ring_buffer, chunk_buffer, read_size)) {
					// Buffer overflow handled in ring_buffer_write
				}
			}
		} else {
			// Sleep if no active recording
			k_sleep(K_MSEC(10));
		}
	}

	printk("Audio input thread stopped\n");
}

/* Audio output thread - pulls from ring buffer and plays audio */
static void audio_output_thread(void *arg1, void *arg2, void *arg3)
{
	(void)arg1;
	(void)arg2;
	(void)arg3;

	printk("Audio output thread started\n");

	static uint8_t chunk_buffer[AUDIO_CHUNK_SIZE];
	size_t chunk_size;

	while (s_audio_streaming) {
		k_mutex_lock(&s_audio_mutex, K_FOREVER);
		
		// Check if we have any active playback sinks
		bool has_spk_sink = false;
		bool has_rtx_sink = false;

		for (int i = 0; i < s_active_path_count; i++) {
			if (!s_active_paths[i].active)
				continue;

			if (s_active_paths[i].sink == SINK_SPK) {
				has_spk_sink = true;
			} else if (s_active_paths[i].sink == SINK_RTX) {
				has_rtx_sink = true;
			}
		}

		k_mutex_unlock(&s_audio_mutex);

		// Play audio to SPK if needed
		if (has_spk_sink) {
			// Pull from ring buffer
			if (ring_buffer_read(&s_ring_buffer, chunk_buffer, &chunk_size)) {
				// Write to speaker track
				ssize_t written = AudioTrack_write(&s_track_spk, chunk_buffer, chunk_size);
				
				if (written < 0) {
					printk("AudioTrack_write failed: %d\n", (int)written);
				}
			}
		} else if (has_rtx_sink) {
			// Pull from ring buffer
			if (ring_buffer_read(&s_ring_buffer, chunk_buffer, &chunk_size)) {
				// Write to RTX output track
				ssize_t written = AudioTrack_write(&s_track_rtx_output, chunk_buffer, chunk_size);
				
				if (written < 0) {
					printk("AudioTrack_write failed: %d\n", (int)written);
				}
			}
		} else {
			// Sleep if no active playback
			k_sleep(K_MSEC(10));
		}
	}

	printk("Audio output thread stopped\n");
}



void audio_init()
{
	int ret;

	if (s_audio_initialized) {
		printk("Audio already initialized\n");
		return;
	}

	printk("Initializing audio subsystem\n");

	// Initialize mutex
	k_mutex_init(&s_audio_mutex);

	// Initialize ring buffer
	ring_buffer_init(&s_ring_buffer);

	// Initialize active paths array
	for (int i = 0; i < 3; i++) {
		s_active_paths[i].active = false;
	}
	s_active_path_count = 0;

	// Set audio system parameters
	String8 param;
	String8_ctor_char(&param, "ADC_PDM_GAIN_A_LEFT=6;"
				  "ADC_PDM_GAIN_D_LEFT=20;"
				  "ADC_PDM_GAIN_A_RIGHT=6;"
				  "ADC_PDM_GAIN_D_RIGHT=20");
	ret = AudioSystem_setParameters(0, &param);
	String8_dtor(&param);
	if (ret != 0) {
		printk("Failed to set audio parameters: %d\n", ret);
		return;
	}

	// Initialize AudioRecord for MIC and RTX input interleaved
	ret = AudioRecord_ctor(&s_record, 0, SAMPLE_RATE, PCM_16_BIT, 
	                       C62_AUDIO_CHANNEL_MIC | C62_AUDIO_CHANNEL_RX, 0, NULL);
	if (ret != 0) {
		printk("Failed to initialize AudioRecord RTX_INPUT: %d\n", ret);
		return;
	}

	// Initialize AudioTrack for speaker output
	ret = AudioTrack_ctor(&s_track_spk, SAMPLE_RATE, PCM_16_BIT, 
	                      C62_AUDIO_CHANNEL_SPK, 0, NULL);
	if (ret != 0) {
		printk("Failed to initialize AudioTrack SPK: %d\n", ret);
		AudioRecord_dtor(&s_record);
		return;
	}

	// Initialize AudioTrack for RTX output
	ret = AudioTrack_ctor(&s_track_rtx_output, SAMPLE_RATE, PCM_16_BIT, 
	                      C62_AUDIO_CHANNEL_TX, 0, NULL);
	if (ret != 0) {
		printk("Failed to initialize AudioTrack RTX_OUTPUT: %d\n", ret);
		AudioRecord_dtor(&s_record);
		AudioTrack_dtor(&s_track_spk);
		return;
	}

	s_audio_initialized = true;
	printk("Audio subsystem initialized successfully\n");
}

void audio_terminate()
{
	if (!s_audio_initialized) {
		return;
	}

	printk("Terminating audio subsystem\n");

	// Stop streaming if active
	if (s_audio_streaming) {
		s_audio_streaming = false;
		
		// Wait for both threads to finish
		k_thread_join(&s_audio_input_thread, K_SECONDS(1));
		k_thread_join(&s_audio_output_thread, K_SECONDS(1));
		
		// Stop all active devices
		if (s_mic_active || s_rtx_input_active) {
			AudioRecord_stop(&s_record);
			s_mic_active = false;
			s_rtx_input_active = false;
		}
		if (s_spk_active) {
			AudioTrack_stop(&s_track_spk);
			s_spk_active = false;
		}
		if (s_rtx_output_active) {
			AudioTrack_stop(&s_track_rtx_output);
			s_rtx_output_active = false;
		}
	}

	// Clean up audio objects
	AudioRecord_dtor(&s_record);
	AudioTrack_dtor(&s_track_spk);
	AudioTrack_dtor(&s_track_rtx_output);

	// Clear all paths
	k_mutex_lock(&s_audio_mutex, K_FOREVER);
	for (int i = 0; i < 3; i++) {
		s_active_paths[i].active = false;
	}
	s_active_path_count = 0;
	k_mutex_unlock(&s_audio_mutex);

	s_audio_initialized = false;
	printk("Audio subsystem terminated\n");
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
	if (!s_audio_initialized) {
		printk("Audio not initialized, initializing now\n");
		audio_init();
	}

    if(PATH(source, sink) == PATH(SOURCE_RTX, SINK_SPK))
    {
        radio_enableAfOutput();
        gpio_pin_set_dt(&speaker_enable, 1); 
    }

	k_mutex_lock(&s_audio_mutex, K_FOREVER);

	// Check if path already exists
	for (int i = 0; i < s_active_path_count; i++) {
		if (s_active_paths[i].source == source && 
		    s_active_paths[i].sink == sink &&
		    s_active_paths[i].active) {
			printk("Audio path already connected: %d->%d\n", source, sink);
			k_mutex_unlock(&s_audio_mutex);
			return;
		}
	}

	// Add new path
	if (s_active_path_count < 3) {
		s_active_paths[s_active_path_count].source = source;
		s_active_paths[s_active_path_count].sink = sink;
		s_active_paths[s_active_path_count].active = true;
		s_active_path_count++;

		printk("Audio path will connect: %d->%d (total: %d)\n", 
		       source, sink, s_active_path_count);

		AudioRecord_start(&s_record);
		// Start the appropriate source device based on the path
		if (source == SOURCE_MIC && !s_mic_active ) {
			s_mic_active = true;
			printk("Started input\n");
		} else if (source == SOURCE_RTX && !s_rtx_input_active) {
			s_rtx_input_active = true;
			printk("Started RTX input\n");
		}
		
		// Start the appropriate sink device based on the path
		if (sink == SINK_SPK && !s_spk_active) {
			AudioTrack_start(&s_track_spk);
			s_spk_active = true;
			printk("Started SPK output\n");
		} else if (sink == SINK_RTX && !s_rtx_output_active) {
			AudioTrack_start(&s_track_rtx_output);
			s_rtx_output_active = true;
			printk("Started RTX output\n");
		}

		// Start streaming thread if not already running
		if (!s_audio_streaming) {
			s_audio_streaming = true;
			
			// Create input thread (recording)
			k_thread_create(&s_audio_input_thread, s_audio_input_stack, AUDIO_THREAD_STACK_SIZE,
			                audio_input_thread, NULL, NULL, NULL,
			                AUDIO_THREAD_PRIORITY, 0, K_NO_WAIT);
			k_thread_name_set(&s_audio_input_thread, "audio_input");

			// Create output thread (playback)
			k_thread_create(&s_audio_output_thread, s_audio_output_stack, AUDIO_THREAD_STACK_SIZE,
			                audio_output_thread, NULL, NULL, NULL,
			                AUDIO_THREAD_PRIORITY, 0, K_NO_WAIT);
			k_thread_name_set(&s_audio_output_thread, "audio_output");
			
			printk("Audio streaming threads started\n");
		} 
	} else {
		printk("Maximum audio paths reached\n");
	}

	k_mutex_unlock(&s_audio_mutex);
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
	if (!s_audio_initialized) {
		return;
	}

	if(PATH(source, sink) == PATH(SOURCE_RTX, SINK_SPK))
    {
        gpio_pin_set_dt(&speaker_enable, 0); 
        radio_disableAfOutput();
    }

	k_mutex_lock(&s_audio_mutex, K_FOREVER);

	// Find and remove the path
	bool found = false;
	for (int i = 0; i < s_active_path_count; i++) {
		if (s_active_paths[i].source == source && 
		    s_active_paths[i].sink == sink &&
		    s_active_paths[i].active) {
			s_active_paths[i].active = false;
			found = true;
			
			// Compact array by moving last element to this position
			if (i < s_active_path_count - 1) {
				s_active_paths[i] = s_active_paths[s_active_path_count - 1];
			}
			s_active_path_count--;
			
			printk("Audio path disconnected: %d->%d (remaining: %d)\n", 
			       source, sink, s_active_path_count);
			break;
		}
	}

	if (!found) {
		printk("Audio path not found: %d->%d\n", source, sink);
	}

	// Stop streaming if no active paths
	if (s_active_path_count == 0 && s_audio_streaming) {
		s_audio_streaming = false;
		k_mutex_unlock(&s_audio_mutex);
		
		// Wait for both threads to finish
		k_thread_join(&s_audio_input_thread, K_SECONDS(1));
		k_thread_join(&s_audio_output_thread, K_SECONDS(1));
		
		// Stop all active devices
		if (s_mic_active || s_rtx_input_active) {
			AudioRecord_stop(&s_record);
			s_mic_active = false;
			s_rtx_input_active = false;
			printk("Stopped input\n");
		}
		if (s_spk_active) {
			AudioTrack_stop(&s_track_spk);
			s_spk_active = false;
			printk("Stopped SPK output\n");
		}
		if (s_rtx_output_active) {
			AudioTrack_stop(&s_track_rtx_output);
			s_rtx_output_active = false;
			printk("Stopped RTX output\n");
		}
		
		printk("Audio streaming stopped\n");
	} else {
		// Check if we need to stop any devices that are no longer used
		bool mic_needed = false;
		bool rtx_input_needed = false;
		bool spk_needed = false;
		bool rtx_output_needed = false;
		
		// Check if any remaining paths use each device
		for (int i = 0; i < s_active_path_count; i++) {
			if (s_active_paths[i].active) {
				if (s_active_paths[i].source == SOURCE_MIC) mic_needed = true;
				if (s_active_paths[i].source == SOURCE_RTX) rtx_input_needed = true;
				if (s_active_paths[i].sink == SINK_SPK) spk_needed = true;
				if (s_active_paths[i].sink == SINK_RTX) rtx_output_needed = true;
			}
		}
		
		// Stop devices that are no longer needed
		if (!mic_needed && s_mic_active && !rtx_input_needed && s_rtx_input_active) {
			AudioRecord_stop(&s_record);
			s_mic_active = false;
			s_rtx_input_active = false;
			printk("Stopped input\n");
		}
		if (!spk_needed && s_spk_active) {
			AudioTrack_stop(&s_track_spk);
			s_spk_active = false;
			printk("Stopped SPK output\n");
		}
		if (!rtx_output_needed && s_rtx_output_active) {
			AudioTrack_stop(&s_track_rtx_output);
			s_rtx_output_active = false;
			printk("Stopped RTX output\n");
		}
		
		k_mutex_unlock(&s_audio_mutex);
	}
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink   p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink   p2Sink)

{
	// If both paths use the same source, they're incompatible
	if (p1Source == p2Source && p1Source != SOURCE_MCU) {
		return false;
	}

	// If both paths use the same sink, they're incompatible
	if (p1Sink == p2Sink && p1Sink != SINK_MCU) {
		return false;
	}

	// MCU source/sink can be used multiple times (software buffers)
	// Hardware sources/sinks (MIC, SPK, RTX) can only be used once

	return true;
}

