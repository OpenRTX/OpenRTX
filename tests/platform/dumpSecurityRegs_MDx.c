/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/*
 *
 * Use instructions:
 * setup like normal, e.g. meson setup --cross-file cross_arm.txt build_arm
 * configure this test: meson configure -Dtest=dumpSecurityRegs_MDx build_arm/
 * make sure START_TSK_STKSIZE in openrtx/src/bootstrap.c is increased (2048/sizeof(CPU_STK) is a good number) or test will not run
 * flash like normal, e.g. meson compile -C build_arm openrtx_md380_flash
 *
 * Then reboot the radio into this app once it's flashed.
 * It will print out the registers after you press a button.
 *
 * Example: minicom -D /dev/ttyACM0
 * (ctrl-a o to bring up the menu, serial port setup to choose 115200 8N1)
 * (ctrl-a l to bring up minicom logging, enter to set the log filename)
 * press enter key or whatever you like, view output
 * (ctrl-a q  to close minicom)
 * the logged output is in that .cap file you named with ctrl-L
 *
 * Name it with the model number (MD380, MD380V, MD380G, MD380VG) (or MD390 variants)
 * and then the serial number, like MD380_SN12345678.cap
 * put it in this repo under data/tests/dumpSecurityRegs_MDx
 *
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <W25Qx.h>

void printChunk(void *chunk)
{
    uint8_t *ptr = ((uint8_t *) chunk);
    for(size_t i = 0; i < 16; i++) printf("%02x ", ptr[i]);
    for(size_t i = 0; i < 16; i++)
    {
        if((ptr[i] > 0x22) && (ptr[i] < 0x7f)) printf("%c", ptr[i]);
        else printf(".");
    }
}

void printSecurityRegister(uint32_t reg)
{
    uint8_t secRegister[256] = {0};
    W25Qx_wakeup();
    W25Qx_readSecurityRegister(reg, secRegister, 256);
    W25Qx_sleep();

    for(uint32_t addr = 0; addr < 256; addr += 16)
    {
        printf("\r\n%02lx: ", addr);
        printChunk(&(secRegister[addr]));
    }
}

int main()
{
    W25Qx_init();

    while(1)
    {
        getchar();

        printf("0x1000:");
        printSecurityRegister(0x1000);

        printf("\r\n\r\n0x2000:");
        printSecurityRegister(0x2000);

        printf("\r\n\r\n0x3000:");
        printSecurityRegister(0x3000);
    }

    return 0;
}
