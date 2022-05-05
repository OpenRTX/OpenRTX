#!/usr/bin/env python3

import urllib.request
import json
import csv
import os, sys
import time
import os
import subprocess
import struct
import serial
import platform
import getopt, sys
import serial.tools.list_ports
import ntpath
import shutil
import webbrowser
import re


MAX_TRANSFER_SIZE = 32
CREATE_NO_WINDOW = 0x08000000
DETACHED_PROCESS = 0x00000008
overwrite=False
gain='0'
atempo='1.0'
removeSilenceAtStart = False
# PollyPro is not working
forceTTSMP3Usage = True

#FLASH_WRITE_SIZE = 2

def serialInit(serialDev):
    ser = serial.Serial()
    ser.port = serialDev
    ser.baudrate = 115200
    ser.bytesize = serial.EIGHTBITS
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE
    ser.timeout = 1000.0
    #ser.xonxoff = 0
    #ser.rtscts = 0
    ser.write_timeout = 1000.0
    try:
        ser.open()
    except serial.SerialException as err:
        print(str(err))
        sys.exit(1)
    return ser

def getMemoryArea(ser,buf,mode,bufStart,radioStart,length):
    R_SIZE = 8
    snd = bytearray(R_SIZE)
    snd[0] = ord('R')
    snd[1] = mode
    bufPos = bufStart
    radioPos = radioStart
    remaining = length
    while (remaining > 0):
        batch = min(remaining,MAX_TRANSFER_SIZE)
        snd[2] = (radioPos >> 24) & 0xFF
        snd[3] = (radioPos >> 16) & 0xFF
        snd[4] = (radioPos >>  8) & 0xFF
        snd[5] = (radioPos >>  0) & 0xFF
        snd[6] = (batch >> 8) & 0xFF
        snd[7] = (batch >> 0) & 0xFF
        ret = ser.write(snd)

        if (ret != R_SIZE):
            print("ERROR: write() wrote " + str(ret) + " bytes")
            return False
        while (ser.in_waiting == 0):
            time.sleep(0)

        rcv = ser.read(ser.in_waiting)
        if (rcv[0] == snd[0]):
            gotBytes = (rcv[1] << 8) + rcv[2]
            for i in range(0,gotBytes):
                buf[bufPos] = rcv[i+3]
                bufPos += 1
            radioPos += gotBytes
            remaining -= gotBytes
        else:
            print("read stopped (error at " + str(rcv) + ")")
            return False
    return True

def sendCommand(ser,commandNumber, x_or_command_option_number, y, iSize, alignment, isInverted, message):
    # snd allocation? len 64 or 32? or 23?
    snd = bytearray(7+16)
    snd[0] = ord('C')
    snd[1] = commandNumber
    snd[2] = x_or_command_option_number
    snd[3] = y
    snd[4] = iSize
    snd[5] = alignment
    snd[6] = isInverted
    # copy message to snd[7] (max 16 bytes)
    i = 7
    for c in message:
        if (i > 7+16-1):
            break
        snd[i] = ord(c)
        i += 1
    ser.flush()
    ret = ser.write(snd)
    if (ret != 7+16): # length?
        print("ERROR: write() wrote " + str(ret) + " bytes")
        return False
    while (ser.in_waiting == 0):
        time.sleep(0)
    rcv = ser.read(ser.in_waiting)
    return len(rcv) > 2 and rcv[1] == snd[1]


def wavSendData(ser,buf,radioStart,length):
    FLASH_SEND_SIZE = 8
    snd = bytearray(FLASH_SEND_SIZE+MAX_TRANSFER_SIZE)
    snd[0] = ord('W')
    snd[1] = 7#data type 7
    bufPos = 0
    radioPos = radioStart
    remaining = length
    while (remaining > 0):
        transferSize = min(remaining,MAX_TRANSFER_SIZE)

        snd[2] = (radioPos >> 24) & 0xFF
        snd[3] = (radioPos >> 16) & 0xFF
        snd[4] = (radioPos >>  8) & 0xFF
        snd[5] = (radioPos >>  0) & 0xFF
        snd[6] = (transferSize >>  8) & 0xFF
        snd[7] = (transferSize >>  0) & 0xFF
        snd[FLASH_SEND_SIZE:FLASH_SEND_SIZE+transferSize] = buf[bufPos:bufPos+transferSize]

        ret = ser.write(snd)
        if (ret != FLASH_SEND_SIZE+MAX_TRANSFER_SIZE):
            print("ERROR: write() wrote " + str(ret) + " bytes")
            return False
        while (ser.in_waiting == 0):
            time.sleep(0)
        rcv = ser.read(ser.in_waiting)
        if not (rcv[0] == snd[0] and rcv[1] == snd[1]):
            print("ERROR: at "+str(radioPos))
        bufPos += transferSize
        radioPos += transferSize
        remaining -= transferSize
    return True

def convert2AMBE(ser,infile,outfile):

    with open(infile,'rb') as f:
        ambBuf = bytearray(16*1024)# arbitary 16k buffer
        buf = bytearray(f.read())
        f.close();
        sendCommand(ser,0, 0, 0, 0, 0, 0, "")#show CPS screen as this disables the radio etc
        sendCommand(ser,6, 5, 0, 0, 0, 0,  "")#codecInitInternalBuffers
        wavBufPos = 0

        bufLen = len(buf)
        ambBufPos=0;
        ambFrameBuf = bytearray(27)
        startPos=0
        #if (infile[0:11] !="PROMPT_SPACE"):
        #    stripSilence = True;

        if (removeSilenceAtStart==True):
            while (startPos<len(buf) and  buf[startPos]==0 and buf[(startPos+1)]==0):
               startPos = startPos + 2;
            if (startPos == len(buf)):
                startPos = 0

        print("Compress to AMBE "+infile + " pos:+" + str(startPos));

        wavBufPos = startPos

        while (wavBufPos < bufLen):
            #print('.', end='')
            sendCommand(ser,6, 6, 0, 0, 0, 0,  "")#codecInitInternalBuffers
            transferLen = min(960,bufLen-wavBufPos)
            #print("sent " + str(transferLen));
            wavSendData(ser,buf[wavBufPos:wavBufPos+transferLen],0,transferLen)

            getMemoryArea(ser,ambFrameBuf,8,0,0,27)# mode 8 is read from AMBE
            ambBuf[ambBufPos:ambBufPos+27] = ambFrameBuf
            wavBufPos = wavBufPos + 960
            ambBufPos = ambBufPos + 27

        sendCommand(ser,5, 0, 0, 0, 0, 0, "")# close CPS screen
        with open(outfile,'wb') as f:
            f.write(ambBuf[0:ambBufPos])

        #print("")#newline


def convertToRaw(inFile,outFile):
    print("ConvertToRaw "+ inFile + " -> " + outFile + " gain="+gain + " tempo="+atempo)
    callArgs = ['ffmpeg','-y','-i', inFile,'-filter:a','atempo=+'+atempo+',volume='+gain+'dB','-ar','8000','-f','s16le',outFile]
    if os.name == 'nt':
        subprocess.call(callArgs, creationflags=CREATE_NO_WINDOW)#'-af','silenceremove=1:0:-50dB'
    elif os.name == 'posix':
        subprocess.call(callArgs)#'-af','silenceremove=1:0:-50dB'



def downloadPollyPro(voiceName,fileStub,promptText,speechSpeed):
    retval=True
    hasDownloaded = False
    myobj = {'text-input': promptText,
             'voice':voiceName,
             'format':'mp3',# mp3 or ogg_vorbis or json
             'frequency':'22050',
             'effect':speechSpeed}

    data = urllib.parse.urlencode(myobj)
    data = data.encode('ascii')

    mp3FileName = voiceName + "/" +fileStub+".mp3"
    rawFileName = voiceName + "/" +fileStub+".raw"
    m17Filename = voiceName + "/" +fileStub+".m17"
    if (not os.path.exists(mp3FileName) or overwrite==True):
        with urllib.request.urlopen("https://voicepolly.pro/speech-converter.php", data) as f:
            resp = f.read().decode('utf-8')
            print("PollyPro: Downloading synthesised speech for text: \"" + promptText + "\" -> " + mp3FileName)
            if resp.endswith('.mp3'):
                with urllib.request.urlopen(resp) as response, open(mp3FileName, 'wb') as out_file:
                    audioData = response.read() # a `bytes` object
                    out_file.write(audioData)
                    hasDownloaded = True
                    retval = True
            else:
                print("Error requesting sound " + resp)
                retval=False
#    else:
#        print("Download skipping " + file_name)

    if (hasDownloaded == True or not os.path.exists(rawFileName) or overwrite == True):
        convertToRaw(mp3FileName,rawFileName)
        if (os.path.exists(m17Filename)):
            os.remove(m17Filename)# ambe file is now out of date, so delete it

    return retval

def downloadTTSMP3(voiceName,fileStub,promptText):
    myobj = {'msg': promptText,
             'lang':voiceName,
             'source':'ttsmp3.com'}

    data = urllib.parse.urlencode(myobj)
    myStr = str.replace(data,"+","%20") #hacky fix because urlencode is not encoding spaces to %20
    data = myStr.encode('ascii')


    mp3FileName = voiceName + "/" +fileStub+".mp3"
    rawFileName = voiceName + "/" +fileStub+".raw"
    m17Filename = voiceName + "/" +fileStub+".m17"
    hasDownloaded = False

    if (not os.path.exists(mp3FileName) or overwrite==True):
        print("Download TTSMP3 " +  promptText)
        with urllib.request.urlopen("https://ttsmp3.com/makemp3_new.php", data) as f:
            resp = f.read().decode('utf-8')
            print("TTSMP3: Downloading synthesised speech for text: \"" + promptText + "\" -> " + mp3FileName)
            print(resp)
            data = json.loads(resp)
            if (data['Error'] == 0):
                print(data['URL'])
                # Download the file from `url` and save it locally under `file_name`:
                with urllib.request.urlopen(data['URL']) as response, open(mp3FileName, 'wb') as out_file:
                    mp3data = response.read() # a `bytes` object
                    out_file.write(mp3data)
                    ## need to resample to 8kHz sample rate because ttsmp3 files are 22.05kHz
                    out_file.close()
                    hasDownloaded = True

            else:
                print("Error requesting sound")
                return False

    if (hasDownloaded == True or not os.path.exists(rawFileName) or overwrite == True):
        convertToRaw(mp3FileName,rawFileName)
        if (os.path.exists(m17Filename)):
            os.remove(m17Filename)# ambe file is now out of date, so delete it

    return True


def downloadSpeechForWordList(filename,voiceName):
    retval = True
    speechSpeed="normal"

    with open(filename,"r",encoding='utf-8') as csvfile:
        reader = csv.DictReader(filter(lambda row: row[0]!='#', csvfile))
        for row in reader:
            promptName = row['PromptName'].strip()

            speechPrefix = row['PromptSpeechPrefix'].strip()

            ## PollyPro is not working.
            if ((forceTTSMP3Usage == False) and (speechPrefix != "") and False):
                #Use VoicePolly as its not a special SSML that it doesnt handle
                if (speechPrefix.find("<prosody rate=")!=-1):
                    matchObj = re.search(r'\".*\"',speechPrefix)
                    if (matchObj):
                        speechSpeed = matchObj.group(0)[1:-1]

                downloadPollyPro(voiceName, promptName, row['PromptText'], speechSpeed)
            else:
                promptTTSText = row['PromptSpeechPrefix'].strip() +  row['PromptText'] + row['PromptSpeechPostfix'].strip()

                if (downloadTTSMP3(voiceName,promptName,promptTTSText)==False):
                    retval=False
                    break
        # Add voice name as last prompt
        if (downloadTTSMP3(voiceName, "PROMPT_VOICE_NAME", voiceName)==False):
            retval=False
        return retval



def encodeFile(ser,fileStub):
    if ((not os.path.exists(fileStub+".m17")) or overwrite==True):
        convert2AMBE(ser,fileStub+".raw",fileStub+".m17")
        #os.remove(fileStub+".raw")
##    else:
##       print("Encode skipping " + fileStub)

def encodeWordList(ser,filename,voiceName,forceReEncode):
    with open(filename,"r",encoding='utf-8') as csvfile:
        reader = csv.DictReader(filter(lambda row: row[0]!='#', csvfile))
        for row in reader:
            promptName = row['PromptName'].strip()
            fileStub = voiceName + "/" + promptName

            encodeFile(ser,fileStub)
        promptName = "PROMPT_VOICE_NAME"
        fileStub = voiceName + "/" + promptName

        encodeFile(ser,fileStub)

def buildDataPack(filename,voiceName,outputFileName):
    print("Building...")
    promptsDict={}#create an empty dictionary
    with open(filename,"r",encoding='utf-8') as csvfile:
        reader = csv.DictReader(filter(lambda row: row[0]!='#', csvfile))
        for row in reader:
            promptName = row['PromptName'].strip()
            infile = voiceName+"/" + promptName+".m17"
            with open(infile,'rb') as f:
                promptsDict[promptName] = bytearray(f.read())
                f.close()
        promptName = "PROMPT_VOICE_NAME"
        infile = voiceName+"/" + promptName+".m17"
        with open(infile,'rb') as f:
            promptsDict[promptName] = bytearray(f.read())
            f.close()
                
    MAX_PROMPTS = 350
    headerTOCSize = (MAX_PROMPTS * 4) + 4 + 4
    outBuf = bytearray(headerTOCSize)
    outBuf[0:3]  = bytes([0x56, 0x50, 0x00, 0x00])#Magic number
    outBuf[4:7]  = bytes([0x04, 0x00, 0x09, 0x06])#Version number
    outBuf[8:11] = bytes([0x00, 0x00, 0x00, 0x00])#First prompt audio is at offset zero
    bufPos=12;
    cumulativelength=0;
    for prompt in promptsDict:
        cumulativelength = cumulativelength + len(promptsDict[prompt]);
        outBuf[bufPos+3] = (cumulativelength >> 24) & 0xFF
        outBuf[bufPos+2] = (cumulativelength >> 16) & 0xFF
        outBuf[bufPos+1] = (cumulativelength >>  8) & 0xFF
        outBuf[bufPos+0] = (cumulativelength >>  0) & 0xFF
        bufPos = bufPos + 4
    #outputFileName = voiceName+'/voice_prompts_'+voiceName+'.bin'
    with open(outputFileName,'wb') as f:
        f.write(outBuf[0:headerTOCSize])#Should be headerTOCSize
        for prompt in promptsDict:
            f.write(promptsDict[prompt])
    f.close()
    print("Built voice pack "+outputFileName);


PROGRAM_VERSION = "0.0.2"

def usage(message=""):
    print("GD-77 voice prompts creator. v" + PROGRAM_VERSION)
    if (message != ""):
        print()
        print(message)
        print()

    print("Usage:  " + ntpath.basename(sys.argv[0]) + " [OPTION]")
    print("")
    print("    -h Display this help text,")
    print("    -c Configuration file (csv) - using this overrides all other options")
    print("    -f=<wordlist_csv_file> : Wordlist file. Required for all functions")
    ##print("    -n=<Voice_name>       : Voice name for synthesised speech from Voicepolly.pro and temporary folder name")
    ##print("    -s                    : Download synthesised speech from Voicepolly.pro")
    print("    -T                    : Download synthesised speech from ttsmp3.com")
    print("    -e                    : Encode previous download synthesised speech files, using the GD-77")
    print("    -b                    : Build voice prompts data pack from Encoded spech files ")
    print("    -d=<device>           : Use the specified device as serial port,")
    print("    -o                    : Overwrite existing files")
    print("    -g=gain               : Audio level gain adjust in db.  Default is 0, but can be negative or positive numbers")
    print("    -t=tempo              : Audio tempo (from 0.5 to 2).  Default is {}".format(atempo))
    print("    -r                    : Remove silence from beginning of audio files")
    print("")

def main():
    global overwrite
    global gain
    global atempo
    global removeSilenceAtStart, forceTTSMP3Usage

    fileName   = ""#wordlist_english.csv"
    outputName = ""#voiceprompts.bin"
    voiceName = ""#Matthew or Nicole etc
    configName = ""

    # Default tty
    if (platform.system() == 'Windows'):
        serialDev = "COM71"
    else:
        serialDev = "/dev/ttyACM0"
    #Automatically search for the OpenGD77 device port
    for port in serial.tools.list_ports.comports():
        if (port.description.find("OpenGD77")==0):
            #print("Found OpenGD77 on port "+port.device);
            serialDev = port.device
            
    # Command line argument parsing
    try:
        ##opts, args = getopt.getopt(sys.argv[1:], "hof:n:seb:d:c:g:Tt:")
        opts, args = getopt.getopt(sys.argv[1:], "hof:eb:d:c:g:Tt:")
    except getopt.GetoptError as err:
        print(str(err))
        usage("")
        sys.exit(2)

    if os.name == 'nt':
        if (str(shutil.which("ffmpeg.exe")).find("ffmpeg") == -1):
            usage("ERROR: You must install ffmpeg. See https://www.ffmpeg.org/download.html")
            #webbrowser.open("https://www.ffmpeg.org/download.html")
            sys.exit(2)
    elif os.name == 'posix':
        if (str(shutil.which("ffmpeg")).find("ffmpeg") == -1):
            usage("ERROR: You must install ffmpeg. See https://www.ffmpeg.org/download.html")
            #webbrowser.open("https://www.ffmpeg.org/download.html")
            sys.exit(2)

    for opt, arg in opts:
        if opt in ("-h"):
            usage()
            sys.exit(2)
        elif opt in ("-f"):
            fileName = arg
        #elif opt in ("-n"):
        #    voiceName = arg
        elif opt in ("-d"):
            serialDev = arg
        elif opt in ("-c"):
            configName = arg
        elif opt in ("-o"):
            overwrite = True
        elif opt in ("-g"):
            gain = arg
        elif opt in ("-r"):
            removeSilenceAtStart = arg
        elif opt in ("-T"):
            forceTTSMP3Usage = True
        elif opt in ('-t'):
            atempo = arg

    if (configName!=""):
        print("Using Config file: {}...".format(configName))

        with open(configName,"r",encoding='utf-8') as csvfile:
            reader = csv.DictReader(filter(lambda row: row[0]!='#', csvfile))
            for row in reader:
                wordlistFilename = row['Wordlist_file'].strip()
                voiceName = row['Voice_name'].strip()
                voicePackName = row['Voice_pack_name'].strip()
                download = row['Download'].strip()
                encode = row['Encode'].strip()
                createPack = row['Createpack'].strip()
                gain = row['Volume_change_db'].strip()
                rs = row['Remove_silence'].strip()
                cfg_atempo = row['Audio_tempo'].strip()
         ## If Audio_tempo is not set, use the default value
                
                if cfg_atempo != '':
                    atempo = cfg_atempo

                ## Add audio tempo value to the filename
                voicePackName = voicePackName.replace('.vpr', '-' + atempo + '.vpr');

                print("Processing " + wordlistFilename+" "+voiceName+" "+voicePackName)

                if not os.path.exists(voiceName):
                    print("Creating folder " + voiceName + " for temporary files")
                    os.mkdir(voiceName);

                if (rs=='y' or rs=='Y'):
                    removeSilenceAtStart = True
                else:
                    removeSilenceAtStart = False

                if (download=='y' or download=='Y'):
                    if (downloadSpeechForWordList(wordlistFilename,voiceName)==False):
                     sys.exit(2)

                if (encode=='y' or encode=='Y'):
                    ser = serialInit(serialDev)

                    encodeWordList(ser,wordlistFilename,voiceName,True)
                    if (ser.is_open):
                        ser.close()
                if (createPack=='y' or createPack=='Y'):
                    buildDataPack(wordlistFilename,voiceName,voicePackName)

        sys.exit(0)


    if (fileName=="" or voiceName==""):
        usage("ERROR: Filename and Voicename must be specified for all operations")
        sys.exit(2)

    if not os.path.exists(voiceName):
        print("Creating folder " + voiceName + " for temporary files")
        os.mkdir(voiceName);

    #for opt, arg in opts:
    #    if opt in ("-s"):
    #        if (downloadSpeechForWordList(fileName,voiceName)==False):
    #            sys.exit(2)

    for opt, arg in opts:
        if opt in ("-e"):
            ser = serialInit(serialDev)
            encodeWordList(ser,fileName,voiceName,True)
            if (ser.is_open):
                ser.close()

    for opt, arg in opts:
        if opt in ("-b"):
            outputName = arg
            buildDataPack(fileName,voiceName,outputName)

main()
sys.exit(0)
