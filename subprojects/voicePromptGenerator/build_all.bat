rem need to ensure openRTX\scripts\c2enc.exe is in your path. 
cd languages\english_uk
py ..\..\OpenRTXVoicePromptsBuilder.py -c config.csv

cd ..\english_usa
py ..\..\OpenRTXVoicePromptsBuilder.py -c config.csv
