#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Generate OpenRTX voice prompts from C source headers.

This script:
1. Parses voicePrompts.h for the PROMPT_* enum, ui_strings.h for the order
   of the stringsTable_t fields and EnglishStrings.h for their text
2. Generates TTS audio using Kokoro
3. Processes audio files (ffmpeg resample to 8kHz, c2enc to codec2)
4. Packages everything into a VPC binary file
5. Writes a manifest next to it, listing every prompt's text and the
   generation settings

TTS output is not bit-identical across machines, so the committed VPC is
checked through its manifest instead of by regenerating it: --check parses
the headers, rebuilds the manifest and compares it with the committed one.
That needs no TTS dependencies and gives the same answer everywhere.
"""

from __future__ import annotations

import argparse
import contextlib
import dataclasses
import difflib
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path
from typing import TYPE_CHECKING, Iterator

from check_strings import extract_struct_fields

# Imported where needed, so --check runs without the TTS dependencies.
if TYPE_CHECKING:
    from kokoro_onnx import Kokoro

# Bump when a change to the audio pipeline (resampling, codec2 mode,
# silence, language...) changes the generated audio without changing any
# prompt text or setting recorded in the manifest.
PIPELINE_VERSION = 1

# Kokoro model release and the SHA256 of each file in it, in the order
# Kokoro() takes them.
KOKORO_MODEL_URL = ("https://github.com/thewh1teagle/kokoro-onnx/releases/"
                    "download/model-files-v1.0/")
KOKORO_MODEL_FILES = (
    ("kokoro-v1.0.onnx",
     "7d5df8ecf7d4b1878015a32686053fd0eebe2bc377234608764cc0ef3636a6c5"),
    ("voices-v1.0.bin",
     "bca610b8308e8d99f32e6fe4197e7ec01679264efed0cac9140fe9c29f1fbf7d"),
)
KOKORO_LANG = "en-gb"
KOKORO_SAMPLE_RATE = 24000

# Style-vector blend: the two British voices carry the accent, the American
# third steadies the vowel glides that otherwise make isolated letters
# (E, R, N, P) hard to tell apart after codec2.
DEFAULT_VOICE = "bm_lewis:0.34+bm_fable:0.33+am_michael:0.33"
DEFAULT_TEMPO = 1.3
DEFAULT_GAIN_DB = 0.0

# Header that c2enc writes at the start of a ".c2" output file. Every
# prompt in the VPC keeps it, and the firmware skips it before decoding
# (CODEC2_HEADER_SIZE in voicePrompts.c).
CODEC2_HEADER_MAGIC = b"\xc0\xde\xc2"
CODEC2_MODE = 3200

# VPC file header, checked by the firmware (voicePrompts.c).
VPC_MAGIC = 0x5056    # 'VP'
VPC_VERSION = 0x1000  # v1.0

# How to fix a failed --check, printed at the end of the failure.
CHECK_FAIL_HELP = """
Regenerate the voice prompts instead of editing either file by hand:

  sudo apt install ffmpeg codec2       # or: brew install ffmpeg codec2
  uv venv && uv pip install -r scripts/requirements-voiceprompts.txt
  .venv/bin/python scripts/generate_voice_prompts.py

then commit voiceprompts.vpc and voiceprompts.vpc.manifest together. The
first run downloads the TTS model (about 350 MB). See "Voice Prompts" in
CONTRIBUTING.md; if you cannot run the generator, ask a maintainer to
regenerate the files for you."""


@dataclasses.dataclass(frozen=True)
class Settings:
    """Generation settings recorded in the manifest."""

    voice: str = DEFAULT_VOICE
    tempo: float = DEFAULT_TEMPO
    gain_db: float = DEFAULT_GAIN_DB


def manifest_path(vpc: Path) -> Path:
    """Path of the manifest describing a VPC file."""
    return vpc.with_name(vpc.name + ".manifest")


def sha256_file(path: Path) -> str:
    """SHA256 of a file, read in chunks."""
    digest = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


@contextlib.contextmanager
def work_dir(parent: Path | None, keep: bool) -> Iterator[Path]:
    """
    Create a private directory for intermediate files, inside parent if
    given, and remove it afterwards unless keep is set. Only the directory
    created here is ever removed, never parent itself.
    """
    if parent is not None:
        parent.mkdir(parents=True, exist_ok=True)

    path = Path(tempfile.mkdtemp(prefix="openrtx_vpc_", dir=parent))
    try:
        yield path
    finally:
        if keep:
            print(f"Temporary files kept in: {path}")
        else:
            shutil.rmtree(path, ignore_errors=True)


class VoicePromptGenerator:
    """Generate voice prompts for OpenRTX from source headers."""

    def __init__(self, project_root: Path,
                 overrides_file: Path | None = None,
                 voice_prompts_header: Path | None = None,
                 ui_strings_header: Path | None = None,
                 english_strings_header: Path | None = None):
        include_dir = project_root / "openrtx" / "include"
        self.project_root = project_root
        self.voice_prompts_header = (
            voice_prompts_header or include_dir / "core" / "voicePrompts.h")
        self.ui_strings_header = (
            ui_strings_header or include_dir / "ui" / "ui_strings.h")
        self.english_strings_header = (
            english_strings_header
            or include_dir / "ui" / "EnglishStrings.h")
        self.toc_size = self._parse_toc_size(
            project_root / "openrtx" / "src" / "core" / "voicePrompts.c")
        self.overrides = (self._load_overrides(overrides_file)
                          if overrides_file else {})

        self._kokoro = None
        self._soundfile = None

    @staticmethod
    def _parse_toc_size(voice_prompts_source: Path) -> int:
        """
        Read the ToC size from VOICE_PROMPTS_TOC_SIZE in voicePrompts.c, so
        the generated file layout can never drift from what the firmware
        expects.
        """
        content = voice_prompts_source.read_text(encoding='utf-8')
        match = re.search(r'#define\s+VOICE_PROMPTS_TOC_SIZE\s+(\d+)', content)
        if not match:
            raise ValueError(
                "Could not find VOICE_PROMPTS_TOC_SIZE in "
                f"{voice_prompts_source}")

        return int(match.group(1))

    @staticmethod
    def _load_overrides(overrides_file: Path) -> dict[str, str]:
        """
        Load pronunciation overrides from JSON file. Keys starting with an
        underscore are documentation ("_comment", ...) and are dropped.
        """
        with open(overrides_file, 'r', encoding='utf-8') as f:
            overrides = json.load(f)

        return {key: value for key, value in overrides.items()
                if not key.startswith('_')}

    def parse_prompt_enum(self) -> list[tuple[str, str]]:
        """
        Parse the voicePrompt enum in voicePrompts.h.

        Each entry's trailing comment is the text spoken for it; a comment-
        only line right below an entry, with its // in the same column as
        the entry's, continues that entry's comment. The
        parser only accepts what it can number the way the compiler does
        (one PROMPT_* entry per line, no explicit values, ended by
        NUM_VOICE_PROMPTS) and fails on anything else, so a header change
        cannot silently shift the prompt numbers.

        Returns:
            List of (enum_name, comment_text) tuples in enum order
        """
        header_file = self.voice_prompts_header
        content = header_file.read_text(encoding='utf-8')

        match = re.search(r'enum\s+voicePrompt\s*\{(.*?)\};', content,
                          re.DOTALL)
        if not match:
            raise ValueError(f"Could not find voicePrompt enum in "
                             f"{header_file}")

        entry_re = re.compile(r'(\w+)\s*(=[^,/]*)?,?\s*(?://\s*(.*))?$')
        entries = []
        comment_column = None  # column of the previous entry's comment

        for raw_line in match.group(1).split('\n'):
            line = raw_line.strip()
            if not line:
                comment_column = None
                continue

            if line.startswith('//'):
                if raw_line.index('//') == comment_column:
                    name, comment = entries[-1]
                    extra = line[2:].strip()
                    entries[-1] = (name, f"{comment} {extra}")
                else:
                    comment_column = None
                continue

            entry = entry_re.match(line)
            if not entry:
                raise ValueError(f"Unsupported line in the voicePrompt enum "
                                 f"of {header_file}: {line}")

            name, value, comment = entry.groups()
            if name == "NUM_VOICE_PROMPTS":
                return entries

            if value is not None:
                raise ValueError(f"{name} in {header_file} has an explicit "
                                 "value; prompts are numbered in order")
            if not name.startswith("PROMPT_"):
                raise ValueError(f"Unexpected voicePrompt entry {name} in "
                                 f"{header_file}")

            entries.append((name, (comment or "").strip()))
            comment_column = (raw_line.index('//') if comment is not None
                              else None)

        raise ValueError(f"NUM_VOICE_PROMPTS not found in the voicePrompt "
                         f"enum of {header_file}")

    def parse_english_strings(self) -> dict[str, str]:
        """
        Parse the englishStrings initializer in EnglishStrings.h.

        Only single, plain string literals are supported; escapes and
        literal concatenation are rejected rather than mis-read.

        Returns:
            Dictionary of field name to text
        """
        header_file = self.english_strings_header
        content = header_file.read_text(encoding='utf-8')

        match = re.search(r'(?:static\s+)?const\s+stringsTable_t\s+'
                          r'englishStrings\s*=\s*\{(.*?)\};',
                          content, re.DOTALL)
        if not match:
            raise ValueError(f"Could not find englishStrings initializer in "
                             f"{header_file}")

        body = match.group(1)
        strings = {}
        string_re = re.compile(r'\.(\w+)\s*=\s*"([^"\\]*)"\s*([,}]|$)')

        for field in re.finditer(r'\.(\w+)\s*=', body):
            literal = string_re.match(body, field.start())
            if not literal:
                raise ValueError(f"Unsupported initializer for "
                                 f".{field.group(1)} in {header_file}: use "
                                 "a single string literal without escapes")
            strings[literal.group(1)] = literal.group(2)

        return strings

    def get_prompt_text(self, enum_name: str, comment: str) -> str:
        """
        Text spoken for a PROMPT_* enum entry: an override if there is one,
        nothing for PROMPT_SILENCE, the digit or letter itself, and
        otherwise the entry's comment in voicePrompts.h.
        """
        if enum_name in self.overrides:
            return self.overrides[enum_name]

        suffix = enum_name[len("PROMPT_"):]
        if enum_name == "PROMPT_SILENCE":
            return ""
        if re.fullmatch(r'\d|[A-Z]', suffix):
            return suffix
        if comment:
            return comment

        raise ValueError(f"{enum_name} has nothing to speak: add a comment "
                         "to it in voicePrompts.h or an override")

    def resolve_prompts(self) -> list[tuple[str, str]]:
        """
        List every prompt in VPC order with the text that is spoken for it:
        the PROMPT_* enum, then the string table in stringsTable_t
        declaration order (the order the firmware numbers them in), then
        the voice name.

        Returns:
            List of (name, text) tuples
        """
        fields = extract_struct_fields(self.ui_strings_header)
        if not fields:
            raise ValueError(f"No stringsTable_t fields found in "
                             f"{self.ui_strings_header}")

        english = self.parse_english_strings()
        missing = [f for f in fields if f not in english]
        extra = [f for f in english if f not in fields]
        if missing or extra:
            raise ValueError(f"{self.english_strings_header} does not match "
                             f"stringsTable_t: missing {missing}, "
                             f"unknown {extra}")

        prompts = [(name, self.get_prompt_text(name, comment))
                   for name, comment in self.parse_prompt_enum()]
        for field in fields:
            name = f"STRING_{field}"
            prompts.append((name, self.overrides.get(name, english[field])))
        prompts.append(("VOICE_NAME",
                        self.overrides.get("PROMPT_VOICE_NAME", "English")))

        names = {name for name, _ in prompts} | {"PROMPT_VOICE_NAME"}
        unknown = sorted(set(self.overrides) - names)
        if unknown:
            raise ValueError(f"Overrides for unknown prompts: {unknown}")

        if len(prompts) >= self.toc_size:
            raise ValueError(f"{len(prompts)} prompts do not fit in a ToC "
                             f"of {self.toc_size} entries")

        return prompts

    def build_manifest(self, prompts: list[tuple[str, str]],
                       settings: Settings, vpc: Path) -> str:
        """
        Describe a VPC file: the settings it was generated with and the text
        of every prompt in it, one per line, so a change shows up in review
        as a readable diff. The sha256 line covers the VPC and every line
        below it, so neither file can be changed alone, and a hand edit to
        the manifest fails the check instead of passing with stale audio.

        Args:
            prompts: Prompt list, see resolve_prompts()
            settings: Generation settings
            vpc: The VPC file the manifest describes

        Returns:
            Manifest text
        """
        body = [
            f"pipeline: {PIPELINE_VERSION}",
            f"toc-size: {self.toc_size}",
            f"voice: {settings.voice}",
            f"tempo: {settings.tempo}",
            f"gain-db: {settings.gain_db}",
        ]
        body += [f"model: {name} {sha256}"
                 for name, sha256 in KOKORO_MODEL_FILES]
        body.append("")
        body += [f"{i:03d} {name} {json.dumps(text, ensure_ascii=False)}"
                 for i, (name, text) in enumerate(prompts)]
        body = "\n".join(body) + "\n"

        digest = hashlib.sha256(vpc.read_bytes())
        digest.update(body.encode('utf-8'))

        return (f"# Generated by scripts/generate_voice_prompts.py together "
                f"with\n# {vpc.name}. Do not edit: regenerate both files.\n"
                f"sha256: {digest.hexdigest()}\n" + body)

    def check_manifest(self, vpc: Path, settings: Settings) -> bool:
        """
        Check that the committed VPC is up to date: rebuild its manifest
        from the headers and the given settings, and compare it with the
        committed manifest.

        Returns:
            True if the manifest matches
        """
        manifest = manifest_path(vpc)
        for path in (vpc, manifest):
            if not path.exists():
                print(f"FAIL: {path} not found")
                return False

        expected = self.build_manifest(self.resolve_prompts(), settings,
                                       vpc)
        committed = manifest.read_text(encoding='utf-8')
        if committed == expected:
            print(f"OK: {vpc} is up to date")
            return True

        def without_hash(text):
            return [line for line in text.splitlines()
                    if not line.startswith("sha256: ")]

        if without_hash(committed) == without_hash(expected):
            print(f"FAIL: {manifest} does not match {vpc}. One of the two "
                  "was changed\nwithout running the generator; do not edit "
                  "them by hand.")
        else:
            print(f"FAIL: {vpc} is out of date. These prompts or settings "
                  "changed:\n")
            sys.stdout.writelines(difflib.unified_diff(
                committed.splitlines(keepends=True),
                expected.splitlines(keepends=True),
                f"{manifest} (committed)", f"{manifest} (expected)"))

        print(CHECK_FAIL_HELP)
        return False

    def build_vpc(self, c2_files: list[Path], output_vpc: Path):
        """
        Package codec2 files into VPC binary format:
        - Header (8 bytes): magic (4), version (4)
        - ToC (VOICE_PROMPTS_TOC_SIZE * 4 bytes): uint32 offset of each
          prompt in the audio data
        - Audio data: each prompt's codec2 file as written by c2enc,
          starting with its 7-byte codec2 header, which the firmware skips

        Args:
            c2_files: List of codec2 file paths in prompt order
            output_vpc: Output VPC file path
        """
        # The firmware computes a prompt's length as toc[n + 1] - toc[n],
        # so the entry after the last prompt must exist and hold the total.
        if len(c2_files) >= self.toc_size:
            raise ValueError(f"{len(c2_files)} prompts do not fit in a ToC "
                             f"of {self.toc_size} entries")

        toc = []
        audio_data = bytearray()
        for c2_file in c2_files:
            c2_data = c2_file.read_bytes()
            if not c2_data.startswith(CODEC2_HEADER_MAGIC):
                raise ValueError(f"{c2_file} has no codec2 header")

            toc.append(len(audio_data))
            audio_data.extend(c2_data)

        toc += [len(audio_data)] * (self.toc_size - len(toc))

        with open(output_vpc, 'wb') as f:
            f.write(struct.pack('<II', VPC_MAGIC, VPC_VERSION))
            f.write(struct.pack(f'<{self.toc_size}I', *toc))
            f.write(audio_data)

        print(f"Built VPC file: {output_vpc} "
              f"({len(audio_data)} bytes of audio data)")

    @staticmethod
    def _resolve_kokoro_files(model_dir: Path) -> tuple[Path, ...]:
        """
        Resolve Kokoro model files (.onnx and .bin), downloading them if
        missing. Every file is checked against its pinned SHA256, so a
        corrupted cache or a changed upstream release fails loudly instead
        of silently producing different prompts. A download goes to a
        temporary file first, so an interrupted one leaves nothing behind.

        Args:
            model_dir: Directory to cache Kokoro model files

        Returns:
            Tuple of (onnx_path, voices_bin_path)
        """
        paths = []
        for name, sha256 in KOKORO_MODEL_FILES:
            path = model_dir / name
            if not path.exists():
                print(f"Downloading {name} to {model_dir}...")
                model_dir.mkdir(parents=True, exist_ok=True)
                fd, partial = tempfile.mkstemp(dir=model_dir,
                                               prefix=f".{name}.")
                try:
                    with os.fdopen(fd, 'wb') as out, \
                         urllib.request.urlopen(KOKORO_MODEL_URL + name) \
                         as response:
                        shutil.copyfileobj(response, out)
                    if sha256_file(Path(partial)) != sha256:
                        raise ValueError(f"Downloaded {name} does not match "
                                         "its pinned SHA256")
                    os.replace(partial, path)
                finally:
                    Path(partial).unlink(missing_ok=True)

            actual = sha256_file(path)
            if actual != sha256:
                raise ValueError(
                    f"Checksum mismatch for {path}: expected {sha256}, "
                    f"got {actual}. Delete the file to download it again.")

            paths.append(path)

        return tuple(paths)

    def _load_tts(self, model_dir: Path):
        """Load Kokoro and the audio dependencies used to run it."""
        from kokoro_onnx import Kokoro
        import onnxruntime as ort
        import soundfile

        onnx_path, voices_path = self._resolve_kokoro_files(model_dir)
        print(f"Loading Kokoro model: {onnx_path}")

        # The order of floating-point reductions depends on the thread
        # count and the graph optimizations, so pin both: the output is then
        # repeatable on a given machine. It still differs slightly between
        # CPU types, which is why --check compares manifests, not audio.
        options = ort.SessionOptions()
        options.intra_op_num_threads = 1
        options.inter_op_num_threads = 1
        options.graph_optimization_level = (
            ort.GraphOptimizationLevel.ORT_ENABLE_ALL)
        session = ort.InferenceSession(str(onnx_path), options,
                                       providers=["CPUExecutionProvider"])

        self._kokoro = Kokoro.from_session(session, str(voices_path))
        self._soundfile = soundfile

    @staticmethod
    def resolve_voice(kokoro: Kokoro, voice: str):
        """
        Turn a voice spec into a Kokoro style vector.

        A plain name ('bm_lewis') is returned as-is. A blend
        ('bm_lewis:0.5+bm_fable:0.5') is the weighted sum of the named
        style vectors.
        """
        if '+' not in voice and ':' not in voice:
            return voice

        blend = None
        for part in voice.split('+'):
            name, _, weight = part.partition(':')
            try:
                weight = float(weight) if weight else 1.0
            except ValueError:
                raise ValueError(
                    f"Invalid weight in voice blend: {part}") from None

            style = kokoro.get_voice_style(name) * weight
            blend = style if blend is None else blend + style

        return blend

    def generate_tts(self, text: str, output_wav: Path, settings: Settings):
        """
        Generate TTS audio using the Kokoro Python API.

        Args:
            text: Text to speak, or an IPA phoneme string enclosed in
                  slashes ('/ˈiːː/') to bypass the grapheme-to-phoneme step;
                  empty for 0.1 s of silence
            output_wav: Output WAV file path
            settings: Generation settings
        """
        if not text:
            samples = [0.0] * (KOKORO_SAMPLE_RATE // 10)
            self._soundfile.write(str(output_wav), samples,
                                  KOKORO_SAMPLE_RATE)
            return

        is_phonemes = len(text) > 2 and text[0] == '/' and text[-1] == '/'
        if is_phonemes:
            text = text[1:-1]

        samples, sample_rate = self._kokoro.create(
            text,
            voice=self.resolve_voice(self._kokoro, settings.voice),
            speed=settings.tempo,
            lang=KOKORO_LANG,
            is_phonemes=is_phonemes
        )
        self._soundfile.write(str(output_wav), samples, sample_rate)

    @staticmethod
    def _run(command: list[str], what: str):
        """Run an external tool, raising with its output if it fails."""
        result = subprocess.run(command, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        if result.returncode != 0:
            raise RuntimeError(f"{what} failed:\n"
                               f"{result.stderr.decode(errors='replace')}")

    def convert_to_8khz_raw(self, wav_file: Path, raw_file: Path,
                            gain_db: float):
        """Convert WAV to 8kHz 16-bit signed PCM raw using ffmpeg."""
        self._run(['ffmpeg', '-y', '-i', str(wav_file),
                   '-filter:a', f'volume={gain_db}dB',
                   '-ar', '8000', '-f', 's16le', str(raw_file)],
                  f"ffmpeg on {wav_file}")

    def encode_codec2(self, raw_file: Path, c2_file: Path):
        """
        Encode raw audio to codec2 using c2enc. The ".c2" extension of
        c2_file makes c2enc write the codec2 header that the VPC format
        keeps for every prompt.
        """
        if c2_file.suffix != ".c2":
            raise ValueError(f"{c2_file} must end in .c2")

        self._run(['c2enc', str(CODEC2_MODE), str(raw_file), str(c2_file)],
                  f"c2enc on {raw_file}")

    def _synth_prompt(self, text: str, stem: str, temp_dir: Path,
                      settings: Settings) -> Path:
        """
        Run one prompt through the full synth pipeline: TTS to wav, resample
        to 8 kHz raw, encode to codec2.

        Returns:
            Path of the resulting codec2 file
        """
        wav_file = temp_dir / f"{stem}.wav"
        raw_file = temp_dir / f"{stem}.raw"
        c2_file = temp_dir / f"{stem}.c2"

        self.generate_tts(text, wav_file, settings)
        self.convert_to_8khz_raw(wav_file, raw_file, settings.gain_db)
        self.encode_codec2(raw_file, c2_file)

        return c2_file

    def generate_all(self, output_vpc: Path, settings: Settings,
                     model_dir: Path, temp_dir: Path | None = None,
                     keep_temp: bool = False):
        """
        Generate the complete voice prompt VPC file and its manifest.

        Args:
            output_vpc: Output VPC file path
            settings: Generation settings
            model_dir: Directory containing Kokoro model files
            temp_dir: Where to create the directory for intermediate files
            keep_temp: Keep intermediate files for debugging
        """
        print("Parsing C headers...")
        prompts = self.resolve_prompts()
        print(f"Found {len(prompts)} voice prompts")

        self._load_tts(model_dir)

        with work_dir(temp_dir, keep_temp) as tmp:
            print(f"Using temporary directory: {tmp}")

            c2_files = []
            print(f"\nGenerating {len(prompts)} voice prompts...")
            for i, (name, text) in enumerate(prompts):
                print(f"  [{i + 1}/{len(prompts)}] {name}: '{text}'")
                c2_files.append(self._synth_prompt(
                    text, f"{i:03d}_{name}", tmp, settings))

            print("\nBuilding VPC file...")
            self.build_vpc(c2_files, output_vpc)

        manifest = manifest_path(output_vpc)
        manifest.write_text(
            self.build_manifest(prompts, settings, output_vpc),
            encoding='utf-8')
        print(f"Wrote manifest: {manifest}")

        # Touch the assembly file so ninja knows to recompile
        # (voicePromptData.S embeds voiceprompts.vpc via .incbin)
        s_file = (self.project_root / "openrtx" / "src" / "core"
                  / "voicePromptData.S")
        if s_file.exists():
            s_file.touch()

        print(f"\nGenerated {output_vpc}: {len(prompts)} prompts, "
              f"{output_vpc.stat().st_size / 1024:.1f} KB")


def missing_tools() -> list[str]:
    """External tools needed for generation that are not installed."""
    return [f"  {cmd} (install with: {hint})"
            for cmd, hint in (('ffmpeg', 'apt install ffmpeg'),
                              ('c2enc', 'apt install codec2'))
            if shutil.which(cmd) is None]


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    default_overrides = script_dir / 'voice_overrides.json'
    default_model_dir = Path.home() / '.cache' / 'kokoro-tts'

    parser = argparse.ArgumentParser(
        description='Generate OpenRTX voice prompts from C source headers',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate with default settings
  %(prog)s

  # Generate with a single voice, or a weighted blend of voices
  %(prog)s --voice bm_george
  %(prog)s --voice bm_lewis:0.5+bm_fable:0.5

  # Keep temporary files for debugging
  %(prog)s --keep-temp

  # Verify the committed VPC is up to date (CI mode)
  %(prog)s --check
        """
    )
    parser.add_argument(
        '--project-root', type=Path, default=script_dir.parent,
        help='OpenRTX project root directory (default: the one containing '
             'this script)')
    parser.add_argument(
        '-o', '--output', type=Path,
        help='Output VPC file path (default: <project-root>/voiceprompts.vpc)')
    parser.add_argument(
        '--overrides', type=Path, default=default_overrides,
        help='JSON file with pronunciation overrides '
             f'(default: {default_overrides})')
    parser.add_argument(
        '--no-overrides', action='store_true',
        help='Disable pronunciation overrides')
    parser.add_argument(
        '--voice', default=DEFAULT_VOICE,
        help='Kokoro voice name, or blend as name:weight+name:weight '
             f'(default: {DEFAULT_VOICE})')
    parser.add_argument(
        '--gain', type=float, default=DEFAULT_GAIN_DB,
        help=f'Volume adjustment in dB (default: {DEFAULT_GAIN_DB})')
    parser.add_argument(
        '--tempo', type=float, default=DEFAULT_TEMPO,
        help=f'Speech speed multiplier (default: {DEFAULT_TEMPO})')
    parser.add_argument(
        '--model-dir', type=Path, default=default_model_dir,
        help=f'Directory for Kokoro model files (default: {default_model_dir})')
    parser.add_argument(
        '--temp-dir', type=Path,
        help='Directory in which to create the private directory for '
             'intermediate files (default: the system temporary directory)')
    parser.add_argument(
        '--keep-temp', action='store_true',
        help='Keep intermediate files for debugging')
    parser.add_argument(
        '--voice-prompts-header', type=Path,
        help='Path to voicePrompts.h '
             '(default: <project-root>/openrtx/include/core/voicePrompts.h)')
    parser.add_argument(
        '--ui-strings-header', type=Path,
        help='Path to ui_strings.h '
             '(default: <project-root>/openrtx/include/ui/ui_strings.h)')
    parser.add_argument(
        '--english-strings-header', type=Path,
        help='Path to EnglishStrings.h '
             '(default: <project-root>/openrtx/include/ui/EnglishStrings.h)')
    parser.add_argument(
        '--check', action='store_true',
        help='Check mode: compare the manifest of the committed VPC with '
             'the one the current headers and settings give, without '
             'generating any audio (exit 1 if out of date)')
    args = parser.parse_args()

    output = args.output or args.project_root / 'voiceprompts.vpc'
    settings = Settings(args.voice, args.tempo, args.gain)

    try:
        generator = VoicePromptGenerator(
            project_root=args.project_root,
            overrides_file=None if args.no_overrides else args.overrides,
            voice_prompts_header=args.voice_prompts_header,
            ui_strings_header=args.ui_strings_header,
            english_strings_header=args.english_strings_header)

        if args.check:
            return 0 if generator.check_manifest(output, settings) else 1

        missing = missing_tools()
        if missing:
            print("Missing required tools:\n" + "\n".join(missing),
                  file=sys.stderr)
            return 1

        generator.generate_all(output, settings, args.model_dir,
                               args.temp_dir, args.keep_temp)
    except (OSError, RuntimeError, ValueError) as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
