#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Checks that every per-language UI string table in openrtx/include/ui/ stays
in sync -- with each other, and with the actual stringsTable_t struct.

Two checks, in order:

1. Every *Strings.h file (English included) is checked against
   stringsTable_t's own field list in ui_strings.h -- the actual ground
   truth. EnglishStrings.h is what Weblate translators work from and what
   GetEnglishStringTableOffset() searches, but it's still just one file that
   can itself drift from the struct (e.g. a field added to the struct with
   no *Strings.h file updated to match, or a stale field left in a
   translation after the struct was trimmed). Comparing translations only
   against English can't catch English itself being out of sync.

2. Every other *Strings.h file is checked against EnglishStrings.h, same as
   before -- catches translations that lag behind a source-language change,
   even when both still happen to match the struct's field set.

A MISSING field is a real bug, not just a style issue: these are C designated
initializers (`.field = "text"`), so a field a translation doesn't set is
implicitly zero-initialized to a NULL const char* -- every consumer in
voicePromptUtils.c/ui_menu.c/etc. dereferences these pointers directly, so a
missing field is a null-pointer-dereference risk the first time that string
is displayed or spoken, not merely a blank string.

An EXTRA field (present in a file but not in the thing it's compared
against) is harmless at runtime but usually means a stale/renamed field.

A different ORDER of the same fields is flagged as informational only, not
a failure: designated initializers are order-independent in C, and the
actual runtime indexing (see voicePrompts.c, which computes offsets via
pointer arithmetic on the struct itself) depends only on stringsTable_t's
field declaration order in ui_strings.h -- a single shared definition, not
on how any individual *Strings.h file orders its own initializer list.
Consistent order just makes translations easier to diff against English.

Exit status is non-zero only if any file is missing a field that the thing
it's compared against has, or has an extra field the other one doesn't.
"""

import re
import sys
from pathlib import Path

UI_DIR = Path(__file__).resolve().parent.parent / "openrtx" / "include" / "ui"
SOURCE_FILE = "EnglishStrings.h"
STRUCT_FILE = "ui_strings.h"
FIELD_RE = re.compile(r"^\s*\.(\w+)\s*=")
STRUCT_FIELD_RE = re.compile(r"^\s*const\s+char\*\s*(\w+)\s*;")


def extract_fields(path: Path) -> list[str]:
    fields = []
    with path.open(encoding="utf-8") as f:
        for line in f:
            m = FIELD_RE.match(line)
            if m:
                fields.append(m.group(1))
    return fields


def extract_struct_fields(path: Path) -> list[str]:
    """Parse stringsTable_t's own field declaration order out of ui_strings.h."""
    fields = []
    in_struct = False
    with path.open(encoding="utf-8") as f:
        for line in f:
            stripped = line.strip()
            if not in_struct:
                if stripped == "typedef struct":
                    in_struct = True
                continue
            if stripped == "}":
                break
            m = STRUCT_FIELD_RE.match(line)
            if m:
                fields.append(m.group(1))
    return fields


def main() -> int:
    struct_path = UI_DIR / STRUCT_FILE
    if not struct_path.is_file():
        print(f"error: struct definition not found: {struct_path}", file=sys.stderr)
        return 2

    struct_fields = extract_struct_fields(struct_path)
    if not struct_fields:
        print(f"error: no fields parsed from stringsTable_t in {struct_path}", file=sys.stderr)
        return 2

    source_path = UI_DIR / SOURCE_FILE
    if not source_path.is_file():
        print(f"error: source string table not found: {source_path}", file=sys.stderr)
        return 2

    source_fields = extract_fields(source_path)
    if not source_fields:
        print(f"error: no fields parsed from {source_path}", file=sys.stderr)
        return 2

    other_files = sorted(
        p for p in UI_DIR.glob("*Strings.h") if p.name != SOURCE_FILE
    )
    if not other_files:
        print(f"error: no translation string tables found alongside {SOURCE_FILE}", file=sys.stderr)
        return 2

    failed = False

    # 1. Every *Strings.h file (English included) against the actual struct --
    # the check English-vs-others alone can't do, since it never questions
    # whether English itself still matches stringsTable_t.
    for path in sorted(UI_DIR.glob("*Strings.h")):
        fields = extract_fields(path)

        missing = [f for f in struct_fields if f not in fields]
        extra = [f for f in fields if f not in struct_fields]

        if missing:
            failed = True
            print(f"{path.name}: MISSING {len(missing)} field(s) present in stringsTable_t ({STRUCT_FILE}):")
            print(f"  (these will be NULL const char* at runtime -- crash risk when displayed/spoken):")
            for f in missing:
                print(f"  - {f}")

        if extra:
            failed = True
            print(f"{path.name}: has {len(extra)} field(s) not present in stringsTable_t ({STRUCT_FILE}) (stale/renamed):")
            for f in extra:
                print(f"  - {f}")

        if not missing and not extra:
            print(f"{path.name}: OK, matches stringsTable_t ({len(fields)} fields)")

    print()

    # 2. Every translation against EnglishStrings.h -- catches a translation
    # lagging a source-language change even when both still match the struct.
    for path in other_files:
        fields = extract_fields(path)

        missing = [f for f in source_fields if f not in fields]
        extra = [f for f in fields if f not in source_fields]

        if missing:
            failed = True
            print(f"{path.name}: MISSING {len(missing)} field(s) present in {SOURCE_FILE}")
            print(f"  (these will be NULL const char* at runtime -- crash risk when displayed/spoken):")
            for f in missing:
                print(f"  - {f}")

        if extra:
            failed = True
            print(f"{path.name}: has {len(extra)} field(s) not present in {SOURCE_FILE} (stale/removed):")
            for f in extra:
                print(f"  - {f}")

        if not missing and not extra and fields != source_fields:
            print(f"{path.name}: note -- same fields as {SOURCE_FILE}, different order (informational, not a failure)")

        if not missing and not extra and fields == source_fields:
            print(f"{path.name}: OK ({len(fields)} fields, in sync)")

    if failed:
        print()
        print("String tables are out of sync. Every *Strings.h file must define at least")
        print(f"the same fields as {SOURCE_FILE} (extra/stale fields should be cleaned up too).")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
