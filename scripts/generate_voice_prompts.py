#!/usr/bin/env python3
"""
Generate OpenRTX voice prompts from C source headers.

This script:
1. Parses voicePrompts.h to extract the PROMPT_* enum
2. Parses ui_strings.h to extract stringsTable_t field names and text values
3. Generates TTS audio using Kokoro
4. Processes audio files (ffmpeg resample to 8kHz, c2enc to codec2)
5. Packages everything into a VPC binary file

SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""

import argparse
import json
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, List, Tuple

from kokoro_onnx import Kokoro


class VoicePromptGenerator:
    """Generate voice prompts for OpenRTX from source headers."""
    
    # Magic number and version for VPC file format
    VPC_MAGIC = 0x5056  # 'VP'
    VPC_VERSION = 0x1000  # v1.0

    def __init__(self, project_root: Path, overrides_file: Path = None,
                 voice_prompts_header: Path = None,
                 ui_strings_header: Path = None):
        self.project_root = project_root
        self.overrides = self._load_overrides(overrides_file) if overrides_file else {}
        openrtx_dir = project_root / "openrtx"
        self.voice_prompts_header = voice_prompts_header or openrtx_dir / "include/core/voicePrompts.h"
        self.ui_strings_header = ui_strings_header or openrtx_dir / "src/ui/default/ui_strings.c"
        self.max_prompts = self._parse_toc_size(
            openrtx_dir / "src/core/voicePrompts.c")

    @staticmethod
    def _parse_toc_size(voice_prompts_source: Path) -> int:
        """
        Read the ToC size from VOICE_PROMPTS_TOC_SIZE in voicePrompts.c, so
        the generated file layout can never drift from what the firmware
        expects.
        """
        with open(voice_prompts_source, 'r') as f:
            content = f.read()

        match = re.search(r'#define\s+VOICE_PROMPTS_TOC_SIZE\s+(\d+)', content)
        if not match:
            raise ValueError(
                f"Could not find VOICE_PROMPTS_TOC_SIZE in {voice_prompts_source}")

        return int(match.group(1))

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
        header_file = self.voice_prompts_header
        
        with open(header_file, 'r') as f:
            content = f.read()
        
        # Find the voicePrompt enum
        enum_pattern = r'enum\s+voicePrompt\s*\{(.*?)\};'
        match = re.search(enum_pattern, content, re.DOTALL)
        
        if not match:
            raise ValueError("Could not find voicePrompt enum in voicePrompts.h")
        
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
    
    def parse_strings_table(self) -> List[Tuple[str, str]]:
        """
        Parse ui_strings.h to extract stringsTable_t field names and text values
        from the englishStrings initializer.
        
        Returns:
            List of (field_name, text_value) tuples in declaration order
        """
        header_file = self.ui_strings_header
        
        with open(header_file, 'r') as f:
            content = f.read()
        
        # Find the englishStrings initialization
        init_pattern = r'(?:static\s+)?const\s+stringsTable_t\s+englishStrings\s*=\s*\{(.*?)\};'
        match = re.search(init_pattern, content, re.DOTALL)
        
        if not match:
            raise ValueError("Could not find englishStrings initializer")
        
        init_body = match.group(1)
        
        # Extract field assignments: .fieldName = "value"
        assignment_pattern = r'\.(\w+)\s*=\s*"([^"]*)"'
        entries = []
        
        for match in re.finditer(assignment_pattern, init_body):
            entries.append((match.group(1), match.group(2)))
        
        return entries
    
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
    
    @staticmethod
    def _resolve_kokoro_files(model_dir: Path) -> tuple:
        """
        Resolve Kokoro model files (.onnx and .bin), downloading if necessary.
        
        Args:
            model_dir: Directory to cache Kokoro model files
        
        Returns:
            Tuple of (onnx_path, voices_bin_path)
        """
        onnx_url = (
            "https://github.com/thewh1teagle/kokoro-onnx/releases/download/"
            "model-files-v1.0/kokoro-v1.0.onnx"
        )
        voices_url = (
            "https://github.com/thewh1teagle/kokoro-onnx/releases/download/"
            "model-files-v1.0/voices-v1.0.bin"
        )
        
        onnx_path = model_dir / "kokoro-v1.0.onnx"
        voices_path = model_dir / "voices-v1.0.bin"
        
        # Download ONNX model if missing
        if not onnx_path.exists():
            print(f"Downloading Kokoro ONNX model to {model_dir}...")
            model_dir.mkdir(parents=True, exist_ok=True)
            subprocess.run(
                ["curl", "-L", "--fail", "-o", str(onnx_path), onnx_url],
                check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
        
        # Download voices file if missing
        if not voices_path.exists():
            print(f"Downloading Kokoro voices file to {model_dir}...")
            model_dir.mkdir(parents=True, exist_ok=True)
            subprocess.run(
                ["curl", "-L", "--fail", "-o", str(voices_path), voices_url],
                check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
        
        return onnx_path, voices_path

    def generate_tts(self, text: str, output_wav: Path,
                     kokoro: Kokoro, voice_name: str = "am_michael",
                     lang_code: str = "en-gb", speed: float = 1.0):
        """
        Generate TTS audio using the Kokoro Python API.

        Args:
            text: Text to speak
            output_wav: Output WAV file path
            kokoro: Loaded Kokoro instance
            voice_name: Kokoro voice name (e.g. 'am_michael')
            lang_code: Language code for Kokoro
            speed: Speech speed multiplier (1.0 = normal)
        """
        import soundfile as sf
        
        if not text:  # Silence
            # Create 0.1 second silent WAV at 24kHz (Kokoro's native rate)
            silence_duration = 0.1
            sample_rate = 24000
            samples = [0.0] * int(silence_duration * sample_rate)
            sf.write(str(output_wav), samples, sample_rate)
            return
        
        # Generate audio using Kokoro (returns 24kHz samples)
        samples, sample_rate = kokoro.create(
            text,
            voice=voice_name,
            speed=speed,
            lang=lang_code
        )
        
        # Write WAV file using soundfile
        sf.write(str(output_wav), samples, sample_rate)
    
    def convert_to_8khz_raw(self, wav_file: Path, raw_file: Path,
                           gain_db: float = 0.0):
        """
        Convert WAV to 8kHz 16-bit signed PCM raw using ffmpeg.

        Args:
            wav_file: Input WAV file
            raw_file: Output raw file
            gain_db: Volume adjustment in dB
        """
        try:
            subprocess.run([
                'ffmpeg', '-y', '-i', str(wav_file),
                '-filter:a', f'volume={gain_db}dB',
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
        - Header (8 bytes): magic (4), version (4)
        - ToC (VOICE_PROMPTS_TOC_SIZE * 4 bytes): uint32 offsets to each
          codec2 data
        - Audio data: concatenated codec2 files
        
        Args:
            c2_files: List of codec2 file paths in order
            output_vpc: Output VPC file path
        """
        # Build table of contents and collect audio data
        toc = [0] * self.max_prompts
        audio_data = bytearray()

        for i, c2_file in enumerate(c2_files):
            if i >= self.max_prompts:
                print(f"Warning: Too many prompts ({len(c2_files)}), truncating to {self.max_prompts}")
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
        for i in range(len(c2_files), self.max_prompts):
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

    def _split_vpc(self, data: bytes) -> List[bytes]:
        """
        Split a VPC file's audio body into per-prompt segments using its
        table of contents.
        """
        HEADER_SIZE = 8          # magic (4) + version (4)
        META_SIZE = HEADER_SIZE + self.max_prompts * 4

        toc = struct.unpack(f'<{self.max_prompts}I',
                            data[HEADER_SIZE:META_SIZE])
        audio = data[META_SIZE:]

        bounds = list(toc) + [len(audio)]
        return [audio[bounds[i]:bounds[i + 1]]
                for i in range(self.max_prompts)]

    def check_vpc(self, reference: Path, generated: Path,
                  threshold: float = 0.01, frame_slack: int = 2) -> bool:
        """
        Compare two VPC files, allowing minor audio drift from
        cross-platform differences in ONNX inference. Inference on a
        different platform can produce a slightly longer or shorter
        rendering of a prompt, so each prompt is compared separately:
        its length may differ by a few codec2 frames and its overlapping
        audio by a fraction of bits, rather than requiring a byte-exact
        table of contents.

        Args:
            reference:   The committed VPC file.
            generated:   The freshly generated VPC file.
            threshold:   Maximum overall fraction of differing *bits* in
                         the overlapping audio (0.0–1.0). Default 0.01 (1%).
            frame_slack: Maximum per-prompt length difference, in codec2
                         frames. Default 2.

        Returns:
            True if the files are equivalent within the tolerances.
        """
        CODEC2_FRAME_SIZE = 8    # bytes per codec2 3200bps frame

        ref_data = reference.read_bytes()
        gen_data = generated.read_bytes()

        # --- header comparison ---
        if ref_data[:8] != gen_data[:8]:
            print("FAIL: VPC header differs")
            return False

        # --- per-prompt comparison ---
        ref_prompts = self._split_vpc(ref_data)
        gen_prompts = self._split_vpc(gen_data)

        total_bits = 0
        diff_bits = 0
        length_ok = True

        for i, (ref, gen) in enumerate(zip(ref_prompts, gen_prompts)):
            slack = abs(len(ref) - len(gen))
            if slack > frame_slack * CODEC2_FRAME_SIZE:
                print(f"FAIL: prompt {i} length differs by {slack} bytes "
                      f"({len(ref)} vs {len(gen)}, "
                      f"allowed {frame_slack * CODEC2_FRAME_SIZE})")
                length_ok = False
                continue

            common = min(len(ref), len(gen))
            total_bits += common * 8
            diff_bits += sum(bin(a ^ b).count('1')
                             for a, b in zip(ref[:common], gen[:common]))

        if not length_ok:
            return False

        if total_bits == 0:
            # No audio data in either file — trivially equal.
            return True

        diff_pct = diff_bits / total_bits
        print(f"Audio body: {diff_bits}/{total_bits} bits differ "
              f"({diff_pct:.4%})")

        if diff_pct > threshold:
            print(f"FAIL: audio drift {diff_pct:.4%} exceeds "
                  f"threshold {threshold:.4%}")
            return False

        print("OK: VPC file is within tolerance")
        return True

    def _synth_prompt(self, text: str, stem: str, temp_dir: Path,
                      kokoro, voice_name: str, tempo: float, gain_db: float,
                      keep_temp: bool) -> Path:
        """
        Run one prompt through the full synth pipeline: TTS to wav, resample
        to 8 kHz raw, encode to codec2. Intermediate wav/raw files are
        removed unless keep_temp is set.

        Args:
            text: Text to synthesize
            stem: File name stem for the intermediate and output files

        Returns:
            Path of the resulting codec2 file
        """
        wav_file = temp_dir / f"{stem}.wav"
        raw_file = temp_dir / f"{stem}.raw"
        c2_file = temp_dir / f"{stem}.c2"

        self.generate_tts(text, wav_file, kokoro, voice_name, speed=tempo)
        self.convert_to_8khz_raw(wav_file, raw_file, gain_db)
        self.encode_codec2(raw_file, c2_file)

        if not keep_temp:
            wav_file.unlink(missing_ok=True)
            raw_file.unlink(missing_ok=True)

        return c2_file

    def generate_all(self, output_vpc: Path, temp_dir: Path = None,
                    voice_name: str = "am_michael",
                    model_dir: Path = None,
                    gain_db: float = 0.0, tempo: float = 1.3,
                    keep_temp: bool = False):
        """
        Generate complete voice prompt VPC file.

        Args:
            output_vpc: Output VPC file path
            temp_dir: Temporary directory for intermediate files
            voice_name: Kokoro voice name (e.g. 'am_michael')
            model_dir: Directory containing Kokoro model files
            gain_db: Volume adjustment in dB
            tempo: Speech speed multiplier (passed to Kokoro)
            keep_temp: Keep temporary files for debugging
        """
        # Parse headers
        print("Parsing C headers...")
        prompt_enum = self.parse_prompt_enum()
        string_table = self.parse_strings_table()
        english_strings = dict(string_table)
        
        print(f"Found {len(prompt_enum)} voice prompts")
        print(f"Found {len(string_table)} string table entries")
        
        # Load Kokoro model files
        if model_dir is None:
            model_dir = Path.home() / ".cache" / "kokoro-tts"
        
        onnx_path, voices_path = self._resolve_kokoro_files(model_dir)
        print(f"Loading Kokoro model: {onnx_path}")
        kokoro = Kokoro(str(onnx_path), str(voices_path))
        
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

                print(f"  [{i+1}/{len(prompt_enum)}] {enum_name}: '{text}'")

                c2_files.append(self._synth_prompt(
                    text, f"{i:03d}_{enum_name}", temp_dir, kokoro,
                    voice_name, tempo, gain_db, keep_temp))
            
            # Generate prompts from string table
            print(f"\nGenerating {len(string_table)} string table prompts...")
            for i, (field_name, text) in enumerate(string_table):
                
                # Check for override
                override_key = f"STRING_{field_name}"
                if override_key in self.overrides:
                    text = self.overrides[override_key]
                
                prompt_idx = len(prompt_enum) + i

                print(f"  [{i+1}/{len(string_table)}] {field_name}: '{text}'")

                c2_files.append(self._synth_prompt(
                    text, f"{prompt_idx:03d}_STRING_{field_name}", temp_dir,
                    kokoro, voice_name, tempo, gain_db, keep_temp))
            
            # Add voice name as last prompt
            voice_text = "English"
            if "PROMPT_VOICE_NAME" in self.overrides:
                voice_text = self.overrides["PROMPT_VOICE_NAME"]
            
            prompt_idx = len(c2_files)

            print(f"\nGenerating voice name: '{voice_text}'")

            c2_files.append(self._synth_prompt(
                voice_text, f"{prompt_idx:03d}_VOICE_NAME", temp_dir, kokoro,
                voice_name, tempo, gain_db, keep_temp))

            # Build VPC file
            print(f"\nBuilding VPC file...")
            self.build_vpc(c2_files, output_vpc)
            
            # Touch the assembly file so ninja knows to recompile
            # (voicePromptData.S embeds voiceprompts.vpc via .incbin)
            s_file = self.project_root / "openrtx" / "src" / "core" / "voicePromptData.S"
            if s_file.exists():
                print(f"\nTouching {s_file} to force rebuild...")
                s_file.touch()
            
            print(f"\n✓ Successfully generated: {output_vpc}")
            print(f"  Total prompts: {len(c2_files)}")
            print(f"  File size: {output_vpc.stat().st_size / 1024:.1f} KB")
            
        finally:
            # Clean up temporary directory
            if not keep_temp:
                shutil.rmtree(temp_dir, ignore_errors=True)
                print(f"\nCleaned up temporary directory")
            else:
                print(f"\nTemporary files kept in: {temp_dir}")


def _check_dependencies():
    """Check that required external tools are available."""
    missing = []
    for cmd, install_hint in [
        ('ffmpeg', 'apt install ffmpeg'),
        ('c2enc', 'apt install codec2'),
    ]:
        if shutil.which(cmd) is None:
            missing.append(f"  {cmd} (install with: {install_hint})")
    if missing:
        print("Missing required dependencies:", file=sys.stderr)
        print("\n".join(missing), file=sys.stderr)
        sys.exit(1)


def main():
    script_dir = Path(__file__).parent
    default_overrides = script_dir / 'voice_overrides.json'
    default_model_dir = Path.home() / '.cache' / 'kokoro-tts'

    parser = argparse.ArgumentParser(
        description='Generate OpenRTX voice prompts from C source headers',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate with default settings
  %(prog)s

  # Generate with custom voice name
  %(prog)s --voice am_michael

  # Keep temporary files for debugging
  %(prog)s --keep-temp

  # Disable pronunciation overrides
  %(prog)s --no-overrides

  # Verify committed VPC is still up-to-date (CI mode)
  %(prog)s --check --voice am_michael --tempo 1.3
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
        default=default_overrides,
        help=f'JSON file with pronunciation overrides (default: {default_overrides})'
    )

    parser.add_argument(
        '--no-overrides',
        action='store_true',
        help='Disable pronunciation overrides'
    )
    
    parser.add_argument(
        '--voice',
        default='am_michael',
        help='Kokoro voice name (default: am_michael)'
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
        default=1.3,
        help='Audio tempo multiplier (default: 1.3)'
    )
    
    parser.add_argument(
        '--model-dir',
        type=Path,
        default=default_model_dir,
        help=f'Directory for Kokoro model files (default: {default_model_dir})'
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
        default=script_dir.parent,
        help='OpenRTX project root directory'
    )

    parser.add_argument(
        '--voice-prompts-header',
        type=Path,
        help='Path to voicePrompts.h (default: <project-root>/openrtx/include/core/voicePrompts.h)'
    )

    parser.add_argument(
        '--ui-strings-header',
        type=Path,
        help='Path to ui_strings source file (default: <project-root>/openrtx/src/ui/default/ui_strings.c)'
    )

    parser.add_argument(
        '--check',
        action='store_true',
        help='Check mode: generate to a temp file and compare against the '
             'committed VPC (exit 1 if out of date)'
    )

    parser.add_argument(
        '--check-threshold',
        type=float,
        default=0.05,
        metavar='FRAC',
        help='Max fraction of differing audio bits allowed in check mode '
             '(default: 0.05 = 5%%)'
    )

    parser.add_argument(
        '--check-frame-slack',
        type=int,
        default=2,
        metavar='FRAMES',
        help='Max per-prompt length difference in codec2 frames allowed '
             'in check mode (default: 2)'
    )

    args = parser.parse_args()

    _check_dependencies()

    overrides_file = None if args.no_overrides else args.overrides
    
    # Create generator
    generator = VoicePromptGenerator(
        project_root=args.project_root,
        overrides_file=overrides_file,
        voice_prompts_header=args.voice_prompts_header,
        ui_strings_header=args.ui_strings_header
    )
    
    # Generate VPC file
    if args.check:
        # Check mode: generate to a temp file and compare
        reference = args.output
        if not reference.exists():
            print(f"Error: reference VPC not found: {reference}",
                  file=sys.stderr)
            sys.exit(1)

        check_output = reference.with_suffix('.vpc.check')
        try:
            generator.generate_all(
                output_vpc=check_output,
                temp_dir=args.temp_dir,
                voice_name=args.voice,
                model_dir=args.model_dir,
                gain_db=args.gain,
                tempo=args.tempo,
                keep_temp=args.keep_temp
            )
            ok = generator.check_vpc(
                reference, check_output, args.check_threshold,
                args.check_frame_slack
            )
        except Exception as e:
            print(f"\nError: {e}", file=sys.stderr)
            sys.exit(1)
        finally:
            check_output.unlink(missing_ok=True)

        sys.exit(0 if ok else 1)
    else:
        try:
            generator.generate_all(
                output_vpc=args.output,
                temp_dir=args.temp_dir,
                voice_name=args.voice,
                model_dir=args.model_dir,
                gain_db=args.gain,
                tempo=args.tempo,
                keep_temp=args.keep_temp
            )
        except Exception as e:
            print(f"\nError: {e}", file=sys.stderr)
            sys.exit(1)


if __name__ == '__main__':
    main()
