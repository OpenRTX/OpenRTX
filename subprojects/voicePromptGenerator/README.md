# Voice prompts 

This repository contains the voice prompt data and scripts and synthesised voice files, used by the OpenRTX firmware

This is a work in progress.

The most stable versions are the UK and USA english versions.

The python script is used to create the voice data pack (.vpr) files which are used by the OpenRTX firmware.

The script reads a wordlist file and downloads synthesised audio from various online TextToSpeech sites, which use Amazon Polly as their engine
The speach audio files are converted to 8kHz sample rate.
The files must be in this format, because the audio data rate used by the proprietary codec in the OpenGD77/OpenRTX firmware only supports 8kHz sample rate

Once the speech files have been downloaded, they are converted to M17 format using  codec2

Once all files have been processed to M17, the script packages the files into a single Voice Prompt pack binary file which must be uploaded to the radio.

Please note! the words in the wordlist.csv file must be synchronized with the source code as follows:
Strings upto and including PROMPT_RIGHT_BRACE correspond to voice prompts defined in the voicePrompt_t enum in voicePrompts.h.
These are strings which are not in the UIStrings table because they represent numbers, characters, symbols, and prompts which are pronounced differently to how they are displayed.
strings from LANGUAGE_NAME to the end must correspond directly to the strings in the stringsTable_t struct defined in UIStrings.h.
Do not reorder any strings unless they are also reordered  in  the wordlist.csv file,  voicePrompt_t enum in voicePrompts.h, and stringsTable_t struct in UIStrings.h together.
If you do, you will break what is spoken for the various symbols or strings.