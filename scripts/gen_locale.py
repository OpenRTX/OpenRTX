#!/usr/bin/env python3
"""
Generate localized UI strings for OpenRTX.

Two modes:
1. Generate C source code from ui_strings.h + PO file
2. Generate POT file from ui_strings.h annotations

Usage:
    # Generate C source for Spanish locale
    python3 gen_locale.py --struct ui_strings.h --po es.po --output ui_strings_generated.c
    
    # Generate C source for English (no PO file needed)
    python3 gen_locale.py --struct ui_strings.h --output ui_strings_generated.c
    
    # Generate POT file
    python3 gen_locale.py --struct ui_strings.h --pot OpenRTX.pot
"""

import argparse
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import List, Tuple, Optional

try:
    import polib
except ImportError:
    print(f"ERROR: polib module not found. Install with: pip install polib", file=sys.stderr)
    sys.exit(1)


# Hardcoded abbreviation mappings
ABBREVIATIONS = {
    'GPS': 'GPS',
    'VHF': 'VHF',
    'UHF': 'UHF',
    'DMR': 'DMR',
    'M17': 'M17',
    'RSSI': 'RSSI',
    'FM': 'FM',
    'LCD': 'LCD',
    'UTC': 'UTC',
    'CTCSS': 'CTCSS',
    'CAN': 'CAN',
    'PTT': 'PTT',
}


def camel_to_title(field_name: str) -> str:
    """
    Convert camelCase field name to Title Case with some special handling.
    
    Examples:
        gpsSetTime -> GPS Set Time
        batteryVoltage -> Battery Voltage
        UTCTimeZone -> UTC Timezone (special case)
    """
    # Special cases that can't be derived automatically
    special_cases = {
        'UTCTimeZone': 'UTC Timezone',
    }
    
    if field_name in special_cases:
        return special_cases[field_name]
    
    # Insert spaces before uppercase letters
    result = re.sub(r'([a-z])([A-Z])', r'\1 \2', field_name)
    
    # Handle consecutive uppercase letters (acronyms)
    result = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1 \2', result)
    
    # Capitalize first letter and each word after a space
    words = result.split()
    titled_words = []
    for word in words:
        # Check if it's a known abbreviation
        if word.upper() in ABBREVIATIONS:
            titled_words.append(ABBREVIATIONS[word.upper()])
        else:
            titled_words.append(word.capitalize())
    
    return ' '.join(titled_words)


def extract_struct_fields(header_path: Path) -> List[Tuple[str, Optional[str], List[str]]]:
    """
    Parse ui_strings.h to extract field names and their i18n annotations.
    
    Returns:
        List of (field_name, msgid, translator_comments)
        msgid is None if no quoted annotation was found
        translator_comments is a list of unquoted annotation lines
    """
    with open(header_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Find the typedef struct
    struct_match = re.search(
        r'typedef\s+struct\s*\{(.*?)\}\s*stringsTable_t\s*;',
        content,
        re.DOTALL
    )
    
    if not struct_match:
        print(f"ERROR: Could not find stringsTable_t typedef in {header_path}", file=sys.stderr)
        sys.exit(1)
    
    struct_body = struct_match.group(1)
    lines = struct_body.split('\n')
    
    fields = []
    comment_buffer = []
    
    for line in lines:
        line = line.strip()
        
        # Collect i18n comments
        if line.startswith('// i18n:'):
            comment_text = line[8:].strip()  # Remove '// i18n:' prefix
            comment_buffer.append(comment_text)
            continue
        
        # Check for field definition
        field_match = re.match(r'const\s+char\s*\*\s*(\w+)\s*;', line)
        if field_match:
            field_name = field_match.group(1)
            
            # Extract msgid from comments
            msgid = None
            translator_comments = []
            
            for comment in comment_buffer:
                # Check if it's a quoted msgid
                quoted_match = re.match(r'^"(.*)"$', comment)
                if quoted_match:
                    msgid = quoted_match.group(1)
                else:
                    # It's a translator comment
                    translator_comments.append(comment)
            
            # If no msgid annotation, derive from field name
            if msgid is None:
                msgid = camel_to_title(field_name)
            
            fields.append((field_name, msgid, translator_comments))
            comment_buffer = []
    
    if not fields:
        print(f"ERROR: No fields found in stringsTable_t", file=sys.stderr)
        sys.exit(1)
    
    return fields


def escape_c_string(s: str) -> str:
    """Escape a string for use as a C string literal."""
    # Replace backslash first
    s = s.replace('\\', '\\\\')
    # Replace double quote
    s = s.replace('"', '\\"')
    # Replace newline
    s = s.replace('\n', '\\n')
    # Replace tab
    s = s.replace('\t', '\\t')
    # Replace carriage return
    s = s.replace('\r', '\\r')
    return s


def generate_c_source(fields: List[Tuple[str, str, List[str]]], 
                      po_file: Optional[Path],
                      output_path: Path):
    """
    Generate C source file with uiStrings and englishMsgids structs.
    
    Args:
        fields: List of (field_name, msgid, translator_comments)
        po_file: Path to PO file, or None for English-only
        output_path: Path to output C file
    """
    # Load PO file if provided
    translated_strings = {}
    locale_name = "English"
    
    if po_file:
        try:
            po = polib.pofile(str(po_file))
            locale_name = po.metadata.get('Language', po_file.stem)
            
            # Build translation map
            for entry in po:
                if entry.msgstr:
                    translated_strings[entry.msgid] = entry.msgstr
        except Exception as e:
            print(f"ERROR: Failed to parse PO file {po_file}: {e}", file=sys.stderr)
            sys.exit(1)
    
    # Determine max field name length for alignment
    max_field_len = max(len(field[0]) for field in fields)
    
    # Generate C code
    lines = [
        "/*",
        f" * Auto-generated by scripts/gen_locale.py on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
        f" * Locale: {locale_name}",
        " * DO NOT EDIT",
        " */",
        "",
        '#include "ui/ui_strings.h"',
        "",
        "const stringsTable_t uiStrings = {",
    ]
    
    # Generate uiStrings struct
    for field_name, msgid, _ in fields:
        # Get translation or fall back to English msgid
        translation = translated_strings.get(msgid, msgid)
        escaped = escape_c_string(translation)
        padding = ' ' * (max_field_len - len(field_name))
        lines.append(f'    .{field_name}{padding} = "{escaped}",')
    
    lines.append("};")
    lines.append("")
    
    # Generate englishMsgids struct
    lines.append("const stringsTable_t englishMsgids = {")
    
    for field_name, msgid, _ in fields:
        escaped = escape_c_string(msgid)
        padding = ' ' * (max_field_len - len(field_name))
        lines.append(f'    .{field_name}{padding} = "{escaped}",')
    
    lines.append("};")
    lines.append("")
    
    # Write to file
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    
    print(f"Generated {output_path}")


def generate_pot(fields: List[Tuple[str, str, List[str]]], pot_path: Path):
    """
    Generate POT file from struct fields and annotations.
    
    Args:
        fields: List of (field_name, msgid, translator_comments)
        pot_path: Path to output POT file
    """
    pot = polib.POFile()
    
    # Add metadata
    pot.metadata = {
        'Project-Id-Version': 'OpenRTX',
        'POT-Creation-Date': datetime.now().strftime('%Y-%m-%d %H:%M%z'),
        'MIME-Version': '1.0',
        'Content-Type': 'text/plain; charset=UTF-8',
        'Content-Transfer-Encoding': '8bit',
    }
    
    # Add entries
    for field_name, msgid, translator_comments in fields:
        entry = polib.POEntry(
            msgid=msgid,
            msgstr='',
            comment='\n'.join(translator_comments) if translator_comments else None,
            occurrences=[('openrtx/include/ui/ui_strings.h', field_name)],
        )
        pot.append(entry)
    
    # Write to file
    pot_path.parent.mkdir(parents=True, exist_ok=True)
    pot.save(str(pot_path))
    
    print(f"Generated {pot_path}")


def main():
    parser = argparse.ArgumentParser(
        description='Generate localized UI strings for OpenRTX'
    )
    parser.add_argument(
        '--struct',
        type=Path,
        required=True,
        help='Path to ui_strings.h header file'
    )
    
    # Output mode: either C source or POT
    output_group = parser.add_mutually_exclusive_group(required=True)
    output_group.add_argument(
        '--output',
        type=Path,
        help='Path to output C source file'
    )
    output_group.add_argument(
        '--pot',
        type=Path,
        help='Path to output POT file'
    )
    
    # Optional PO file for translations
    parser.add_argument(
        '--po',
        type=Path,
        help='Path to PO file with translations (optional, for English use no --po)'
    )
    
    args = parser.parse_args()
    
    # Validate inputs
    if not args.struct.exists():
        print(f"ERROR: Struct file not found: {args.struct}", file=sys.stderr)
        sys.exit(1)
    
    if args.po and not args.po.exists():
        print(f"ERROR: PO file not found: {args.po}", file=sys.stderr)
        sys.exit(1)
    
    if args.po and args.pot:
        print(f"ERROR: Cannot specify both --po and --pot", file=sys.stderr)
        sys.exit(1)
    
    # Extract fields from header
    fields = extract_struct_fields(args.struct)
    
    print(f"Extracted {len(fields)} fields from {args.struct}")
    
    # Generate output
    if args.pot:
        generate_pot(fields, args.pot)
    elif args.output:
        generate_c_source(fields, args.po, args.output)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
