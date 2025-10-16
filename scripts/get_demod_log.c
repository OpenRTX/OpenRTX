/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

int fd;

int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        if (tcgetattr (fd, &tty) != 0)
        {
                printf("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes", errno);
}

typedef struct
{
    int16_t sample;
    int32_t conv;
    float   conv_th;
    int32_t sample_index;
    float   qnt_pos_avg;
    float   qnt_neg_avg;
    int32_t symbol;
    int32_t frame_index;
    uint8_t flags;
    uint8_t _empty;
}
__attribute__((packed)) log_entry_t;

int main() {
    //char *portname = "/dev/ttyACM0";
    char *portname = "/dev/serial/by-id/usb-STMicroelectronics_STM32_Virtual_ComPort_in_FS_Mode_00000000050C-if00";
    fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    while (fd < 0)
    {
        printf ("error %d opening %s: %s", errno, portname, strerror (errno));
        sleep(1);
        fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    }
    set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
    log_entry_t log = { 0 };
    FILE *csv_log = fopen("serial_demod_log.csv", "w");
    fprintf(csv_log, "Sample,Convolution,Threshold,Index,Max,Min,Symbol,I,Flags\n");
    while(true)
    {
        read(fd, &log, sizeof(log));
        fprintf(csv_log, "%" PRId16 ",%d,%f,%d,%f,%f,%d,%d,%d\n",
                log.sample,
                log.conv,
                log.conv_th,
                log.sample_index,
                log.qnt_pos_avg,
                log.qnt_neg_avg,
                log.symbol,
                log.frame_index,
                log.flags);
        fflush(csv_log);
    }
    fclose(csv_log);
}
