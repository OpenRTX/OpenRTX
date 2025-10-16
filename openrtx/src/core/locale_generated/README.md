# Pre-generated Locale Files

This directory contains pre-generated C source files for each supported locale. These files are generated from:
- The struct definition in `openrtx/include/ui/ui_strings.h`
- Translation files (PO files) in `openrtx/src/locale/`

## Why Pre-generated?

These files are checked into source control so that developers can build OpenRTX without needing to set up a Python environment with `polib`. The build system simply includes the appropriate pre-generated file based on the `-Dlocale=<language>` meson option.

## Supported Locales

- `cs` - Czech
- `de` - German
- `en_US` - English (United States)
- `es` - Spanish
- `fr` - French
- `it_IT` - Italian
- `ja` - Japanese
- `ko` - Korean
- `mk` - Macedonian
- `nl` - Dutch
- `pl_PL` - Polish
- `pt_BR` - Portuguese (Brazil)
- `sv` - Swedish
- `th` - Thai
- `uk_UA` - Ukrainian
- `zh_Hans` - Chinese (Simplified)

## Regenerating Files

### For Translators

After updating PO files in `openrtx/src/locale/`, regenerate the C files:

```bash
# Set up Python environment (one time)
python3 -m venv .venv
source .venv/bin/activate  # or .venv\Scripts\activate on Windows
pip install -r requirements.txt

# Regenerate all locale files
meson compile -C build_linux regenerate-locales
```

### For CI/CD

To verify the generated files are up-to-date:

```bash
# Regenerate files
meson compile -C build_linux regenerate-locales

# Check for differences
git diff --exit-code openrtx/src/core/locale_generated/
```

If there are differences, the CI should fail and ask the contributor to regenerate the files.

## How It Works

The `scripts/gen_locale.py` script:
1. Parses the `stringsTable_t` struct from `ui_strings.h`
2. Reads translations from the PO file (or uses English fallbacks for en_US)
3. Generates a C file with fully initialized `uiStrings` and `englishMsgids` structs

Each generated file is ~10KB and contains all 83+ translated strings for that locale.
