/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014, 2015, 2016 by Terraneo Federico and   *
 *      Luigi Rinaldi                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
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

#ifndef CC2520_CONSTANTS_H
#define CC2520_CONSTANTS_H

namespace miosix {

/**
 * CC2520 registers, both FREG and SREG
 */
enum class CC2520Register
{
    FRMFILT0         = 0x00,
    FRMFILT1         = 0x01,
    SRCMATCH         = 0x02,
    SRCSHORTEN0      = 0x04,
    SRCSHORTEN1      = 0x05,
    SRCSHORTEN2      = 0x06,
    SRCEXTEN0        = 0x08,
    SRCEXTEN1        = 0x09,
    SRCEXTEN2        = 0x0A,
    FRMCTRL0         = 0x0C,
    FRMCTRL1         = 0x0D,
    RXENABLE0        = 0x0E,
    RXENABLE1        = 0x0F,
    EXCFLAG0         = 0x10,
    EXCFLAG1         = 0x11,
    EXCFLAG2         = 0x12,
    EXCMASKA0        = 0x14,
    EXCMASKA1        = 0x15,
    EXCMASKA2        = 0x16,
    EXCMASKB0        = 0x18,
    EXCMASKB1        = 0x19,
    EXCMASKB2        = 0x1A,
    EXCBINDX0        = 0x1C,
    EXCBINDX1        = 0x1D,
    EXCBINDY0        = 0x1E,
    EXCBINDY1        = 0x1F,
    GPIOCTRL0        = 0x20,
    GPIOCTRL1        = 0x21,
    GPIOCTRL2        = 0x22,
    GPIOCTRL3        = 0x23,
    GPIOCTRL4        = 0x24,
    GPIOCTRL5        = 0x25,
    GPIOPOLARITY     = 0x26,
    GPIOCTRL         = 0x28,
    DPUCON           = 0x2A,
    DPUSTAT          = 0x2C,
    FREQCTRL         = 0x2E,
    FREQTUNE         = 0x2F,
    TXPOWER          = 0x30,
    TXCTRL           = 0x31,
    FSMSTAT0         = 0x32,
    FSMSTAT1         = 0x33,
    FIFOPCTRL        = 0x34,
    FSMCTRL          = 0x35,
    CCACTRL0         = 0x36,
    CCACTRL1         = 0x37,
    RSSI             = 0x38,
    RSSISTAT         = 0x39,
    RXFIRST          = 0x3C,
    RXFIFOCNT        = 0x3E,
    TXFIFOCNT        = 0x3F,
    CHIPID           = 0x40,
    VERSION          = 0x42,
    EXTCLOCK         = 0x44,
    MDMCTRL0         = 0x46,
    MDMCTRL1         = 0x47,
    FREQEST          = 0x48,
    RXCTRL           = 0x4A,
    FSCTRL           = 0x4C,
    FSCAL0           = 0x4E,
    FSCAL1           = 0x4F,
    FSCAL2           = 0x50,
    FSCAL3           = 0x51,
    AGCCTRL0         = 0x52,
    AGCCTRL1         = 0x53,
    AGCCTRL2         = 0x54,
    AGCCTRL3         = 0x55,
    ADCTEST0         = 0x56,
    ADCTEST1         = 0x57,
    ADCTEST2         = 0x58,
    MDMTEST0         = 0x5A,
    MDMTEST1         = 0x5B,
    DACTEST0         = 0x5C,
    DACTEST1         = 0x5D,
    ATEST            = 0x5E,
    DACTEST2         = 0x5F,
    PTEST0           = 0x60,
    PTEST1           = 0x61,
    RESERVED         = 0x62,
    DPUBIST          = 0x7A,
    ACTBIST          = 0x7C,
    RAMBIST          = 0x7E,
};

/**
 * Convert a CC2520Register to a byte, to send it through SPI
 * \param CC2520Register register
 * \return the byte value
 */
inline unsigned char toByte(CC2520Register x)
{
    return static_cast<unsigned char>(x);
}

/**
 * CC2520 commands
 */
enum class CC2520Command
{
    SNOP                 = 0x00,
    IBUFLD               = 0x02,
    SIBUFEX              = 0x03, //Command Strobe
    SSAMPLECCA           = 0x04, //Command Strobe
    SRES                 = 0x0f,
    MEMORY_MASK          = 0x0f,
    MEMORY_READ          = 0x10, // MEMRD
    MEMORY_WRITE         = 0x20, // MEMWR
    RXBUF                = 0x30,
    RXBUFCP              = 0x38,
    RXBUFMOV             = 0x32,
    TXBUF                = 0x3A,
    TXBUFCP              = 0x3E,
    RANDOM               = 0x3C,
    SXOSCON              = 0x40,
    STXCAL               = 0x41, //Command Strobe
    SRXON                = 0x42, //Command Strobe
    STXON                = 0x43, //Command Strobe
    STXONCCA             = 0x44, //Command Strobe
    SRFOFF               = 0x45, //Command Strobe
    SXOSCOFF             = 0x46, //Command Strobe
    SFLUSHRX             = 0x47, //Command Strobe
    SFLUSHTX             = 0x48, //Command Strobe
    SACK                 = 0x49, //Command Strobe
    SACKPEND             = 0x4A, //Command Strobe
    SNACK                = 0x4B, //Command Strobe
    SRXMASKBITSET        = 0x4C, //Command Strobe
    SRXMASKBITCLR        = 0x4D, //Command Strobe
    RXMASKAND            = 0x4E,
    RXMASKOR             = 0x4F,
    MEMCP                = 0x50,
    MEMCPR               = 0x52,
    MEMXCP               = 0x54,
    MEMXWR               = 0x56,
    BCLR                 = 0x58,
    BSET                 = 0x59,
    CTR_UCTR             = 0x60,
    CBCMAC               = 0x64,
    UCBCMAC              = 0x66,
    CCM                  = 0x68,
    UCCM                 = 0x6A,
    ECB                  = 0x70,
    ECBO                 = 0x72,
    ECBX                 = 0x74,
    INC                  = 0x78,
    ABORT                = 0x7F,
    REGISTER_READ        = 0x80,
    REGISTER_WRITE       = 0xC0,
};

/**
 * Convert a CC2520Command to a byte, to send it through SPI
 * \param CC2520Command register
 * \return the byte value
 */
inline unsigned char toByte(CC2520Command x)
{
    return static_cast<unsigned char>(x);
}

/**
 * Status bits
 */
using CC2520StatusBitmask=unsigned char;
namespace CC2520Status
{
    const CC2520StatusBitmask RX_ACTIVE    = 0x01; // RX is active
    const CC2520StatusBitmask TX_ACTIVE    = 0x02; // TX is active
    const CC2520StatusBitmask DPU_L_ACTIVE = 0x04; // low  priority DPU is active
    const CC2520StatusBitmask DPU_H_ACTIVE = 0x08; // high  priority DPU is active
    const CC2520StatusBitmask EXC_B        = 0x10; // at least one exception on channel B is raise
    const CC2520StatusBitmask EXC_A        = 0x20; // at least one exception on channel A is raise
    const CC2520StatusBitmask RSSI_VALID   = 0x40; // RSSI value is valid
    const CC2520StatusBitmask XOSC         = 0x80; // XOSC is stable and running
};

/**
 * List of all the exceptions
 */
using CC2520ExceptionBitmask=unsigned int;
namespace CC2520Exception
{
    const CC2520ExceptionBitmask RF_IDL           = (1<<0);
    const CC2520ExceptionBitmask TX_FRM_DONE      = (1<<1);
    const CC2520ExceptionBitmask TX_ACK_DONE      = (1<<2);
    const CC2520ExceptionBitmask TX_UNDERFLOW     = (1<<3);
    const CC2520ExceptionBitmask TX_OVERFLOW      = (1<<4);
    const CC2520ExceptionBitmask RX_UNDERFLOW     = (1<<5);
    const CC2520ExceptionBitmask RX_OVERFLOW      = (1<<6);
    const CC2520ExceptionBitmask RXENABLE_ZERO    = (1<<7);
    const CC2520ExceptionBitmask RX_FRM_DONE      = (1<<8);
    const CC2520ExceptionBitmask RX_FRM_ACCEPETED = (1<<9);
    const CC2520ExceptionBitmask SR_MATCH_DONE    = (1<<10);
    const CC2520ExceptionBitmask SR_MATCH_FOUND   = (1<<11);
    const CC2520ExceptionBitmask FIFOP            = (1<<12);
    const CC2520ExceptionBitmask SFD              = (1<<13);
    const CC2520ExceptionBitmask DPU_DONE_L       = (1<<14);
    const CC2520ExceptionBitmask DPU_DONE_H       = (1<<15);
    const CC2520ExceptionBitmask MEMADDR_ERROR    = (1<<16);
    const CC2520ExceptionBitmask USAGE_ERROR      = (1<<17);
    const CC2520ExceptionBitmask OPERAND_ERROR    = (1<<18);
    const CC2520ExceptionBitmask SPI_ERROR        = (1<<19);
    const CC2520ExceptionBitmask RF_NO_LOCK       = (1<<20);
    const CC2520ExceptionBitmask RX_FRM_ABORTED   = (1<<21);
    const CC2520ExceptionBitmask RXBUFMOV_TIMEOUT = (1<<22);
}

/**
 * Internal state machine, or at least the part we are using
 */
enum class CC2520State
{
    IDLE,
    DEEPSLEEP,
    RX
};

/**
 * Transmission power
 */
enum class CC2520TxPower
{
    P_5 = 0xf7,  ///< +5dBm
    P_3 = 0xf2,  ///< +3dBm
    P_2 = 0xab,  ///< +2dBm
    P_1 = 0x13,  ///< +1dBm
    P_0 = 0x32,  ///< +0dBm
    P_m2 = 0x81, ///< -2dBm
    P_m7 = 0x2c, ///< -7dBm
    P_m18 = 0x03 ///< -18dBm
};

/**
 * Convert a CC2520TxPower to a byte, to send it through SPI
 * \param CC2520TxPower register
 * \return the byte value
 */
inline unsigned char toByte(CC2520TxPower x)
{
    return static_cast<unsigned char>(x);
}

} //namespace miosix

#endif //CC2520_CONSTANTS_H
