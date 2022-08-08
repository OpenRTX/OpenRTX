#include <DMRCodec.hpp>
#include <interfaces/gpio.h>
#include <interfaces/radio.h>
#include <hwconfig.h>

using namespace DMR;

static bool running = false;
static vocoder_mode_t vocoder_mode;
static std::unique_ptr<uint8_t[]> vocoded_in = std::make_unique<uint8_t[]>(DMR_BUFFER_SIZE);
static std::unique_ptr<uint8_t[]> vocoded_out = std::make_unique<uint8_t[]>(DMR_BUFFER_SIZE);

static void* update(void* arg);

Codec::Codec() : codecThread()
{
}

Codec::~Codec()
{
    stop();
}

void Codec::start(vocoder_mode_t mode)
{
    vocoder_mode = mode;
    radio_initVocoder();
    running = true;
    startThread(update);
}

void Codec::startThread(void *(*func) (void *)) {
    pthread_attr_t codecAttr;
    pthread_attr_init(&codecAttr);
    pthread_attr_setstacksize(&codecAttr, 16384);

    #ifdef _MIOSIX
        // Set priority of vocoder thread to the maximum one, the same of RTX thread.
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(0);
        pthread_attr_setschedparam(&codecAttr, &param);
    #endif

    pthread_create(&codecThread, &codecAttr, func, NULL);
}

static void* update(void* arg)
{
    // static bool led = false;
    while(running)
    {
    //     if (led) {
    //         gpio_clearPin(RED_LED);
    //         gpio_setPin(GREEN_LED);
    //         led = false;
    //     } else {
    //         gpio_setPin(RED_LED);
    //         gpio_clearPin(GREEN_LED);
    //         led = true;
    //     }
    //     //sleepFor(0, 30);
    //     if (vocoder_mode == VOCODER_DIR_DECODE)
    //     {
    //         radio_readVocoder(vocoded_in.get(), DMR_BUFFER_SIZE);
    //         // ambe2Decode(vocoded_in.get(), vocoded_out.get());
    //         //radio_sendVocoder(vocoded_out.get(), DMR_BUFFER_SIZE);
    //     }
    //     else
    //     {
    //         //radio_readVocoder(vocoded_in.get(), DMR_BUFFER_SIZE);
    //         // ambe2Encode(vocoded_in.get(), vocoded_out.get());
    //         //radio_sendVocoder(vocoded_out.get(), DMR_BUFFER_SIZE);
    //     }
        sleepFor(0, 30);
    }
    return (void*) 0;
}

void Codec::stop()
{
    // Disable SPI2
    running = false;
    radio_stopVocoder();
}

bool Codec::isLocked()
{
    // Return true if the DMR frames are being received
    return radio_isDMRLocked();
}
