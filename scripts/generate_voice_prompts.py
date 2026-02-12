#!/usr/bin/env python3
"""
Generate OpenRTX voice prompts from C source headers.

This script:
1. Parses voicePrompts.h to extract the PROMPT_* enum
2. Parses ui_strings.h to extract stringsTable_t field names
3. Parses EnglishStrings.h to get the actual text values
4. Generates TTS audio using Piper
5. Processes audio files (ffmpeg resample to 8kHz, c2enc to codec2)
6. Packages everything into a VPC binary file

SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""

import argparse
import json
import re
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path
from typing import Dict, List, Tuple


class VoicePromptGenerator:
    """Generate voice prompts for OpenRTX from source headers."""
    
    # Magic number and version for VPC file format
    VPC_MAGIC = 0x5056  # 'VP'
    VPC_VERSION = 0x1000  # v1.0
    MAX_PROMPTS = 350  # Maximum number of prompts in ToC
    
    def __init__(self, project_root: Path, overrides_file: Path = None):
        self.project_root = project_root
        self.overrides = self._load_overrides(overrides_file) if overrides_file else {}
        self.openrtx_dir = project_root / "openrtx"
        
    def _load_overrides(self, overrides_file: Path) -> Dict[str, str]:
        """Load pronunciation overrides from JSON file."""
        if not overrides_file.exists():
            print(f"Warning: Overrides file not found: {overrides_file}")
            return {}
        
        with open(overrides_file, 'r') as f:
            return json.load(f)
    
    def parse_prompt_enum(self) -> List[Tuple[str, str]]:
        """
        Parse voicePrompts.h to extract PROMPT_* enum values with comments.
        
        Returns:
            List of (enum_name, comment_text) tuples
        """
        header_file = self.openrtx_dir / "include/core/voicePrompts.h"
        
        with open(header_file, 'r') as f:
            content = f.read()
        
        # Find the voicePrompt_t enum
        enum_pattern = r'typedef\s+enum\s*\{(.*?)\}\s*voicePrompt_t;'
        match = re.search(enum_pattern, content, re.DOTALL)
        
        if not match:
            raise ValueError("Could not find voicePrompt_t enum in voicePrompts.h")
        
        enum_body = match.group(1)
        
        # Extract enum entries with comments
        # Pattern: PROMPT_NAME, // comment (on same line)
        lines = enum_body.split('\n')
        entries = []
        
        for line in lines:
            line = line.strip()
            if not line or line.startswith('//'):
                continue
            
            # Match: PROMPT_NAME, // comment
            match = re.match(r'(PROMPT_\w+)\s*,?\s*(?://\s*(.*))?', line)
            if match:
                name = match.group(1)
                
                # Stop at NUM_VOICE_PROMPTS
                if name == "NUM_VOICE_PROMPTS":
                    break
                
                comment = match.group(2).strip() if match.group(2) else ""
                entries.append((name, comment))
        
        return entries
    
    def parse_strings_table_fields(self) -> List[str]:
        """
        Parse ui_strings.h to extract stringsTable_t field names.
        
        Returns:
            List of field names
        """
        header_file = self.openrtx_dir / "include/ui/ui_strings.h"
        
        with open(header_file, 'r') as f:
            content = f.read()
        
        # Find the stringsTable_t typedef struct
        struct_pattern = r'typedef\s+struct\s*\{(.*?)\}\s*stringsTable_t;'
        match = re.search(struct_pattern, content, re.DOTALL)
        
        if not match:
            raise ValueError("Could not find stringsTable_t in ui_strings.h")
        
        struct_body = match.group(1)
        
        # Extract field names: const char* fieldName;
        field_pattern = r'const\s+char\s*\*\s*(\w+)\s*;'
        fields = re.findall(field_pattern, struct_body)
        
        return fields
    
    def parse_english_strings(self) -> Dict[str, str]:
        """
        Parse EnglishStrings.h to extract text values.
        
        Returns:
            Dictionary mapping field names to text values
        """
        header_file = self.openrtx_dir / "include/ui/EnglishStrings.h"
        
        with open(header_file, 'r') as f:
            content = f.read()
        
        # Find the englishStrings initialization
        # Pattern: const stringsTable_t englishStrings = { ... };
        init_pattern = r'const\s+stringsTable_t\s+englishStrings\s*=\s*\{(.*?)\};'
        match = re.search(init_pattern, content, re.DOTALL)
        
        if not match:
            raise ValueError("Could not find englishStrings in EnglishStrings.h")
        
        init_body = match.group(1)
        
        # Extract field assignments: .fieldName = "value"
        assignment_pattern = r'\.(\w+)\s*=\s*"([^"]*)"'
        assignments = {}
        
        for match in re.finditer(assignment_pattern, init_body):
            field = match.group(1)
            value = match.group(2)
            assignments[field] = value
        
        return assignments
    
    def get_prompt_text(self, enum_name: str, comment: str, 
                       string_fields: Dict[str, str]) -> str:
        """
        Get the text to speak for a given prompt.
        
        Args:
            enum_name: The PROMPT_* enum name
            comment: The comment from the enum
            string_fields: Dictionary of string table values
        
        Returns:
            Text to be spoken
        """
        # Check overrides first
        if enum_name in self.overrides:
            return self.overrides[enum_name]
        
        # Special handling for specific prompts
        if enum_name == "PROMPT_SILENCE":
            return ""  # Empty/silence
        
        # Numbers 0-9
        if enum_name.startswith("PROMPT_") and enum_name[7:].isdigit():
            return enum_name[7:]
        
        # Letters A-Z
        if re.match(r'PROMPT_[A-Z]$', enum_name):
            return enum_name[7:]
        
        # Phonetic alphabet
        if enum_name.endswith("_PHONETIC"):
            # Use the comment which contains the phonetic word
            return comment if comment else enum_name.replace("_PHONETIC", "")
        
        # Use comment if available and non-empty
        if comment:
            return comment
        
        # Try to match with string table fields
        # Convert PROMPT_GPS_OFF -> gpsOff
        field_name = self._prompt_to_field_name(enum_name)
        if field_name in string_fields:
            return string_fields[field_name]
        
        # Fallback: use enum name without PROMPT_ prefix
        return enum_name.replace("PROMPT_", "").replace("_", " ")
    
    def _prompt_to_field_name(self, enum_name: str) -> str:
        """
        Convert PROMPT_GPS_OFF to gpsOff (camelCase).
        
        Args:
            enum_name: The PROMPT_* enum name
        
        Returns:
            Corresponding field name in camelCase
        """
        # Remove PROMPT_ prefix
        name = enum_name.replace("PROMPT_", "")
        
        # Split by underscore and convert to camelCase
        parts = name.split("_")
        if not parts:
            return name.lower()
        
        # First word lowercase, rest capitalized
        return parts[0].lower() + "".join(p.capitalize() for p in parts[1:])
    
    def generate_tts(self, text: str, output_wav: Path, voice_model: str = "en_US-lessac-medium",
                     data_dir: Path = None):
        """
        Generate TTS audio using Piper.
        
        Args:
            text: Text to speak
            output_wav: Output WAV file path
            voice_model: Piper voice model name
            data_dir: Directory containing downloaded voice models
        """
        if not text:  # Silence
            # Create 0.1 second silent WAV
            with wave.open(str(output_wav), 'wb') as wav:
                wav.setnchannels(1)
                wav.setsampwidth(2)
                wav.setframerate(22050)
                wav.writeframes(b'\x00\x00' * 2205)  # 0.1 seconds
            return
        
        # Use Piper command-line tool
        try:
            cmd = [
                'piper',
                '--model', voice_model,
                '--output_file', str(output_wav)
            ]
            if data_dir:
                cmd.extend(['--data-dir', str(data_dir)])
            subprocess.run(cmd, input=text.encode('utf-8'), check=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        except subprocess.CalledProcessError as e:
            print(f"Error generating TTS for '{text}': {e.stderr.decode()}")
            raise
        except FileNotFoundError:
            print("Error: 'piper' command not found. Please install piper-tts:")
            print("  pip install piper-tts")
            sys.exit(1)
    
    def convert_to_8khz_raw(self, wav_file: Path, raw_file: Path, 
                           gain_db: float = 0.0, tempo: float = 1.25):
        """
        Convert WAV to 8kHz 16-bit signed PCM raw using ffmpeg.
        
        Args:
            wav_file: Input WAV file
            raw_file: Output raw file
            gain_db: Volume adjustment in dB
            tempo: Audio tempo adjustment
        """
        try:
            subprocess.run([
                'ffmpeg', '-y', '-i', str(wav_file),
                '-filter:a', f'atempo={tempo},volume={gain_db}dB',
                '-ar', '8000',
                '-f', 's16le',
                str(raw_file)
            ], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        except subprocess.CalledProcessError as e:
            print(f"Error converting {wav_file} to raw: {e.stderr.decode()}")
            raise
        except FileNotFoundError:
            print("Error: 'ffmpeg' command not found. Please install ffmpeg.")
            sys.exit(1)
    
    def encode_codec2(self, raw_file: Path, c2_file: Path, mode: int = 3200):
        """
        Encode raw audio to Codec2 format using c2enc.
        
        Args:
            raw_file: Input 8kHz raw file
            c2_file: Output codec2 file
            mode: Codec2 bitrate mode (3200 bps)
        """
        try:
            subprocess.run([
                'c2enc', str(mode), str(raw_file), str(c2_file)
            ], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        except subprocess.CalledProcessError as e:
            print(f"Error encoding {raw_file} to codec2: {e.stderr.decode()}")
            raise
        except FileNotFoundError:
            print("Error: 'c2enc' command not found. Please install codec2.")
            sys.exit(1)
    
    def build_vpc(self, c2_files: List[Path], output_vpc: Path):
        """
        Package codec2 files into VPC binary format.
        
        VPC Format:
        - Header (12 bytes): magic (4), version (4), first_offset (4)
        - ToC (350 * 4 bytes): offsets to each codec2 data
        - Audio data: concatenated codec2 files
        
        Args:
            c2_files: List of codec2 file paths in order
            output_vpc: Output VPC file path
        """
        # Build table of contents and collect audio data
        toc = [0] * self.MAX_PROMPTS
        audio_data = bytearray()
        
        for i, c2_file in enumerate(c2_files):
            if i >= self.MAX_PROMPTS:
                print(f"Warning: Too many prompts ({len(c2_files)}), truncating to {self.MAX_PROMPTS}")
                break
            
            if c2_file.exists():
                c2_data = c2_file.read_bytes()
                toc[i] = len(audio_data)
                audio_data.extend(c2_data)
            else:
                print(f"Warning: Missing codec2 file: {c2_file}")
                toc[i] = len(audio_data)  # Point to current position (empty)
        
        # Fill remaining ToC entries with total audio size as sentinel.
        # The C code computes each prompt's length as toc[N+1] - toc[N],
        # so the entry after the last prompt must equal the total audio size
        # to avoid a uint32_t underflow that wraps to ~4GB.
        total_audio_size = len(audio_data)
        for i in range(len(c2_files), self.MAX_PROMPTS):
            toc[i] = total_audio_size
        
        # Write VPC file
        with open(output_vpc, 'wb') as f:
            # Header
            f.write(struct.pack('<I', self.VPC_MAGIC))    # Magic number
            f.write(struct.pack('<I', self.VPC_VERSION))  # Version
            f.write(struct.pack('<I', 0))                 # First prompt at offset 0
            
            # Table of contents (remaining offsets)
            for offset in toc[1:]:
                f.write(struct.pack('<I', offset))
            
            # Audio data
            f.write(audio_data)
        
        print(f"Built VPC file: {output_vpc} ({len(audio_data)} bytes of audio data)")
    
    def generate_all(self, output_vpc: Path, temp_dir: Path = None,
                    voice_model: str = "en_US-lessac-medium",
                    data_dir: Path = None,
                    gain_db: float = 0.0, tempo: float = 1.25,
                    keep_temp: bool = False):
        """
        Generate complete voice prompt VPC file.
        
        Args:
            output_vpc: Output VPC file path
            temp_dir: Temporary directory for intermediate files
            voice_model: Piper voice model to use
            data_dir: Directory containing downloaded voice models
            gain_db: Volume adjustment in dB
            tempo: Audio tempo adjustment
            keep_temp: Keep temporary files for debugging
        """
        # Parse headers
        print("Parsing C headers...")
        prompt_enum = self.parse_prompt_enum()
        string_fields_list = self.parse_strings_table_fields()
        english_strings = self.parse_english_strings()
        
        print(f"Found {len(prompt_enum)} voice prompts")
        print(f"Found {len(string_fields_list)} string table entries")
        
        # Create temporary directory
        if temp_dir is None:
            temp_dir = Path(tempfile.mkdtemp(prefix="openrtx_vpc_"))
        else:
            temp_dir.mkdir(parents=True, exist_ok=True)
        
        print(f"Using temporary directory: {temp_dir}")
        
        try:
            c2_files = []
            
            # Generate prompts from enum
            print(f"\nGenerating {len(prompt_enum)} voice prompts...")
            for i, (enum_name, comment) in enumerate(prompt_enum):
                text = self.get_prompt_text(enum_name, comment, english_strings)
                
                # File paths
                wav_file = temp_dir / f"{i:03d}_{enum_name}.wav"
                raw_file = temp_dir / f"{i:03d}_{enum_name}.raw"
                c2_file = temp_dir / f"{i:03d}_{enum_name}.c2"
                
                print(f"  [{i+1}/{len(prompt_enum)}] {enum_name}: '{text}'")
                
                # Generate and process
                self.generate_tts(text, wav_file, voice_model, data_dir)
                self.convert_to_8khz_raw(wav_file, raw_file, gain_db, tempo)
                self.encode_codec2(raw_file, c2_file)
                
                c2_files.append(c2_file)
                
                # Clean up intermediate files unless keeping
                if not keep_temp:
                    wav_file.unlink(missing_ok=True)
                    raw_file.unlink(missing_ok=True)
            
            # Generate prompts from string table
            print(f"\nGenerating {len(string_fields_list)} string table prompts...")
            for i, field_name in enumerate(string_fields_list):
                text = english_strings.get(field_name, field_name)
                
                # Check for override
                override_key = f"STRING_{field_name}"
                if override_key in self.overrides:
                    text = self.overrides[override_key]
                
                prompt_idx = len(prompt_enum) + i
                
                # File paths
                wav_file = temp_dir / f"{prompt_idx:03d}_STRING_{field_name}.wav"
                raw_file = temp_dir / f"{prompt_idx:03d}_STRING_{field_name}.raw"
                c2_file = temp_dir / f"{prompt_idx:03d}_STRING_{field_name}.c2"
                
                print(f"  [{i+1}/{len(string_fields_list)}] {field_name}: '{text}'")
                
                # Generate and process
                self.generate_tts(text, wav_file, voice_model, data_dir)
                self.convert_to_8khz_raw(wav_file, raw_file, gain_db, tempo)
                self.encode_codec2(raw_file, c2_file)
                
                c2_files.append(c2_file)
                
                # Clean up intermediate files unless keeping
                if not keep_temp:
                    wav_file.unlink(missing_ok=True)
                    raw_file.unlink(missing_ok=True)
            
            # Add voice name as last prompt
            voice_name = "English"
            if "PROMPT_VOICE_NAME" in self.overrides:
                voice_name = self.overrides["PROMPT_VOICE_NAME"]
            
            prompt_idx = len(c2_files)
            wav_file = temp_dir / f"{prompt_idx:03d}_VOICE_NAME.wav"
            raw_file = temp_dir / f"{prompt_idx:03d}_VOICE_NAME.raw"
            c2_file = temp_dir / f"{prompt_idx:03d}_VOICE_NAME.c2"
            
            print(f"\nGenerating voice name: '{voice_name}'")
            self.generate_tts(voice_name, wav_file, voice_model, data_dir)
            self.convert_to_8khz_raw(wav_file, raw_file, gain_db, tempo)
            self.encode_codec2(raw_file, c2_file)
            c2_files.append(c2_file)
            
            if not keep_temp:
                wav_file.unlink(missing_ok=True)
                raw_file.unlink(missing_ok=True)
            
            # Build VPC file
            print(f"\nBuilding VPC file...")
            self.build_vpc(c2_files, output_vpc)
            
            print(f"\n✓ Successfully generated: {output_vpc}")
            print(f"  Total prompts: {len(c2_files)}")
            print(f"  File size: {output_vpc.stat().st_size / 1024:.1f} KB")
            
        finally:
            # Clean up temporary directory
            if not keep_temp:
                import shutil
                shutil.rmtree(temp_dir, ignore_errors=True)
                print(f"\nCleaned up temporary directory")
            else:
                print(f"\nTemporary files kept in: {temp_dir}")


def main():
    parser = argparse.ArgumentParser(
        description='Generate OpenRTX voice prompts from C source headers',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate with default settings
  %(prog)s

  # Generate with custom voice model
  %(prog)s --voice en_GB-alan-medium

  # Keep temporary files for debugging
  %(prog)s --keep-temp

  # Use pronunciation overrides
  %(prog)s --overrides scripts/voice_overrides.json
        """
    )
    
    parser.add_argument(
        '-o', '--output',
        type=Path,
        default=Path('voiceprompts.vpc'),
        help='Output VPC file path (default: voiceprompts.vpc)'
    )
    
    parser.add_argument(
        '--overrides',
        type=Path,
        help='JSON file with pronunciation overrides'
    )
    
    parser.add_argument(
        '--voice',
        default='en_US-lessac-medium',
        help='Piper voice model (default: en_US-lessac-medium)'
    )
    
    parser.add_argument(
        '--gain',
        type=float,
        default=0.0,
        help='Volume adjustment in dB (default: 0.0)'
    )
    
    parser.add_argument(
        '--tempo',
        type=float,
        default=1.25,
        help='Audio tempo multiplier (default: 1.25)'
    )
    
    parser.add_argument(
        '--data-dir',
        type=Path,
        help='Directory containing downloaded Piper voice models'
    )

    parser.add_argument(
        '--temp-dir',
        type=Path,
        help='Temporary directory for intermediate files'
    )
    
    parser.add_argument(
        '--keep-temp',
        action='store_true',
        help='Keep temporary files for debugging'
    )
    
    parser.add_argument(
        '--project-root',
        type=Path,
        default=Path(__file__).parent.parent,
        help='OpenRTX project root directory'
    )
    
    args = parser.parse_args()
    
    # Create generator
    generator = VoicePromptGenerator(
        project_root=args.project_root,
        overrides_file=args.overrides
    )
    
    # Generate VPC file
    try:
        generator.generate_all(
            output_vpc=args.output,
            temp_dir=args.temp_dir,
            voice_model=args.voice,
            data_dir=args.data_dir,
            gain_db=args.gain,
            tempo=args.tempo,
            keep_temp=args.keep_temp
        )
    except Exception as e:
        print(f"\nError: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
