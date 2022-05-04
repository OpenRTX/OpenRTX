# Voice prompts 

This repository contains the voice prompt data and scripts and synthesised voice files, used by the OpenRTX firmware

This is a work in progress.

The most stable versions are the UK and USA english versions.

The python script is used to create the voice data pack (.vpr) files which are used by the OpenRTX firmware.

The script reads a wordlist file and downloads synthesised audio from various online TextToSpeech sites, which use Amazon Polly as their engine
The speach audio files are converted to 8kHz sample rate.
The files must be in this format, because the audio data rate used by the proprietary codec in the OpenGD77/OpenRTX firmware only supports 8kHz sample rate

Once the speech files have been downloaded, they are converted to M17 format using  codec2

Once all files have been processed to M17, the script packages the files into a single Voice Prompt pack, binary file.
