#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Tests for generate_voice_prompts.py: header parsing, prompt numbering, the
manifest check and the VPC layout. They use small fixture headers and need
only the Python standard library, so they run in CI next to --check.

Run with: python3 scripts/test_generate_voice_prompts.py
"""

from __future__ import annotations

import contextlib
import io
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import generate_voice_prompts as gvp  # noqa: E402

VOICE_PROMPTS_H = """\
enum voicePrompt
{
    PROMPT_SILENCE,        // Silence
    PROMPT_0,              // 0
    PROMPT_A,              // A
    PROMPT_LONG,           // A long description that wraps onto
                           // the next line.
    PROMPT_SHORT,          // Short
    NUM_VOICE_PROMPTS,
};
"""

UI_STRINGS_H = """\
typedef struct
{
    const char* languageName;
    const char* first;
    const char* second;
}
stringsTable_t;
"""

ENGLISH_STRINGS_H = """\
const stringsTable_t englishStrings =
{
    .languageName = "English",
    .first        = "First",
    .second       = "Second",
};
"""

VOICE_PROMPTS_C = "#define VOICE_PROMPTS_TOC_SIZE 16\n"

SETTINGS = gvp.Settings()


class FixtureProject:
    """A minimal OpenRTX source tree with the files the generator reads."""

    def __init__(self, root: Path, overrides: dict | None = None,
                 voice_prompts_h: str = VOICE_PROMPTS_H,
                 english_strings_h: str = ENGLISH_STRINGS_H,
                 voice_prompts_c: str = VOICE_PROMPTS_C):
        self.root = root
        files = {
            "openrtx/include/core/voicePrompts.h": voice_prompts_h,
            "openrtx/include/ui/ui_strings.h": UI_STRINGS_H,
            "openrtx/include/ui/EnglishStrings.h": english_strings_h,
            "openrtx/src/core/voicePrompts.c": voice_prompts_c,
            "overrides.json": json.dumps(overrides or {}),
        }
        for name, content in files.items():
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding='utf-8')

        self.vpc = root / "voiceprompts.vpc"
        self.vpc.write_bytes(b"vpc data")

    def generator(self) -> gvp.VoicePromptGenerator:
        return gvp.VoicePromptGenerator(self.root,
                                        self.root / "overrides.json")

    def write_manifest(self):
        generator = self.generator()
        gvp.manifest_path(self.vpc).write_text(
            generator.build_manifest(generator.resolve_prompts(), SETTINGS,
                                     self.vpc),
            encoding='utf-8')


class GeneratorTest(unittest.TestCase):

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()

    def project(self, **kwargs) -> FixtureProject:
        return FixtureProject(self.root, **kwargs)


class ParsingTest(GeneratorTest):

    def test_prompt_order_and_text(self):
        prompts = self.project().generator().resolve_prompts()
        self.assertEqual(prompts, [
            ("PROMPT_SILENCE", ""),
            ("PROMPT_0", "0"),
            ("PROMPT_A", "A"),
            ("PROMPT_LONG",
             "A long description that wraps onto the next line."),
            ("PROMPT_SHORT", "Short"),
            ("STRING_languageName", "English"),
            ("STRING_first", "First"),
            ("STRING_second", "Second"),
            ("VOICE_NAME", "English"),
        ])

    def test_strings_follow_struct_order(self):
        reordered = ("const stringsTable_t englishStrings =\n{\n"
                     '    .second       = "Second",\n'
                     '    .languageName = "English",\n'
                     '    .first        = "First",\n'
                     "};\n")
        prompts = self.project(english_strings_h=reordered) \
            .generator().resolve_prompts()
        self.assertEqual([n for n, _ in prompts if n.startswith("STRING_")],
                         ["STRING_languageName", "STRING_first",
                          "STRING_second"])

    def test_section_comment_is_not_a_continuation(self):
        header = VOICE_PROMPTS_H.replace(
            "    PROMPT_SHORT,          // Short\n",
            "    PROMPT_SHORT,          // Short\n    // Section heading\n")
        prompts = dict(self.project(voice_prompts_h=header)
                       .generator().parse_prompt_enum())
        self.assertEqual(prompts["PROMPT_SHORT"], "Short")

    def test_overrides_apply(self):
        prompts = dict(self.project(overrides={
            "_comment": "ignored",
            "PROMPT_A": "/ˈeɪ/",
            "STRING_first": "Number one",
            "PROMPT_VOICE_NAME": "English, test",
        }).generator().resolve_prompts())
        self.assertEqual(prompts["PROMPT_A"], "/ˈeɪ/")
        self.assertEqual(prompts["STRING_first"], "Number one")
        self.assertEqual(prompts["VOICE_NAME"], "English, test")

    def assertRejected(self, message: str, **kwargs):
        with self.assertRaisesRegex(ValueError, message):
            self.project(**kwargs).generator().resolve_prompts()

    def test_rejects_unknown_override(self):
        self.assertRejected("unknown prompts",
                            overrides={"STRING_frist": "First"})

    def test_rejects_prompt_without_text(self):
        self.assertRejected(
            "nothing to speak",
            voice_prompts_h=VOICE_PROMPTS_H.replace(
                "PROMPT_SHORT,          // Short", "PROMPT_SHORT,"))

    def test_rejects_explicit_enum_value(self):
        self.assertRejected(
            "explicit value",
            voice_prompts_h=VOICE_PROMPTS_H.replace("PROMPT_A,",
                                                    "PROMPT_A = 7,"))

    def test_rejects_unterminated_enum(self):
        self.assertRejected(
            "NUM_VOICE_PROMPTS not found",
            voice_prompts_h=VOICE_PROMPTS_H.replace(
                "    NUM_VOICE_PROMPTS,\n", ""))

    def test_rejects_preprocessor_line_in_enum(self):
        self.assertRejected(
            "Unsupported line",
            voice_prompts_h=VOICE_PROMPTS_H.replace(
                "    PROMPT_SHORT,", "#ifdef X\n    PROMPT_SHORT,"))

    def test_rejects_unsupported_string_literals(self):
        for value in ('"Fir\\"st"', '"Fir" "st"', 'NULL'):
            with self.subTest(value=value):
                self.assertRejected(
                    "Unsupported initializer",
                    english_strings_h=ENGLISH_STRINGS_H.replace(
                        '"First"', value))

    def test_rejects_missing_english_string(self):
        self.assertRejected(
            "does not match stringsTable_t",
            english_strings_h=ENGLISH_STRINGS_H.replace(
                '    .second       = "Second",\n', ''))

    def test_rejects_toc_overflow(self):
        self.assertRejected(
            "do not fit",
            voice_prompts_c="#define VOICE_PROMPTS_TOC_SIZE 9\n")


class ManifestTest(GeneratorTest):

    def check(self, project: FixtureProject) -> bool:
        with contextlib.redirect_stdout(io.StringIO()):
            return project.generator().check_manifest(project.vpc,
                                                      SETTINGS)

    def test_up_to_date(self):
        project = self.project()
        project.write_manifest()
        self.assertTrue(self.check(project))

    def test_changed_string_fails(self):
        project = self.project()
        project.write_manifest()
        path = project.root / "openrtx/include/ui/EnglishStrings.h"
        path.write_text(ENGLISH_STRINGS_H.replace("First", "Primary"))
        self.assertFalse(self.check(project))

    def test_hand_edited_manifest_fails(self):
        project = self.project()
        project.write_manifest()
        manifest = gvp.manifest_path(project.vpc)
        manifest.write_text(manifest.read_text().replace('"First"',
                                                         '"Primary"'))
        path = project.root / "openrtx/include/ui/EnglishStrings.h"
        path.write_text(ENGLISH_STRINGS_H.replace("First", "Primary"))
        self.assertFalse(self.check(project))

    def test_changed_vpc_fails(self):
        project = self.project()
        project.write_manifest()
        project.vpc.write_bytes(b"other data")
        self.assertFalse(self.check(project))

    def test_missing_manifest_fails(self):
        self.assertFalse(self.check(self.project()))


class VpcTest(GeneratorTest):

    def c2_file(self, name: str, frames: bytes) -> Path:
        path = self.root / f"{name}.c2"
        path.write_bytes(gvp.CODEC2_HEADER_MAGIC + b"\x01\x02\x00\x00"
                         + frames)
        return path

    def test_layout(self):
        generator = self.project().generator()
        files = [self.c2_file("a", b"A" * 8), self.c2_file("b", b"B" * 16)]
        vpc = self.root / "out.vpc"
        with contextlib.redirect_stdout(io.StringIO()):
            generator.build_vpc(files, vpc)

        data = vpc.read_bytes()
        magic, version = struct.unpack('<II', data[:8])
        toc = struct.unpack('<16I', data[8:8 + 16 * 4])
        self.assertEqual((magic, version), (gvp.VPC_MAGIC, gvp.VPC_VERSION))
        self.assertEqual(toc[:3], (0, 15, 38))
        self.assertEqual(set(toc[2:]), {38})
        self.assertEqual(len(data), 8 + 16 * 4 + 38)

    def test_rejects_file_without_codec2_header(self):
        path = self.root / "raw.c2"
        path.write_bytes(b"\x00" * 8)
        with self.assertRaisesRegex(ValueError, "no codec2 header"):
            self.project().generator().build_vpc([path],
                                                 self.root / "out.vpc")

    def test_rejects_toc_overflow(self):
        files = [self.c2_file(str(i), b"") for i in range(16)]
        with self.assertRaisesRegex(ValueError, "do not fit"):
            self.project().generator().build_vpc(files,
                                                 self.root / "out.vpc")


class WorkDirTest(GeneratorTest):

    def test_only_private_directory_is_removed(self):
        sentinel = self.root / "keep-me"
        sentinel.write_text("user data")
        with gvp.work_dir(self.root, keep=False) as tmp:
            self.assertEqual(tmp.parent, self.root)
            (tmp / "file").write_text("intermediate")
        self.assertFalse(tmp.exists())
        self.assertTrue(sentinel.exists())


if __name__ == '__main__':
    unittest.main()
