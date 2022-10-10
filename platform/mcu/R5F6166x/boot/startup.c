/***************************************************************************
 *   Copyright (C) 2022 by Mark Saunders,                                  *
                           Silvano Seva IU2KWO                             *
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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <string.h>

// Main program entry point
extern int main();

/**
 * Reset handler, called by hardware immediately after reset
 */
void Reset_Handler() __attribute__((interrupt_handler, noreturn));
void Reset_Handler()
{
    // initialise the SP for non-vectored code
    asm("mov.l   #_stack,er7");

    //These are defined in the linker script
    extern unsigned char _etext asm("_etext");
    extern unsigned char _data asm("_data");
    extern unsigned char _edata asm("_edata");
    extern unsigned char _bss_start asm("_bss_start");
    extern unsigned char _bss_end asm("_bss_end");

    //Initialize .data section, clear .bss section
    unsigned char *etext=&_etext;
    unsigned char *data=&_data;
    unsigned char *edata=&_edata;
    unsigned char *bss_start=&_bss_start;
    unsigned char *bss_end=&_bss_end;
    memcpy(data, etext, edata-data);
    memset(bss_start, 0, bss_end-bss_start);

    //set_imask_ccr((_UBYTE)1);
    asm("orc.b #0x80, ccr");

    main();

    while(1) ;
}

/**
 * All unused interrupts call this function.
 */
void Default_Handler()
{
    while(1) ;
}

void __attribute__((weak, alias("Default_Handler"))) INT_Illegal_code();
void __attribute__((weak, alias("Default_Handler"))) INT_Trace();
void __attribute__((weak, alias("Default_Handler"))) INT_NMI();
void __attribute__((weak, alias("Default_Handler"))) INT_TRAP0();
void __attribute__((weak, alias("Default_Handler"))) INT_TRAP1();
void __attribute__((weak, alias("Default_Handler"))) INT_TRAP2();
void __attribute__((weak, alias("Default_Handler"))) INT_TRAP3();
void __attribute__((weak, alias("Default_Handler"))) INT_CPU_Address();
void __attribute__((weak, alias("Default_Handler"))) INT_DMA_Address();
void __attribute__((weak, alias("Default_Handler"))) INT_Sleep();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ0();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ1();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ2();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ3();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ4();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ5();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ6();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ7();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ8();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ9();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ10();
void __attribute__((weak, alias("Default_Handler"))) INT_IRQ11();
void __attribute__((weak, alias("Default_Handler"))) INT_32KOVI_TM32K();
void __attribute__((weak, alias("Default_Handler"))) INT_WOVI();
void __attribute__((weak, alias("Default_Handler"))) INT_CMI();
void __attribute__((weak, alias("Default_Handler"))) INT_ADI();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI0A_TPU0();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI0B_TPU0();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI0C_TPU0();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI0D_TPU0();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI0V_TPU0();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI1A_TPU1();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI1B_TPU1();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI1V_TPU1();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI1U_TPU1();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI2A_TPU2();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI2B_TPU2();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI2V_TPU2();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI2U_TPU2();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI3A_TPU3();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI3B_TPU3();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI3C_TPU3();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI3D_TPU3();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI3V_TPU3();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI4A_TPU4();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI4B_TPU4();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI4V_TPU4();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI4U_TPU4();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI5A_TPU5();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI5B_TPU5();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI5V_TPU5();
void __attribute__((weak, alias("Default_Handler"))) INT_TCI5U_TPU5();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIA0_TMR0();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIB0_TMR0();
void __attribute__((weak, alias("Default_Handler"))) INT_OVI0_TMR0();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIA1_TMR1();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIB1_TMR1();
void __attribute__((weak, alias("Default_Handler"))) INT_OVI1_TMR1();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIA2_TMR2();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIB2_TMR2();
void __attribute__((weak, alias("Default_Handler"))) INT_OVI2_TMR2();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIA3_TMR3();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIB3_TMR3();
void __attribute__((weak, alias("Default_Handler"))) INT_OVI3_TMR3();
void __attribute__((weak, alias("Default_Handler"))) INT_TEND0_DMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_TEND1_DMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_TEND2_DMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_TEND3_DMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_TEND0_EXDMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_TEND1_EXDMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_TEND2_EXDMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_TEND3_EXDMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_EEND0_DMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_EEND1_DMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_EEND2_DMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_EEND3_DMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_EEND0_EXDMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_EEND1_EXDMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_EEND2_EXDMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_EEND3_EXDMAC();
void __attribute__((weak, alias("Default_Handler"))) INT_ERI0_SCI0();
void __attribute__((weak, alias("Default_Handler"))) INT_RXI0_SCI0();
void __attribute__((weak, alias("Default_Handler"))) INT_TXI0_SCI0();
void __attribute__((weak, alias("Default_Handler"))) INT_TEI0_SCI0();
void __attribute__((weak, alias("Default_Handler"))) INT_ERI1_SCI1();
void __attribute__((weak, alias("Default_Handler"))) INT_RXI1_SCI1();
void __attribute__((weak, alias("Default_Handler"))) INT_TXI1_SCI1();
void __attribute__((weak, alias("Default_Handler"))) INT_TEI1_SCI1();
void __attribute__((weak, alias("Default_Handler"))) INT_ERI2_SCI2();
void __attribute__((weak, alias("Default_Handler"))) INT_RXI2_SCI2();
void __attribute__((weak, alias("Default_Handler"))) INT_TXI2_SCI2();
void __attribute__((weak, alias("Default_Handler"))) INT_TEI2_SCI2();
void __attribute__((weak, alias("Default_Handler"))) INT_ERI4_SCI4();
void __attribute__((weak, alias("Default_Handler"))) INT_RXI4_SCI4();
void __attribute__((weak, alias("Default_Handler"))) INT_TXI4_SCI4();
void __attribute__((weak, alias("Default_Handler"))) INT_TEI4_SCI4();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI6A_TPU6();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI6B_TPU6();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI6C_TPU6();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI6D_TPU6();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI6V_TPU6();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI7A_TPU7();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI7B_TPU7();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI7V_TPU7();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI7U_TPU7();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI8A_TPU8();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI8B_TPU8();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI8V_TPU8();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI8U_TPU8();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI9A_TPU9();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI9B_TPU9();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI9C_TPU9();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI9D_TPU9();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI9V_TPU9();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI10A_TPU10();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI10B_TPU10();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI10V_TPU10();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI10U_TPU10();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI11A_TPU11();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI11B_TPU11();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI11V_TPU11();
void __attribute__((weak, alias("Default_Handler"))) INT_TGI11U_TPU11();
void __attribute__((weak, alias("Default_Handler"))) INT_IICI0_IIC2();
void __attribute__((weak, alias("Default_Handler"))) INT_IICI1_IIC2();
void __attribute__((weak, alias("Default_Handler"))) INT_RXI5_SCI5();
void __attribute__((weak, alias("Default_Handler"))) INT_TXI5_SCI5();
void __attribute__((weak, alias("Default_Handler"))) INT_ERI5_SCI5();
void __attribute__((weak, alias("Default_Handler"))) INT_TEI5_SCI5();
void __attribute__((weak, alias("Default_Handler"))) INT_RXI6_SCI6();
void __attribute__((weak, alias("Default_Handler"))) INT_TXI6_SCI6();
void __attribute__((weak, alias("Default_Handler"))) INT_ERI6_SCI6();
void __attribute__((weak, alias("Default_Handler"))) INT_TEI6_SCI6();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIA4_CMIB4_TMR4();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIA5_CMIB5_TMR5();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIA6_CMIB6_TMR6();
void __attribute__((weak, alias("Default_Handler"))) INT_CMIA7_CMIB7_TMR7();
void __attribute__((weak, alias("Default_Handler"))) INT_USBINTN0_USB();
void __attribute__((weak, alias("Default_Handler"))) INT_USBINTN1_USB();
void __attribute__((weak, alias("Default_Handler"))) INT_USBINTN2_USB();
void __attribute__((weak, alias("Default_Handler"))) INT_USBINTN3_USB();
void __attribute__((weak, alias("Default_Handler"))) INT_ADI1();
void __attribute__((weak, alias("Default_Handler"))) INT_RESUME_USB();

//Interrupt vector table
void (* const __Vectors[])() __attribute__((used, section(".vectors"))) =
{
    Reset_Handler,           // vector 0 Power on reset
    0,                       // vector 1 Reserved
    0,                       // vector 2 Reserved
    0,                       // vector 3 Reserved
    INT_Illegal_code,        // vector 4 Illegal code
    INT_Trace,               // vector 5 Trace
    0,                       // vector 6 Reserved
    INT_NMI,                 // vector 7 NMI
    INT_TRAP0,               // vector 8 trap #0
    INT_TRAP1,               // vector 9 trap #1
    INT_TRAP2,               // vector 10 trap #2
    INT_TRAP3,               // vector 11 trap #3
    INT_CPU_Address,         // vector 12 CPU Address error
    INT_DMA_Address,         // vector 13 DMA Address error
    0,                       // vector 14 Reserved
    0,                       // vector 15 Reserved
    0,                       // vector 16 Reserved
    0,                       // vector 17 Reserved
    INT_Sleep,               // vector 18 Sleep
    0,                       // vector 19 Reserved
    0,                       // vector 20 Reserved
    0,                       // vector 21 Reserved
    0,                       // vector 22 Reserved
    0,                       // vector 23 Reserved
    0,                       // vector 24 Reserved
    0,                       // vector 25 Reserved
    0,                       // vector 26 Reserved
    0,                       // vector 27 Reserved
    0,                       // vector 28 Reserved
    0,                       // vector 29 Reserved
    0,                       // vector 30 Reserved
    0,                       // vector 31 Reserved
    0,                       // vector 32 Reserved
    0,                       // vector 33 Reserved
    0,                       // vector 34 Reserved
    0,                       // vector 35 Reserved
    0,                       // vector 36 Reserved
    0,                       // vector 37 Reserved
    0,                       // vector 38 Reserved
    0,                       // vector 39 Reserved
    0,                       // vector 40 Reserved
    0,                       // vector 41 Reserved
    0,                       // vector 42 Reserved
    0,                       // vector 43 Reserved
    0,                       // vector 44 Reserved
    0,                       // vector 45 Reserved
    0,                       // vector 46 Reserved
    0,                       // vector 47 Reserved
    0,                       // vector 48 Reserved
    0,                       // vector 49 Reserved
    0,                       // vector 50 Reserved
    0,                       // vector 51 Reserved
    0,                       // vector 52 Reserved
    0,                       // vector 53 Reserved
    0,                       // vector 54 Reserved
    0,                       // vector 55 Reserved
    0,                       // vector 56 Reserved
    0,                       // vector 57 Reserved
    0,                       // vector 58 Reserved
    0,                       // vector 59 Reserved
    0,                       // vector 60 Reserved
    0,                       // vector 61 Reserved
    0,                       // vector 62 Reserved
    0,                       // vector 63 Reserved
    INT_IRQ0,                // vector 64 External trap IRQ0
    INT_IRQ1,                // vector 65 External trap IRQ1
    INT_IRQ2,                // vector 66 External trap IRQ2
    INT_IRQ3,                // vector 67 External trap IRQ3
    INT_IRQ4,                // vector 68 External trap IRQ4
    INT_IRQ5,                // vector 69 External trap IRQ5
    INT_IRQ6,                // vector 70 External trap IRQ6
    INT_IRQ7,                // vector 71 External trap IRQ7
    INT_IRQ8,                // vector 72 External trap IRQ8
    INT_IRQ9,                // vector 73 External trap IRQ9
    INT_IRQ10,               // vector 74 External trap IRQ10
    INT_IRQ11,               // vector 75 External trap IRQ11
    0,                       // vector 76 Reserved
    0,                       // vector 77 Reserved
    0,                       // vector 78 Reserved
    INT_32KOVI_TM32K,        // vector 79 32KOVI TM32K
    0,                       // vector 80 Reserved
    INT_WOVI,                // vector 81 WOVI
    0,                       // vector 82 Reserved
    INT_CMI,                 // vector 83 CMI
    0,                       // vector 84 Reserved
    0,                       // vector 85 Reserved
    INT_ADI,                 // vector 86 ADI
    0,                       // vector 87 Reserved
    INT_TGI0A_TPU0,          // vector 88 TGI0A TPU0
    INT_TGI0B_TPU0,          // vector 89 TGI0B TPU0
    INT_TGI0C_TPU0,          // vector 90 TGI0C TPU0
    INT_TGI0D_TPU0,          // vector 91 TGI0D TPU0
    INT_TCI0V_TPU0,          // vector 92 TCI0V TPU0
    INT_TGI1A_TPU1,          // vector 93 TGI1A TPU1
    INT_TGI1B_TPU1,          // vector 94 TGI1B TPU1
    INT_TCI1V_TPU1,          // vector 95 TCI1V TPU1
    INT_TCI1U_TPU1,          // vector 96 TCI1U TPU1
    INT_TGI2A_TPU2,          // vector 97 TGI2A TPU2
    INT_TGI2B_TPU2,          // vector 98 TGI2B TPU2
    INT_TCI2V_TPU2,          // vector 99 TCI2V TPU2
    INT_TCI2U_TPU2,          // vector 100 TCI2U TPU2
    INT_TGI3A_TPU3,          // vector 101 TGI3A TPU3
    INT_TGI3B_TPU3,          // vector 102 TGI3B TPU3
    INT_TGI3C_TPU3,          // vector 103 TGI3C TPU3
    INT_TGI3D_TPU3,          // vector 104 TGI3D TPU3
    INT_TCI3V_TPU3,          // vector 105 TCI3V TPU3
    INT_TGI4A_TPU4,          // vector 106 TGI4A TPU4
    INT_TGI4B_TPU4,          // vector 107 TGI4B TPU4
    INT_TCI4V_TPU4,          // vector 108 TCI4V TPU4
    INT_TCI4U_TPU4,          // vector 109 TCI4U TPU4
    INT_TGI5A_TPU5,          // vector 110 TGI5A TPU5
    INT_TGI5B_TPU5,          // vector 111 TGI5B TPU5
    INT_TCI5V_TPU5,          // vector 112 TCI5V TPU5
    INT_TCI5U_TPU5,          // vector 113 TCI5U TPU5
    0,                       // vector 114 Reserved
    0,                       // vector 115 Reserved
    INT_CMIA0_TMR0,          // vector 116 CMIA0 TMR0
    INT_CMIB0_TMR0,          // vector 117 CMIB0 TMR0
    INT_OVI0_TMR0,           // vector 118 OVI0 TMR0
    INT_CMIA1_TMR1,          // vector 119 CMIA1 TMR1
    INT_CMIB1_TMR1,          // vector 120 CMIB1 TMR1
    INT_OVI1_TMR1,           // vector 121 OVI1 TMR1
    INT_CMIA2_TMR2,          // vector 122 CMIA2 TMR2
    INT_CMIB2_TMR2,          // vector 123 CMIB2 TMR2
    INT_OVI2_TMR2,           // vector 124 OVI2 TMR2
    INT_CMIA3_TMR3,          // vector 125 CMIA3 TMR3
    INT_CMIB3_TMR3,          // vector 126 CMIB3 TMR3
    INT_OVI3_TMR3,           // vector 127 OVI3 TMR3
    INT_TEND0_DMAC,          // vector 128 TEND0 DMAC
    INT_TEND1_DMAC,          // vector 129 TEND1 DMAC
    INT_TEND2_DMAC,          // vector 130 TEND2 DMAC
    INT_TEND3_DMAC,          // vector 131 TEND3 DMAC
    INT_TEND0_EXDMAC,        // vector 132 TEND0 EXDMAC
    INT_TEND1_EXDMAC,        // vector 133 TEND1 EXDMAC
    INT_TEND2_EXDMAC,        // vector 134 TEND2 EXDMAC
    INT_TEND3_EXDMAC,        // vector 135 TEND3 EXDMAC
    INT_EEND0_DMAC,          // vector 136 EEND0 DMAC
    INT_EEND1_DMAC,          // vector 137 EEND1 DMAC
    INT_EEND2_DMAC,          // vector 138 EEND2 DMAC
    INT_EEND3_DMAC,          // vector 139 EEND3 DMAC
    INT_EEND0_EXDMAC,        // vector 140 EEND0 EXDMAC
    INT_EEND1_EXDMAC,        // vector 141 EEND1 EXDMAC
    INT_EEND2_EXDMAC,        // vector 142 EEND2 EXDMAC
    INT_EEND3_EXDMAC,        // vector 143 EEND3 EXDMAC
    INT_ERI0_SCI0,           // vector 144 ERI0 SCI0
    INT_RXI0_SCI0,           // vector 145 RXI0 SCI0
    INT_TXI0_SCI0,           // vector 146 TXI0 SCI0
    INT_TEI0_SCI0,           // vector 147 TEI0 SCI0
    INT_ERI1_SCI1,           // vector 148 ERI1 SCI1
    INT_RXI1_SCI1,           // vector 149 RXI1 SCI1
    INT_TXI1_SCI1,           // vector 150 TXI1 SCI1
    INT_TEI1_SCI1,           // vector 151 TEI1 SCI1
    INT_ERI2_SCI2,           // vector 152 ERI2 SCI2
    INT_RXI2_SCI2,           // vector 153 RXI2 SCI2
    INT_TXI2_SCI2,           // vector 154 TXI2 SCI2
    INT_TEI2_SCI2,           // vector 155 TEI2 SCI2
    0,                       // vector 156 Reserved
    0,                       // vector 157 Reserved
    0,                       // vector 158 Reserved
    0,                       // vector 159 Reserved
    INT_ERI4_SCI4,           // vector 160 ERI4 SCI4
    INT_RXI4_SCI4,           // vector 161 RXI4 SCI4
    INT_TXI4_SCI4,           // vector 162 TXI4 SCI4
    INT_TEI4_SCI4,           // vector 163 TEI4 SCI4
    INT_TGI6A_TPU6,          // vector 164 TGI6A TPU6
    INT_TGI6B_TPU6,          // vector 165 TGI6B TPU6
    INT_TGI6C_TPU6,          // vector 166 TGI6C TPU6
    INT_TGI6D_TPU6,          // vector 167 TGI6D TPU6
    INT_TGI6V_TPU6,          // vector 168 TGI6V TPU6
    INT_TGI7A_TPU7,          // vector 169 TGI7A TPU7
    INT_TGI7B_TPU7,          // vector 170 TGI7B TPU7
    INT_TGI7V_TPU7,          // vector 171 TGI7V TPU7
    INT_TGI7U_TPU7,          // vector 172 TGI7U TPU7
    INT_TGI8A_TPU8,          // vector 173 TGI8A TPU8
    INT_TGI8B_TPU8,          // vector 174 TGI8B TPU8
    INT_TGI8V_TPU8,          // vector 175 TGI8V TPU8
    INT_TGI8U_TPU8,          // vector 176 TGI8U TPU8
    INT_TGI9A_TPU9,          // vector 177 TGI9A TPU9
    INT_TGI9B_TPU9,          // vector 178 TGI9B TPU9
    INT_TGI9C_TPU9,          // vector 179 TGI9C TPU9
    INT_TGI9D_TPU9,          // vector 180 TGI9D TPU9
    INT_TGI9V_TPU9,          // vector 181 TGI9V TPU9
    INT_TGI10A_TPU10,        // vector 182 TGI10A TPU10
    INT_TGI10B_TPU10,        // vector 183 TGI10B TPU10
    0,                       // vector 184 Reserved
    0,                       // vector 185 Reserved
    INT_TGI10V_TPU10,        // vector 186 TGI10V TPU10
    INT_TGI10U_TPU10,        // vector 187 TGI10U TPU10
    INT_TGI11A_TPU11,        // vector 188 TGI11A TPU11
    INT_TGI11B_TPU11,        // vector 189 TGI11B TPU11
    INT_TGI11V_TPU11,        // vector 190 TGI11V TPU11
    INT_TGI11U_TPU11,        // vector 191 TGI11U TPU11
    0,                       // vector 192 Reserved
    0,                       // vector 193 Reserved
    0,                       // vector 194 Reserved
    0,                       // vector 195 Reserved
    0,                       // vector 196 Reserved
    0,                       // vector 197 Reserved
    0,                       // vector 198 Reserved
    0,                       // vector 199 Reserved
    0,                       // vector 200 Reserved
    0,                       // vector 201 Reserved
    0,                       // vector 202 Reserved
    0,                       // vector 203 Reserved
    0,                       // vector 204 Reserved
    0,                       // vector 205 Reserved
    0,                       // vector 206 Reserved
    0,                       // vector 207 Reserved
    0,                       // vector 208 Reserved
    0,                       // vector 209 Reserved
    0,                       // vector 210 Reserved
    0,                       // vector 211 Reserved
    0,                       // vector 212 Reserved
    0,                       // vector 213 Reserved
    0,                       // vector 214 Reserved
    0,                       // vector 215 Reserved
    INT_IICI0_IIC2,          // vector 216 IICI0 IIC2
    0,                       // vector 217 Reserved
    INT_IICI1_IIC2,          // vector 218 IICI1 IIC2
    0,                       // vector 219 Reserved
    INT_RXI5_SCI5,           // vector 220 RXI5 SCI5
    INT_TXI5_SCI5,           // vector 221 TXI5 SCI5
    INT_ERI5_SCI5,           // vector 222 ERI5 SCI5
    INT_TEI5_SCI5,           // vector 223 TEI5 SCI5
    INT_RXI6_SCI6,           // vector 224 RXI6 SCI6
    INT_TXI6_SCI6,           // vector 225 TXI6 SCI6
    INT_ERI6_SCI6,           // vector 226 ERI6 SCI6
    INT_TEI6_SCI6,           // vector 227 TEI6 SCI6
    INT_CMIA4_CMIB4_TMR4,    // vector 228 CMIA4/CMIB4 TMR4
    INT_CMIA5_CMIB5_TMR5,    // vector 229 CMIA5/CMIB5 TMR5
    INT_CMIA6_CMIB6_TMR6,    // vector 230 CMIA6/CMIB6 TMR6
    INT_CMIA7_CMIB7_TMR7,    // vector 231 CMIA7/CMIB7 TMR7
    INT_USBINTN0_USB,        // vector 232 USBINTN0 USB
    INT_USBINTN1_USB,        // vector 233 USBINTN1 USB
    INT_USBINTN2_USB,        // vector 234 USBINTN2 USB
    INT_USBINTN3_USB,        // vector 235 USBINTN3 USB
    0,                       // vector 236 Reserved
    INT_ADI1,                // vector 237 ADI1
    INT_RESUME_USB,          // vector 238 RESUME USB
    0,                       // vector 239 Reserved
    0,                       // vector 240 Reserved
    0,                       // vector 241 Reserved
    0,                       // vector 242 Reserved
    0,                       // vector 243 Reserved
    0,                       // vector 244 Reserved
    0,                       // vector 245 Reserved
    0,                       // vector 246 Reserved
    0,                       // vector 247 Reserved
    0,                       // vector 248 Reserved
    0,                       // vector 249 Reserved
    0,                       // vector 250 Reserved
    0,                       // vector 251 Reserved
    0,                       // vector 252 Reserved
    0,                       // vector 253 Reserved
    0,                       // vector 254 Reserved
    0,                       // vector 255 Reserved
};
