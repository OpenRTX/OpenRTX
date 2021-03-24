#!/usr/bin/env python3
# -*- coding: utf-8 -*-
################################################################################################################################################
#
# GD-77 Firmware uploader. By Roger VK3KYY
# 
#
# This script has only been tested on Windows and Linux, it may or may not work on OSX
#
# On Windows,..
# the driver the system installs for the GD-77, which is the HID driver, needs to be replaced by the LibUSB-win32 using Zadig
# for USB device with idVendor=0x15a2, idProduct=0x0073
# Once this driver is installed the CPS and official firmware loader will no longer work as they can't find the device
# To use the CPS etc again, use the DeviceManager to uninstall the driver associated with idVendor=0x15a2, idProduct=0x0073 (this will appear as a libusb-win32 device)
# Then unplug the GD-77 and reconnect, and the HID driver will be re-installed
#
#
# On Linux, depending of you distro, you need to install a special udev rule to automatically unbind the USB HID device to usbhid driver.
#
#
# You also need python3-usb, enum34 and urllib3
#
################################################################################################################################################

######################### Error codes #########################
#
# -1:  Missing firmware file
# -2:  Wrong SGL file format
# -3:  Unencrypted firmware
# -4:  Firmware file is too large
# -5:  Unknown HT model type
# -6:  Command line parsing error
# -7:  Online download firmware location error
# -8:  Online download firmware binary error
# -9:  Online firmware download failure
# -10: Firmare/HT mismatch
# -99: Unsupported GD-77S (will be removed in the futur)
#
###############################################################

import usb
import getopt, sys
import ntpath
import os.path
from array import array
import enum
import urllib3
import re
import tempfile

class SGLFormatOutput(enum.Enum):
    GD_77 = 0
    GD_77S = 1
    DM_1801 = 2
    RD_5R = 3
    UNKNOWN = 4

    def __int__(self):
        return self.value


# Globals
responseOK = [0x41]
outputModes = ["GD-77", "GD-77S", "DM-1801", "RD-5R", "Unknown"]
outputFormat = SGLFormatOutput.GD_77
downloadedFW = ""

########################################################################
# Utilities to dump hex for testing
########################################################################
def hexdump(buf):
    cbuf = ""
    for b in buf:
        cbuf = cbuf + "0x%0.2X " % ord(b)
    return cbuf

def hexdumpArray(buf):
    cbuf = ""
    for b in buf:
        cbuf = cbuf + "0x%0.2X " % b
    return cbuf

def hexdumpArray2(buf):
    cbuf = ""
    for b in buf:
        cbuf = cbuf + "%0.2X-" % b
    return cbuf[:-1]

def strdumpArray(buf):
    cbuf = ""
    for b in buf:
        cbuf = cbuf + chr(b)
    return cbuf

def downloadFirmware(downloadStable):
    url = "https://github.com/rogerclarkmelbourne/OpenGD77/releases"
    urlBase = "http://github.com"
    httpPool = urllib3.PoolManager()
    pattern = ""
    fwVersion = "UNKNOWN"
    fwVersionPatternFormat = r'/{}([0-9\.]+)/'
    urlFW = ""
    webContent = ""
        
    print(" - " + "Try to download the firmware for your {} from the project page".format(outputModes[int(outputFormat)]))
    print(" - " + "Retrieve firmware location");

    try:
        response = httpPool.request('GET', url)
    except urllib3.URLError as e:
        print("".format(e.reason))
        sys.exit(-7)
        
    webContent = str(response.data)
    
    if (outputFormat == SGLFormatOutput.GD_77):
        patternFormat = r'/rogerclarkmelbourne/OpenGD77/releases/download/{}([0-9\.]+)/OpenGD77\.sgl'
    elif (outputFormat == SGLFormatOutput.GD_77S):
        patternFormat = r'/rogerclarkmelbourne/OpenGD77/releases/download/{}([0-9\.]+)/OpenGD77S\.sgl'
    elif (outputFormat == SGLFormatOutput.DM_1801):
        patternFormat = r'/rogerclarkmelbourne/OpenGD77/releases/download/{}([0-9\.]+)/OpenDM1801\.sgl'
    elif (outputFormat == SGLFormatOutput.RD_5R):
        patternFormat = r'/rogerclarkmelbourne/OpenGD77/releases/download/{}([0-9\.]+)/OpenDM5R\.sgl'

    pattern = patternFormat.format("R" if downloadStable == True else "D")
    fwVersionPattern = fwVersionPatternFormat.format("R" if downloadStable == True else "D") 
    contentArray = webContent.split("\n")    
    
    for l in contentArray:
        m = re.search(pattern, l)
        if (m != None):
            urlFW = urlBase + m.group(0)
            
            m = re.search(fwVersionPattern, urlFW)
            if (m != None):
                fwVersion = m.group(0).strip('/')
            
            break
    
    if (len(urlFW)):
        global downloadedFW
        downloadedFW = os.path.join(tempfile.gettempdir(), next(tempfile._get_candidate_names()) + '.sgl')
        
        print(" - " + "Downloading the firmware version {}, please wait".format(fwVersion));
        
        try:
            response = httpPool.request('GET', urlFW, preload_content=False)
        except urllib3.URLError as e:
            print("".format(e.reason))
            sys.exit(-8)

        length = response.getheader('content-length')
        
        if (length != None):
            length = int(length)
            blocksize = max(4096, (length//100))
        else:
            blocksize = 4096
        
        # Download data        
        with open(downloadedFW, "w+b") as f:
            while True:
                data = response.read(blocksize)
                
                if not data:
                    break
                
                f.write(data)
        f.close()
        return True
    
    return False
        
########################################################################
# Send the data packet to the GD-77 and return response
########################################################################
def sendAndGetResponse(dev, cmd):
    USB_WRITE_ENDPOINT  = 0x02
    USB_READ_ENDPOINT   = 0x81
    TRANSFER_LENGTH     = 38
    headerData = [0x0] * 4
        
    headerData[0] = 1
    headerData[1] = 0
    headerData[2] = ((len(cmd) >> 0) & 0xff)
    headerData[3] = ((len(cmd) >> 8) & 0xff)

    cmd = headerData + cmd
    
    #print("TX: " + hexdumpArray2(cmd))
    #print("TX: '{}'".format(strdumpArray(cmd[4:])))

    ret = dev.write(USB_WRITE_ENDPOINT, cmd)
    ret = dev.read(USB_READ_ENDPOINT, TRANSFER_LENGTH + 4, 5000)
    
    #print("RX: " + hexdumpArray2(ret[4:]))
    #print("RX: '{}'".format(strdumpArray(ret[4:])))

    return ret[4:]

########################################################################
# Send the data packet to the GD-77 and confirm the response is correct
########################################################################
def sendAndCheckResponse(dev, cmd, resp):
    USB_WRITE_ENDPOINT  = 0x02
    USB_READ_ENDPOINT   = 0x81
    TRANSFER_LENGTH     = 38
    zeroPad = [0x0] * TRANSFER_LENGTH
    headerData = [0x0] * 4
        
    headerData[0] = 1
    headerData[1] = 0
    headerData[2] = ((len(cmd) >> 0) & 0xff)
    headerData[3] = ((len(cmd) >> 8) & 0xff)

    if (len(resp) < TRANSFER_LENGTH):
        resp = resp + zeroPad[0:TRANSFER_LENGTH - len(resp)]

    cmd = headerData + cmd
    
    #print("TX: " + hexdumpArray2(cmd))
    #print("TX: '{}'".format(strdumpArray(cmd[4:])))

    ret = dev.write(USB_WRITE_ENDPOINT, cmd)
    ret = dev.read(USB_READ_ENDPOINT, TRANSFER_LENGTH + 4, 5000)
    expected = array("B", resp)
    
    #print("RX: " + hexdumpArray2(ret[4:]))
    #print("RX: '{}'".format(strdumpArray(ret[4:])))

    if (expected == ret[4:]):
        return True
    else:
        print("Error. Read returned: " + str(ret))
        return False

 
##############################
# Create checksum data packet
##############################
def createChecksumData(buf, startAddress, endAddress):
    #checksum data starts with a small header, followed by the 32 bit checksum value, least significant byte first
    checkSumData = [ 0x45, 0x4e, 0x44, 0xff, 0xDE, 0xAD, 0xBE, 0xEF ]
    cs = 0
    
    for i in range(startAddress, endAddress):
        cs = cs + buf[i]
     
    checkSumData[4] = (cs % 256) & 0xff
    checkSumData[5] = ((cs >> 8) % 256) & 0xff
    checkSumData[6] = ((cs >> 16) % 256) & 0xff
    checkSumData[7] = ((cs >> 24) % 256) & 0xff
    return checkSumData


def updateBlockAddressAndLength(buf, address, length):
    buf[5] = ((length) % 256) & 0xff
    buf[4] = ((length >> 8) % 256) & 0xff
    buf[3] = ((address) % 256) & 0xff
    buf[2] = ((address >> 8) % 256) & 0xff
    buf[1] = ((address >> 16) % 256) & 0xff
    buf[0] = ((address >> 24) % 256) & 0xff
    return buf


#####################################################
# Open firmware file on disk and sent it to the GD-77
###########################################b##########
def sendFileData(fileBuf, dev):
    dataHeader = [0x00] * (0x20 + 0x06)
    BLOCK_LENGTH = 1024 #1k
    DATA_TRANSFER_SIZE = 0x20
    checksumStartAddress = 0
    address = 0
         
    fileLength = len(fileBuf)
    totalBlocks = (fileLength // BLOCK_LENGTH) + 1

    while address < fileLength:
        if ((address % BLOCK_LENGTH) == 0):
            checksumStartAddress = address
            
        dataHeader = updateBlockAddressAndLength(dataHeader, address, DATA_TRANSFER_SIZE)
        
        if ((address + DATA_TRANSFER_SIZE) < fileLength):
            
            for i in range(DATA_TRANSFER_SIZE):
                dataHeader[6 + i] = fileBuf[address + i]
 
            if  (sendAndCheckResponse(dev, dataHeader, responseOK) == False):
                print("Error sending data")
                return False
                break
            
            address = address + DATA_TRANSFER_SIZE
            
            if ((address % 0x400) == 0):
                print("\r - Sent block " + str(address // BLOCK_LENGTH) + " of "+ str(totalBlocks), end='')
                sys.stdout.flush()

                if (sendAndCheckResponse(dev, createChecksumData(fileBuf, checksumStartAddress, address), responseOK) == False):
                    print("Error sending checksum")
                    return False
                    break
                
        else:
            print("\r - Sending last block                   ", end='')
            sys.stdout.flush()
            
            DATA_TRANSFER_SIZE = fileLength - address

            dataHeader = updateBlockAddressAndLength(dataHeader, address, DATA_TRANSFER_SIZE)
            
            for i in range(DATA_TRANSFER_SIZE):
                dataHeader[6 + i] = fileBuf[address + i]
            
            if (sendAndCheckResponse(dev, dataHeader, responseOK) == False):
                print("Error sending data")
                return False
                break
            
            address = address + DATA_TRANSFER_SIZE

            if (sendAndCheckResponse(dev, createChecksumData(fileBuf, checksumStartAddress, address), responseOK) == False):
                print("Error sending checksum")
                return False
                break

            print("")
    return True

#####################################################
# Probe connected model
###########################################b##########
def probeModel(dev):
    commandLetterA = [ 0x41 ] # 'A'
    command0       = [[ 0x44, 0x4f, 0x57, 0x4e, 0x4c, 0x4f, 0x41, 0x44 ], [ 0x23, 0x55, 0x50, 0x44, 0x41, 0x54, 0x45, 0x3f ]] # 'DOWNLOAD'
    command1       = [ commandLetterA, responseOK ] 
    commandDummy   = [ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF ]
    #commandMOD     = [ 0x46, 0x2D, 0x4D, 0x4F, 0x44, 0xff, 0xff, 0xff ] # F-MOD...
    #commandEND     = [ 0x45, 0x4E, 0x44, 0xFF ] # END.
    commandID      = [ command0, command1 ]
    models         = [[ 'DV01', SGLFormatOutput.GD_77 ], [ 'DV02', SGLFormatOutput.GD_77S ], [ 'DV03', SGLFormatOutput.DM_1801 ]]
    # RD-5R also have "DV02" id

    commandNumber = 0
    while commandNumber < len(commandID):
        if sendAndCheckResponse(dev, commandID[commandNumber][0], commandID[commandNumber][1]) == False:
            return SGLFormatOutput.UNKNOWN
        commandNumber = commandNumber + 1

    resp = sendAndGetResponse(dev, commandDummy)
    ##dummy = sendAndGetResponse(dev, command0[0])

    for x in models:
        if (x[0] == str(resp[:4].tobytes().decode("ascii"))):
            return x[1]

    return SGLFormatOutput.UNKNOWN

###########################################################################################################################################
# Send commands to the GD-77 to verify we are the updater, prepare to program including erasing the internal program flash memory
###########################################################################################################################################
def sendInitialCommands(dev, encodeKey):
    commandLetterA      =[ 0x41] #A
    command0            =[[0x44,0x4f,0x57,0x4e,0x4c,0x4f,0x41,0x44],[0x23,0x55,0x50,0x44,0x41,0x54,0x45,0x3f]] # DOWNLOAD
    command1            =[commandLetterA,responseOK] 
    command3            =[[0x46, 0x2d, 0x50, 0x52, 0x4f, 0x47, 0xff, 0xff],responseOK] #... F-PROG..

    if (outputFormat == SGLFormatOutput.GD_77):
        command2            =[[0x44, 0x56, 0x30, 0x31, (0x61 + 0x00), (0x61 + 0x0C), (0x61 + 0x0D), (0x61 + 0x01)],[0x44, 0x56, 0x30, 0x31]] #.... DV01enhi (DV01enhi comes from deobfuscated sgl file)
        command4            =[[0x53, 0x47, 0x2d, 0x4d, 0x44, 0x2d, 0x37, 0x36, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff],responseOK] #SG-MD-760
        command5            =[[0x4d, 0x44, 0x2d, 0x37, 0x36, 0x30, 0xff, 0xff],responseOK] #MD-760..
    elif (outputFormat == SGLFormatOutput.GD_77S):
        command2            =[[0x44, 0x56, 0x30, 0x32, 0x6D, 0x40, 0x7D, 0x63],[0x44, 0x56, 0x30, 0x32]] #.... DV02Gpmj (thanks Wireshark)
        command4            =[[0x53, 0x47, 0x2d, 0x4d, 0x44, 0x2d, 0x37, 0x33, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff],responseOK] #SG-MD-730
        command5            =[[0x4d, 0x44, 0x2d, 0x37, 0x33, 0x30, 0xff, 0xff],responseOK] #MD-730..
    elif (outputFormat == SGLFormatOutput.DM_1801):
        command2            =[[0x44, 0x56, 0x30, 0x33, 0x74, 0x21, 0x44, 0x39],[0x44, 0x56, 0x30, 0x33]] #.... last 4 bytes of the command are the offset encoded as letters a - p (hard coded fr
        command4            =[[0x42, 0x46, 0x2d, 0x44, 0x4d, 0x52, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff],responseOK] #BF-DMR
        command5            =[[0x31, 0x38, 0x30, 0x31, 0xff, 0xff, 0xff, 0xff],responseOK] # MD-1801
    elif (outputFormat == SGLFormatOutput.RD_5R):
        command2            =[[0x44, 0x56, 0x30, 0x32, 0x53, 0x36, 0x37, 0x62],[0x44, 0x56, 0x30, 0x32]] #.... last 4 bytes of the command are the offset encoded as letters a - p (hard coded fr
        command4            =[[0x42, 0x46, 0x2D, 0x35, 0x52, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff],responseOK] #BF-5R
        command5            =[[0x42, 0x46, 0x2D, 0x35, 0x52, 0xff, 0xff, 0xff],responseOK] # BF-5R

    command6            =[[0x56, 0x31, 0x2e, 0x30, 0x30, 0x2e, 0x30, 0x31],responseOK] #V1.00.01
    commandErase        =[[0x46, 0x2d, 0x45, 0x52, 0x41, 0x53, 0x45, 0xff],responseOK] #F-ERASE
    commandPostErase    =[commandLetterA,responseOK] 
    commandProgram      =[[0x50, 0x52, 0x4f, 0x47, 0x52, 0x41, 0x4d, 0xf],responseOK] #PROGRAM
    commands            =[command0,command1,command2,command3,command4,command5,command6,commandErase,commandPostErase,commandProgram]
    commandNames        =["Sending Download command", "Sending ACK", "Sending encryption key", "Sending F-PROG command", "Sending radio modem number", "Sending radio modem number 2", "Sending version", "Sending erase command", "Send post erase command", "Sending Program command"]
    commandNumber = 0
    
    # Buffer.BlockCopy(encodeKey, 0, command2[0], 4, 4);
    command2[0] = command2[0][0:4] + encodeKey
    
    # Send the commands which the GD-77 expects before the start of the data
    while commandNumber < len(commands):
        print(" - " + commandNames[commandNumber])

        if sendAndCheckResponse(dev, commands[commandNumber][0], commands[commandNumber][1]) == False:
            print("Error sending command")
            return False
            break
        commandNumber = commandNumber + 1
    return True

###########################################################################################################################################
#
###########################################################################################################################################
def checkForSGLAndReturnEncryptedData(fileBuf):
    header_tag = list("SGL!")
    headerModel = []
    
    buf_in_4 = list("".join(map(chr, fileBuf[0:4])))
    headerModel.append(fileBuf[11])

    if buf_in_4 == header_tag:
        # read and decode offset and xor tag
        buf_in_4 = list(fileBuf[0x000C : 0x000C + 4])
        
        for i in range(0, 4):
            buf_in_4[i] = buf_in_4[i] ^ ord(header_tag[i])
        
        offset = buf_in_4[0] + 256 * buf_in_4[1]
        xor_data = [ buf_in_4[2], buf_in_4[3] ]
        
	    # read and decode part of the header
        buf_in_512 = list(fileBuf[offset + 0x0006 : offset + 0x0006 + 512])
    
        xor_idx = 0;
        for i in range(0, 512):
            buf_in_512[i] = buf_in_512[i] ^ xor_data[xor_idx]
            
            xor_idx += 1
            if xor_idx == 2:
                xor_idx = 0

        #
        encodeKey = buf_in_512[0x005D : 0x005D + 4]

        # extract length
        length1 = buf_in_512[0x0000]
        length2 = buf_in_512[0x0001]
        length3 = buf_in_512[0x0002]
        length4 = buf_in_512[0x0003]
        length = (length4 << 24) + (length3 << 16) + (length2 << 8) + length1

        # extract encoded raw firmware
        retBuf = [0x00] * length;
        retBuf = fileBuf[len(fileBuf) - length : len(fileBuf) - length + len(retBuf) ]

        return retBuf, encodeKey, headerModel
    
    print("ERROR: SGL! header is missing.")
    return None, None, None

###########################################################################################################################################
#
###########################################################################################################################################
def usage():
    print("Usage:")
    print("       " + ntpath.basename(sys.argv[0]) + " [OPTION]")
    print("")
    print("    -h, --help                     : Display this help text")
    print("    -f, --firmware=<filename.sgl>  : Flash <filename.sgl> instead of default file \"firmware.sgl\"")
    print("    -m, --model=<type>             : Select transceiver model. Models are: {}".format(", ".join(str(x) for x in outputModes[:-1])) + ".")
    print("    -d, --download                 : Download firmware from the project website")
    print("    -S, --stable                   : Select the stable version while downloading from the project page")
    print("    -U, --unstable                 : Select the development version while downloading from the project page")
    print("")

#####################################################
# Main function.
#####################################################
def main():
    global outputFormat
    sglFile = "firmware.sgl"
    downloadStable = True
    doDownload = False
    doForce = False

    # Command line argument parsing
    try:                                
        opts, args = getopt.getopt(sys.argv[1:], "hf:m:dSUF", ["help", "firmware=", "model=", "download", "stable", "unstable", "force"])
    except getopt.GetoptError as err:
        print(str(err))
        usage()
        sys.exit(-6)

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit(0)
        elif opt in ("-f", "--firmware"):
            sglFile = arg
        elif opt in ("-m", "--model"):
            try:
                index = outputModes.index(arg)
            except ValueError as e:
                print("Model \"{}\" is unknown".format(arg))
                sys.exit(-5)

            outputFormat = SGLFormatOutput(index)
            
            if (outputFormat == SGLFormatOutput.UNKNOWN):
                print("Unsupported model")
                sys.exit(-5)
                
        elif opt in ["-d", "--download"]:
            doDownload = True
        elif opt in ["-S", "--stable"]:
            downloadStable = True
        elif opt in ["-U", "--unstable"]:
            downloadStable = False
        elif opt in ["-F", "--force"]:
            doForce = True
        else:
            assert False, "Unhandled option"

    # Try to connect USB device
    dev = usb.core.find(idVendor=0x15a2, idProduct=0x0073)
    if (dev):
        # Needed on Linux
        try:
            if dev.is_kernel_driver_active(0):
                dev.detach_kernel_driver(0)
        except NotImplementedError as e:
            pass
        
        #seems to be needed for the usb to work !
        dev.set_configuration()

        if (outputFormat == SGLFormatOutput.UNKNOWN):
            outputFormat = probeModel(dev)
            if (outputFormat == SGLFormatOutput.UNKNOWN):
                print("Error. Failed to detect you transceiver model.")
                sys.exit(-5)
            print(" - Detected model: {}".format(outputModes[int(outputFormat)]))
        
        # Try to download the firmware
        if (doDownload == True):
            if (downloadFirmware(downloadStable) == True):
                sglFile = downloadedFW
            else:
                print("Firmware download failed")
                sys.exit(-9)

        if (os.path.isfile(sglFile) == False):
            print("Firmware file \"" + sglFile + "\" is missing.")
            sys.exit(-1)

        print(" - " + "Now flashing your {} with \"{}\"".format(outputModes[int(outputFormat)], sglFile))
        
        with open(sglFile, 'rb') as f:
            fileBuf = f.read()
            
        # Check firmware        
        filename, file_extension = os.path.splitext(sglFile)

        # Define encodeKey according to HT model
        if (outputFormat == SGLFormatOutput.GD_77):
            encodeKey = [ (0x61 + 0x00), (0x61 + 0x0C), (0x61 + 0x0D), (0x61 + 0x01) ]
        elif (outputFormat == SGLFormatOutput.GD_77S):
            encodeKey = [ (0x6D), (0x40), (0x7D), (0x63) ] ## Original header (smaller filelength): was (0x47), (0x70), (0x6d), (0x4a)
        elif (outputFormat == SGLFormatOutput.DM_1801):
            encodeKey = [ (0x74), (0x21), (0x44), (0x39) ]
        elif (outputFormat == SGLFormatOutput.RD_5R):
            encodeKey = [ (0x53), (0x36), (0x37), (0x62) ]

        if (file_extension == ".sgl"):
            firmwareModelTag = { SGLFormatOutput.GD_77: 0x1B , SGLFormatOutput.GD_77S: 0x70, SGLFormatOutput.DM_1801: 0x4F, SGLFormatOutput.RD_5R: 0x5C}
            
            ## Could be a SGL file !
            fileBuf, encodeKey, headerModel = checkForSGLAndReturnEncryptedData(fileBuf)

            if (fileBuf == None):
                print("Error. Missing SGL in .sgl file header")
                sys.exit(-2)
                
            print(" - " + "Firmware file confirmed as SGL")

            if (doForce == False):
                # Check if the firmware matches the transceiver model
                if (headerModel[0] != firmwareModelTag[outputFormat]):
                    print("Error. The firmware doesn't match the transceiver model.")
                    sys.exit(-10)
                    
        else:
            print("Firmware file is an unencrypted binary. Exiting")
            sys.exit(-3)

        if len(fileBuf) > 0x7b000:
            print("Error. Firmware file too large.")
            sys.exit(-4)

        if (sendInitialCommands(dev, encodeKey) == True):
            if (sendFileData(fileBuf, dev) == True):
                print("Firmware update complete. Please reboot the {}.".format(outputModes[int(outputFormat)]))
            else:
                print("Error while sending data")
        else:
            print("Error while sending initial commands")
       
        usb.util.dispose_resources(dev) #free up the USB device
        
    else:
        print("Error. Can't find your transceiver.")
        
    # Remove downloaded firmware, if any
    if (len(downloadedFW)):
        if (os.path.isfile(downloadedFW)):
            os.remove(downloadedFW)


## Run the program
main()
sys.exit(0)
