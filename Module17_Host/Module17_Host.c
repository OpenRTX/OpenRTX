/*
 *   Copyright (C) 2014 by Jonathan Naylor G4KLX and John Hays K7VE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Adapted by K7VE from G4KLX dv3000d
 *
 *   (2025) Adapted by KD0OSS for Module17 DSTAR/P25 hack
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <sys/stat.h>
#include "./imbe_vocoder/imbe_vocoder_api.h"


#if defined(RASPBERRY_PI)
#include <wiringPi.h>
int  wiringPiSetup();
void pinMode(int pin, int mode);
void digitalWrite(int pin, int value);
#define OUTPUT  1
#define HIGH    1
#define LOW     0
#endif

#define VERSION "2025-2-27"

void delay(unsigned int delay)
{
	struct timespec tim, tim2;
	tim.tv_sec = 0;
	tim.tv_nsec = delay * 1000UL;
	nanosleep(&tim, &tim2);
};

#define	AMBE3000_HEADER_LEN	    4U
#define	AMBE3000_START_BYTE	    0x61U
#define AMBE3000_TYPE_CONFIG	0x00
#define AMBE3000_PKT_PARITYMODE	0x3f
#define AMBE3000_COMPAND_ALAW   0x32
#define AMBE3000_PKT_RATEP		0x0a

const uint8_t SDV_RESETSOFTCFG_ALAW[11]  = {0x61, 0x00, 0x07, 0x00, 0x34, 0xE0, 0x00, 0x00, 0xE0, 0x00, 0x00};  // Noise suppression Enabled and A-LAW
const uint8_t AMBE3000_PARITY_DISABLE[8] = {AMBE3000_START_BYTE, 0x00, 0x04, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_PARITYMODE, 0x00, 0x2f, 0x14};
const uint8_t AMBE3000_SET_ALAW[6] = {AMBE3000_START_BYTE, 0x00, 0x01, AMBE3000_TYPE_CONFIG, AMBE3000_COMPAND_ALAW, 0x03};
const uint8_t AMBE2000_2400_1200[17] = {AMBE3000_START_BYTE, 0x00, 0x0d, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_RATEP, 0x01U, 0x30U, 0x07U, 0x63U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x48U};     //DVSI DSTAR
const uint8_t AMBEP251_4400_2800[17] = {AMBE3000_START_BYTE, 0x00, 0x0d, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_RATEP, 0x05U, 0x58U, 0x08U, 0x6BU, 0x10U, 0x30U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x90U};		//DVSI P25 USB Dongle FEC

#define	BUFFER_LENGTH		    400U

int serialMod17Fd = 0;
int serialDongleFd = 0;
int debugD = 0;
int debugM = 0;
char dongletty[50] = "";
char mod17tty[50] = "";
int16_t  decodeTable[256];
imbe_vocoder vocoder;
fd_set fds;
bool dongleDone = true;
bool donglePresent = false;

typedef struct
{
    uint8_t brightness;           // Display brightness
    uint8_t contrast;             // Display contrast
    uint8_t sqlLevel;             // Squelch level
    uint8_t voxLevel;             // Vox level
    int8_t  utc_timezone;         // Timezone, in units of half hours
    bool    gps_enabled;          // GPS active
    char    callsign[10];         // Plaintext callsign
    uint8_t display_timer   : 4,  // Standby timer
            m17_can         : 4;  // M17 CAN
    uint8_t vpLevel         : 3,  // Voice prompt level
            vpPhoneticSpell : 1,  // Phonetic spell enabled
            macroMenuLatch  : 1,  // Automatic latch of macro menu
            _reserved       : 3;
    bool    m17_can_rx;           // Check M17 CAN on RX
    char    m17_dest[10];         // M17 destination
    char    M17_meta_text[53];    // M17 Meta Text to send
    char    dstar_mycall[9];      // DSTAR MyCall
    char    dstar_urcall[9];      // DSTAR UrCall
    char    dstar_rpt1call[9];    // DSTAR Rpt1Call
    char    dstar_rpt2call[9];    // DSTAR Rpt2Call
    char    dstar_suffix[5];      // DSTAR Suffix
    char    dstar_message[21];    // DSTAR slow speed txt
    char    dstar_header[38];     // DSTAR TX header data
    uint32_t p25_srcId;           // P25 Source ID (DMR ID)
	uint32_t p25_dstId;           // P25 Destination ID (DMR ID or TG)
	uint16_t p25_nac;             // P25 NAC
}
__attribute__((packed)) settings_t;

void dump(char *text, unsigned char *data, unsigned int length)
{
	unsigned int offset = 0U;
	unsigned int i;

	fputs(text, stdout);
	fputc('\n', stdout);

	while (length > 0U) {
		unsigned int bytes = (length > 16U) ? 16U : length;

		fprintf(stdout, "%04X:  ", offset);

		for (i = 0U; i < bytes; i++)
			fprintf(stdout, "%02X ", data[offset + i]);

		for (i = bytes; i < 16U; i++)
			fputs("   ", stdout);

		fputs("   *", stdout);

		for (i = 0U; i < bytes; i++) {
			unsigned char c = data[offset + i];

			if (isprint(c))
				fputc(c, stdout);
			else
				fputc('.', stdout);
		}

		fputs("*\n", stdout);

		offset += 16U;

		if (length >= 16U)
			length -= 16U;
		else
			length = 0U;
	}
	
}

#if defined(RASPBERRY_PI)

int openWiringPi(void)
{
        int ret = wiringPiSetup();
        if (ret == -1) {
                fprintf(stderr, "Module17_Host: error when initialising wiringPi\n");
                return 0;
        }

        pinMode(7, OUTPUT);             // Power

        // Reset the hardware
        digitalWrite(7, LOW);

        delay(20UL);

        digitalWrite(7, HIGH);

        delay(750UL);

        if (debug)
                fprintf(stdout, "opened the Wiring Pi library\n");

        return 1;
}

#endif


int openSerial(char *serial)
{
	struct termios tty;
	int fd;
	
	fd = open(serial, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "Module17_Host: error when opening %s, errno=%d\n", serial, errno);
		return fd;
	}

	if (tcgetattr(fd, &tty) != 0) {
		fprintf(stderr, "Module17_Host: error %d from tcgetattr\n", errno);
		return -1;
	}

	cfsetospeed(&tty, B460800);
	cfsetispeed(&tty, B460800);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_iflag &= ~IGNBRK;
	tty.c_lflag = 0;

	tty.c_oflag = 0;
	tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 20;

	tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);

	tty.c_cflag |= (CLOCAL | CREAD);

	tty.c_cflag &= ~(PARENB | PARODD);
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		fprintf(stderr, "Module17_Host: error %d from tcsetattr\n", errno);
		return -1;
	}

	if (debugD && debugM)
		fprintf(stdout, "opened %s\n", serial);
	return fd;
}


unsigned char alawEncode(int16_t sample)
{
    uint16_t samp = (sample & 0xFFFF);
    int sign=(samp & 0x8000) >> 8;
    if (sign != 0)
    {
        samp=(int16_t)-samp;
        sign=0x80;
    }

    if (samp > 32635) samp = 32635;

    int exp=7;
    int expMask;
    for (expMask=0x4000;(samp & expMask)==0 && exp>0; exp--, expMask >>= 1);
    int mantis = (samp >> ((exp == 0) ? 4 : (exp + 3))) & 0x0f;
    unsigned char alaw = (unsigned char)(sign | exp << 4 | mantis);
    return (unsigned char)(alaw^0xD5);
} // end alawEncode


void alawDecode(void)
{
    for (int i = 0; i < 256; i++)
    {
        int input = (i & 0xFF) ^ 85;
        int mantissa = (input & 15) << 4;
        int segment = (input & 112) >> 4;
        int value = mantissa + 8;
        if (segment >= 1) value += 256;
        if (segment > 1) value <<= (segment - 1);
        if ((input & 128) == 0) value = -value;
        decodeTable[i] = (int16_t)value;
    }
} // end alawDecode

void loadConfig()
{

}

void saveConfig(unsigned char *buffer)
{
	uint8_t *config;

	config = (uint8_t*)malloc(sizeof(settings_t) + 4);
	if (config == NULL) return;

	free(config);
}

void getConfig()
{
	uint8_t buf[] = {0x61, 0x00, 0x01, 0x10, 0x01};
    size_t len = write(serialMod17Fd, buf, 5);
	if (len != 5)
	{
		fprintf(stderr, "Module17_Host: error when writing to Module17, errno=%d\n", errno);
		return;
	}
}

int processSerialmod17(void)
{
	unsigned char buffer[BUFFER_LENGTH];
	unsigned int  respLen;
	unsigned int  offset;
	uint8_t       imbe[17] = {0x61U, 0x00U, 0x0DU, 0x00U, 0x00U, 0x0AU};
	int16_t       pcm[160];
	uint8_t       payload[200] = {0x61U, 0x00U, 0xA4U, 0x02U, 0x00U, 0x0AU};
	ssize_t       len;

	len = read(serialMod17Fd, buffer, 1);
	if (len == 0) return 1;

	if (len != 1) {
		fprintf(stderr, "Module17_Host: error when reading from Module17, errno=%d\n", errno);
		return 0;
	}

	if (buffer[0U] != AMBE3000_START_BYTE) {
		fprintf(stderr, "Module17_Host: unknown byte from Module17, 0x%02X\n", buffer[0U]);
		return 1;
	}

	offset = 0U;
	while (offset < (AMBE3000_HEADER_LEN - 1U)) {
		len = read(serialMod17Fd, buffer + 1U + offset, AMBE3000_HEADER_LEN - 1 - offset);
		if (len == 0)
			delay(5UL);

		offset += len;
	}

	respLen = buffer[1U] * 256U + buffer[2U];

	offset = 0U;
	while (offset < respLen) {
		len = read(serialMod17Fd, buffer + AMBE3000_HEADER_LEN + offset, respLen - offset);

		if (len == 0)
			delay(5UL);

		offset += len;
	}

	respLen += AMBE3000_HEADER_LEN;

	if (debugM)
		dump((char*)"Module17 serial data", buffer, respLen);

	switch (buffer[3])
	{
	case 0x03:
		vocoder.decode_4400(pcm, buffer+6);
		for (int i=0;i<160;i++)
		{
			payload[i+6] = alawEncode(pcm[i]);
		}
		if (debugM)
			dump((char*)"Audio sent", payload, 166);
		len = write(serialMod17Fd, payload, 166);
		if (len != 166)
		{
			fprintf(stderr, "Module17_Host: error when writing to Module17, errno=%d\n", errno);
			return 0;
		}
		break;

	case 0x04:
		for (int i=0;i<160;i++)
		{
			pcm[i] = decodeTable[buffer[i+6]];
		}
		vocoder.encode_4400(pcm, imbe+6);
		if (debugM)
			dump((char*)"IMBE sent", imbe, 17);
		len = write(serialMod17Fd, imbe, 17);
		if (len != 17)
		{
			fprintf(stderr, "Module17_Host: error when writing to Module17, errno=%d\n", errno);
			return 0;
		}
		break;

	default:
		if (donglePresent)
		{
			dongleDone = false;
			len = write(serialDongleFd, buffer, respLen);
			if (len != respLen) {
				fprintf(stderr, "Module17_Host: error when writing to the dongle serial port, errno=%d\n", errno);
				return 0;
			}
		}
	}

	return 1;
}

int processSerialdongle(void)
{
	unsigned char buffer[BUFFER_LENGTH];
	unsigned int  respLen;
	unsigned int  offset;
	ssize_t       len;

	len = read(serialDongleFd, buffer, 1);
	if (len == 0) return 1;

	if (len != 1) {
		fprintf(stderr, "Module17_Host: error when reading from the dongle, errno=%d\n", errno);
		return 0;
	}

	if (buffer[0U] != AMBE3000_START_BYTE) {
		fprintf(stderr, "Module17_Host: unknown byte from the DV3000, 0x%02X\n", buffer[0U]);
		return 1;
	}

	offset = 0U;
	while (offset < (AMBE3000_HEADER_LEN - 1U)) {
		len = read(serialDongleFd, buffer + 1U + offset, AMBE3000_HEADER_LEN - 1 - offset);

		if (len == 0)
			delay(1UL);

		offset += len;
	}

	respLen = buffer[1U] * 256U + buffer[2U];

	offset = 0U;
	while (offset < respLen) {
		len = read(serialDongleFd, buffer + AMBE3000_HEADER_LEN + offset, respLen - offset);

		if (len == 0)
			delay(1UL);

		offset += len;
	}

	respLen += AMBE3000_HEADER_LEN;

	if (debugD)
		dump((char*)"AMBE dongle serial data", buffer, respLen);

	dongleDone = true;
	len = write(serialMod17Fd, buffer, respLen);
	if (len != respLen) {
		fprintf(stderr, "Module17_Host: error when writing to the Module17 port, errno=%d\n", errno);
		return 0;
	}
	return 1;
}

int main(int argc, char **argv)
{
	int daemon = 0;
	int topFd;
	int commnum;
	int ret;
	int c;

	while ((c = getopt(argc, argv, "d:a:i:vx")) != -1) {
		switch (c) {
			case 'd':
				daemon = 1;
				break;
			case 'a':
				strcpy(dongletty,optarg);
				break;
			case 'i':
				strcpy(mod17tty,optarg);
				break;
			case 'v':
				fprintf(stdout, "Module17_Host: version " VERSION "\n");
				return 0;
			case 'x':
				debugM = 1;
				debugD = 1;
				break;
			default:
				fprintf(stderr, "Usage: Module17_Host [-d] [-a Dongle tty] [-i Module17 tty] [-v] [-x]\n");
				return 1;
		}
	}

	if (strlen(dongletty) > 8) {
		donglePresent = true;
		// fprintf(stderr, "An AMBE dongle tty filename (-a /dev/ttyXXX) must be set.\n");
	}

	if (strlen(mod17tty) < 1) {
		fprintf(stderr, "An Module17 tty filename (-i /dev/ttyXXX) must be set.\n");
		return 1;
	}

	if (daemon) {
		pid_t pid = fork();

		if (pid < 0) {
			fprintf(stderr, "Module17_Host: error in fork(), exiting\n");
			return 1;
		}

		// If this is the parent, exit
		if (pid > 0)
			return 0;

		// We are the child from here onwards
		setsid();

		umask(0);
	}

#if defined(RASPBERRY_PI)
        ret = openWiringPi();
        if (!ret) {
		fprintf(stderr,"Unable to open WiringPi, exiting\n");
                return 1;
	} else {
		fprintf(stderr,"Reset DV3000\n");
	}
#endif

    serialMod17Fd = openSerial(mod17tty);
	if (serialMod17Fd < 0)
		return 1;
	uint8_t buf[6];

	if (donglePresent)
	{
		serialDongleFd = openSerial(dongletty);
		if (serialDongleFd < 0)
			return 1;

		write(serialDongleFd, SDV_RESETSOFTCFG_ALAW, 11);
		sleep(2);
		read(serialDongleFd, buf, 6);
		if (buf[4] != 0x39)
		{
			fprintf(stderr, "Dongle not resetting.\n");
			fflush(stderr);
			return -1;
		}
		write(serialDongleFd, AMBE3000_PARITY_DISABLE, 8);
		usleep(5000);
		read(serialDongleFd, buf, 6);
/*		write(serialDongleFd, AMBE3000_SET_ALAW, 5);
		usleep(5000);
		read(serialDongleFd, buf, 6);
		if (buf[5] != 0x00)
		{
			fprintf(stderr, "Dongle not in ALAW mode.\n");
			fflush(stderr);
			return -1;
		} */
		write(serialDongleFd, AMBE2000_2400_1200, 17);
		//	write(serialDongleFd, AMBEP251_4400_2800, 17);
		usleep(5000);
		read(serialDongleFd, buf, 6);
	}

	buf[0] = 0x61;
	buf[1] = 0x03;
	write(serialMod17Fd, buf, 2); // let Module17 board know host is active

	topFd = serialMod17Fd;
	if (donglePresent)
	{
		if (serialDongleFd > topFd)
			topFd = serialDongleFd;
	}
	topFd++;

	alawDecode();

	for (;;) {
		FD_ZERO(&fds);
		FD_SET(serialMod17Fd, &fds);
		if (donglePresent)
		{
			FD_SET(serialDongleFd, &fds);
			ret = select(topFd, &fds, NULL, NULL, NULL);
			if (ret < 0) {
				fprintf(stderr, "Module17_Host: error from select, errno=%d\n", errno);
				return 1;
			}
		}

		if (FD_ISSET(serialMod17Fd, &fds) && dongleDone) {
			ret = processSerialmod17();
			if (!ret)
				return 1;
		}

		if (donglePresent)
		{
			if (FD_ISSET(serialDongleFd, &fds)) {
				ret = processSerialdongle();
				if (!ret)
					return 1;
			}
		}
	}

	return 0;
}
