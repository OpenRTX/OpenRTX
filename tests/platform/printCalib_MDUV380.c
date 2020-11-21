
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include "extFlash_MDx.h"

int main()
{
    extFlash_init();
    extFlash_wakeup();

    while(1)
    {
        getchar();

        uint8_t buf[16];
        uint32_t addr = 0x1000;
        for(; addr < 0x1100; addr += sizeof(buf))
        {
            (void) extFlash_readSecurityRegister(addr, buf, sizeof(buf));
            printf("\r\n%lx: ", addr);
            for(unsigned int i = 0; i < sizeof(buf); i++) printf("%03d ", buf[i]);
        }

        puts("\r");

        for(addr = 0x2000; addr < 0x2100; addr += sizeof(buf))
        {
            (void) extFlash_readSecurityRegister(addr, buf, sizeof(buf));
            printf("\r\n%lx: ", addr);
            for(unsigned int i = 0; i < sizeof(buf); i++) printf("%03d ", buf[i]);
        }

        puts("\r");
    }

    return 0;
}
