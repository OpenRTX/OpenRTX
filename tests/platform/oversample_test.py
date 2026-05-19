#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import serial
import time
import argparse
import csv
import numpy as np
from scipy.io.wavfile import write
import sys

def connect_serial(port, baudrate, retries=60, delay=1):
    """
    Connects to the serial port with retries and shows a waiting indicator.
    """
    spinner_chars = ['-', '\\', '|', '/']
    print(f"Attempting to connect to serial port {port}...", flush=True)

    for i in range(retries):
        try:
            ser = serial.Serial(port, baudrate, timeout=1)
            print("\rSuccessfully connected to serial port.   ") # Clear spinner
            return ser
        except serial.SerialException:
            sys.stdout.write(f"\rAttempt {i + 1}/{retries} {spinner_chars[i % len(spinner_chars)]}")
            sys.stdout.flush()
            time.sleep(delay)
    print("\rFailed to connect to serial port after multiple retries.   ") # Clear spinner
    return None

def read_and_decode_data(ser, buffer_size):
    """
    Reads data from the serial port, decodes hex values, and returns a list of integers,
    showing a progress bar.
    """
    data = []
    print(f"Receiving {buffer_size} values from serial...")
    progress_bar_length = 50
    start_time = time.time()

    while len(data) < buffer_size:
        line = ser.readline().strip()
        if line:
            try:
                hex_value = line.decode('ascii')
                data_value = int(hex_value, 16)
                data.append(data_value)

                # Update progress bar
                current_progress = len(data)
                percentage = (current_progress / buffer_size) * 100
                filled_length = int(progress_bar_length * current_progress / buffer_size)
                bar = '█' * filled_length + '-' * (progress_bar_length - filled_length)
                
                # Estimate remaining time
                elapsed_time = time.time() - start_time
                if current_progress > 0:
                    time_per_item = elapsed_time / current_progress
                    remaining_items = buffer_size - current_progress
                    estimated_remaining_time = remaining_items * time_per_item
                    time_str = f" {estimated_remaining_time:.1f}s remaining"
                else:
                    time_str = ""

                sys.stdout.write(f'\rProgress: |{bar}| {percentage:.1f}% ({current_progress}/{buffer_size}){time_str}')
                sys.stdout.flush()
            except (UnicodeDecodeError, ValueError) as e:
                # Clear current line before printing warning
                sys.stdout.write('\r' + ' ' * (progress_bar_length + 60) + '\r')
                sys.stdout.flush()
                print(f"Warning: Could not decode or parse line: {line}. Error: {e}")
                # Re-draw progress bar
                current_progress = len(data) # Recalculate based on actual data
                percentage = (current_progress / buffer_size) * 100
                filled_length = int(progress_bar_length * current_progress / buffer_size)
                bar = '█' * filled_length + '-' * (progress_bar_length - filled_length)
                sys.stdout.write(f'\rProgress: |{bar}| {percentage:.1f}% ({current_progress}/{buffer_size})')
                sys.stdout.flush()

    sys.stdout.write('\n') # New line after progress bar is complete
    print(f"Received {len(data)} values.")
    return data

def normalize_audio(audio_data):
    """
    Normalizes audio data: converts to float, removes DC offset, and scales for no clipping.
    The output is a float array scaled between -1.0 and 1.0.
    """
    #audio_float = (np.array(audio_data, dtype=np.float64) - 32767.5) / 32768.0
    audio_float = np.array(audio_data, dtype=np.float32)

    dc_offset = np.mean(audio_float)
    audio_dc_removed = audio_float - dc_offset

    max_abs_val = np.max(np.abs(audio_dc_removed))

    if max_abs_val > 0:
        scaling_factor = 1.0 / max_abs_val
        audio_normalized = audio_dc_removed * scaling_factor
    else:
        audio_normalized = audio_dc_removed

    audio_normalized = np.clip(audio_normalized, -1.0, 1.0)

    #return audio_normalized.astype(np.float32)
    return audio_normalized

def main():
    parser = argparse.ArgumentParser(
        description="Read audio data from microcontroller, save to CSV and WAV."
    )
    parser.add_argument(
        "filename_base",
        type=str,
        help="Base filename for CSV and WAV files (e.g., 'audio_capture')"
    )
    parser.add_argument(
        "--port",
        type=str,
        default="/dev/ttyACM",
        help="Serial port (e.g., /dev/ttyACM0 or COM3)"
    )
    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Serial baud rate"
    )
    parser.add_argument(
        "--buffer_size",
        type=int,
        default=45 * 1024,
        help="Number of 16-bit unsigned integers in the microcontroller's buffer"
    )
    parser.add_argument(
        "--samplerate",
        type=int,
        default=8000,
        help="Sample rate of the microphone in Hz"
    )

    args = parser.parse_args()

    csv_filename = f"{args.filename_base}.csv"
    wav_filename = f"{args.filename_base}.wav"

    ser = connect_serial(args.port, args.baudrate)
    if not ser:
        return

    try:
        raw_data = read_and_decode_data(ser, args.buffer_size)

        print(f"Saving raw data to {csv_filename}...")
        with open(csv_filename, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            #writer.writerow(['Raw_Value'])
            for value in raw_data:
                writer.writerow([value])
        print("Raw data saved to CSV.")

        if raw_data:
            print("Normalizing audio data to float format...")
            normalized_audio_float = normalize_audio(raw_data)

            print(f"Saving normalized audio to {wav_filename} (float format)...")
            write(wav_filename, args.samplerate, normalized_audio_float)
            print("Normalized audio saved to WAV.")
        else:
            print("No audio data to normalize or save to WAV.")

    finally:
        ser.close()
        print("Serial connection closed.")

if __name__ == "__main__":
    main()

