/*
Startup file for GD32F3x0

Copyright (c) 2020, Bo Gao <7zlaser@gmail.com>

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list
   of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THE
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "gd32f3x0.h"

extern uint32_t _estack, _eheap;

int uart_getchar(void);
void uart_putchar(uint8_t c);

//Dummy CRT system call implementations
int __attribute__((weak)) _fstat(int fd, struct stat *st)
{
	st->st_mode=S_IFCHR;
	return 0;
}

off_t __attribute__((weak)) _lseek(int fd, off_t pos, int whence)
{
	return 0;
}

int __attribute__((weak)) _close(int fd)
{
	return 0;
}

int __attribute__((weak)) _read(int fd, char *buf, int len)
{
	int i, c;
	// if(fd==STDIN_FILENO)
	// for(i=0;i<len;i++)
	// {
	// 	c=uart_getchar();
	// 	if(c==-1) break;
	// 	buf[i]=c&0xff;
	// }
	return i;
}

int __attribute__((weak)) _write(int fd, char *buf, int len)
{
	// int i;
	// if(fd==STDOUT_FILENO)
	// for(i=0;i<len;i++)
	// 	uart_putchar(buf[i]);
	return len;
}

uint8_t* __attribute__((weak)) _sbrk(int inc)
{
	// static uint8_t *heap=NULL;
	uint8_t *prev;
	// if(!heap) heap=(uint8_t*)&_estack;
	// prev=heap;
	// heap+=inc;
	// if(heap>=(uint8_t*)&_eheap)
	// {
	// 	heap=prev;
	// 	errno=ENOMEM;
	// 	return (uint8_t*)-1;
	// }
	return prev;
}

int __attribute__((weak)) _isatty(int fd) {return 1;}

int __attribute__((weak)) _getpid(void) {return -1;}

void __attribute__((weak)) _kill(int pid, int sig) {return;}

void __attribute__((weak)) _exit(int status) {while(1);}
