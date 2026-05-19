#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import argparse
import os
import math
from scipy.io.wavfile import write
import numpy as np

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
    #print(dc_offset, max_abs_val, np.max(np.abs(audio_normalized)))
 
    #return audio_normalized.astype(np.float32)
    return audio_normalized

def calculate_stats(directory_path):
    print("File\t\tDC offset\tRMS")
    files_to_process = []
    for filename in os.listdir(directory_path):
        if filename.endswith(".csv"):
            files_to_process.append(filename)
    
    files_to_process.sort()

    for filename in files_to_process:
        filepath = os.path.join(directory_path, filename)
        numbers = []
        
        with open(filepath, 'r') as f:
            for line in f:
                try:
                    numbers.append(float(line.strip()))
                except ValueError:
                    continue

        normalized_audio_float = normalize_audio(numbers[1:])
        wav_filename = filepath.replace('.csv', '.wav')
        write(wav_filename, 8000, normalized_audio_float)

        if numbers:
            # Calculate DC offset (average)
            dc_offset = sum(numbers) / len(numbers)
            
            # Subtract DC offset and calculate sum of squares for RMS
            sum_sq_after_dc = 0.0
            for num in numbers:
                sum_sq_after_dc += (num - dc_offset) ** 2
            
            rms_after_dc = math.sqrt(sum_sq_after_dc / len(numbers))
            
            print(f"{filename}\t{dc_offset:8.1f}\t{rms_after_dc:8.2f}")
        else:
            print(f"{filename}\tN/A")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Calculate RMS after subtracting DC offset from single-column CSV files.")
    parser.add_argument("path", help="Path to the directory containing CSV files.")
    args = parser.parse_args()

    if not os.path.isdir(args.path):
        print(f"Error: Directory not found at '{args.path}'")
    else:
        calculate_stats(args.path)
