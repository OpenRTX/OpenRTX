#!/usr/bin/env bash
#
# Build OpenRTX voice prompts
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Usage information
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build OpenRTX voice prompts from C source headers using Piper TTS.

Options:
    -h, --help              Show this help message
    -o, --output FILE       Output VPC file (default: voiceprompts.vpc)
    -v, --voice MODEL       Piper voice model (default: en_US-lessac-medium)
    -d, --data-dir DIR      Directory for voice model data (default: ~/.local/share/piper-tts)
    -g, --gain DB           Volume gain in dB (default: 0.0)
    -t, --tempo RATE        Tempo rate (default: 1.25)
    -k, --keep-temp         Keep temporary files for debugging
    --no-overrides          Don't use pronunciation overrides
    
Available voice models:
    en_US-lessac-medium     (recommended for radio - clear, neutral)
    en_GB-alan-medium       (British English)
    en_US-amy-medium        (American female)
    en_US-ryan-high         (American male, high quality)
    
Examples:
    $0                                  # Build with defaults
    $0 --voice en_GB-alan-medium       # Use British voice
    $0 --tempo 1.0 --gain 3.0          # Slower speech, louder
    $0 --keep-temp                      # Keep intermediate files

EOF
    exit 0
}

# Check dependencies
check_dependencies() {
    local missing=0
    
    echo -e "${YELLOW}Checking dependencies...${NC}"
    
    if ! command -v python3 &> /dev/null; then
        echo -e "${RED}✗ python3 not found${NC}"
        missing=1
    else
        echo -e "${GREEN}✓ python3${NC}"
    fi
    
    if ! command -v piper &> /dev/null; then
        echo -e "${RED}✗ piper not found${NC}"
        echo -e "  Install with: pip install piper-tts"
        missing=1
    else
        echo -e "${GREEN}✓ piper${NC}"
    fi
    
    if ! command -v ffmpeg &> /dev/null; then
        echo -e "${RED}✗ ffmpeg not found${NC}"
        echo -e "  Install with: apt install ffmpeg (or brew install ffmpeg on macOS)"
        missing=1
    else
        echo -e "${GREEN}✓ ffmpeg${NC}"
    fi
    
    if ! command -v c2enc &> /dev/null; then
        echo -e "${RED}✗ c2enc not found${NC}"
        echo -e "  Install with: apt install libcodec2-dev"
        missing=1
    else
        echo -e "${GREEN}✓ c2enc${NC}"
    fi
    
    if [ $missing -eq 1 ]; then
        echo -e "\n${RED}Missing required dependencies. Please install them and try again.${NC}"
        exit 1
    fi
    
    echo ""
}

# Parse command line arguments
OUTPUT="$PROJECT_ROOT/voiceprompts.vpc"
VOICE="en_US-lessac-medium"
DATA_DIR="${HOME}/.local/share/piper-tts"
GAIN="0.0"
TEMPO="1.25"
KEEP_TEMP=""
USE_OVERRIDES="--overrides $SCRIPT_DIR/voice_overrides.json"

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            ;;
        -o|--output)
            OUTPUT="$2"
            shift 2
            ;;
        -v|--voice)
            VOICE="$2"
            shift 2
            ;;
        -d|--data-dir)
            DATA_DIR="$2"
            shift 2
            ;;
        -g|--gain)
            GAIN="$2"
            shift 2
            ;;
        -t|--tempo)
            TEMPO="$2"
            shift 2
            ;;
        -k|--keep-temp)
            KEEP_TEMP="--keep-temp"
            shift
            ;;
        --no-overrides)
            USE_OVERRIDES=""
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            ;;
    esac
done

# Check dependencies
check_dependencies

# Ensure voice model is downloaded
echo -e "${YELLOW}Ensuring voice model is available...${NC}"
mkdir -p "$DATA_DIR"
python3 -m piper.download_voices "$VOICE" --data-dir "$DATA_DIR"
echo -e "${GREEN}✓ Voice model ready${NC}"
echo ""

# Run the generator
echo -e "${GREEN}Generating voice prompts...${NC}"
echo -e "  Output: $OUTPUT"
echo -e "  Voice: $VOICE"
echo -e "  Data dir: $DATA_DIR"
echo -e "  Gain: ${GAIN}dB"
echo -e "  Tempo: ${TEMPO}x"
echo ""

python3 "$SCRIPT_DIR/generate_voice_prompts.py" \
    --output "$OUTPUT" \
    --voice "$VOICE" \
    --data-dir "$DATA_DIR" \
    --gain "$GAIN" \
    --tempo "$TEMPO" \
    --project-root "$PROJECT_ROOT" \
    $USE_OVERRIDES \
    $KEEP_TEMP

echo -e "\n${GREEN}✓ Voice prompts build complete!${NC}"
echo -e "  Output file: $OUTPUT"
