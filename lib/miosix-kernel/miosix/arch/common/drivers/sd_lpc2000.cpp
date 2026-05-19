/*
 * Integration in Miosix by Terraneo Federico.
 * Based on code by Martin Thomas to initialize SD cards from LPC2000
 */

#include "sd_lpc2000.h"
#include "interfaces/bsp.h"
#include "LPC213x.h"
#include "board_settings.h" //For sdVoltage
#include <cstdio>
#include <errno.h>

//Note: enabling debugging might cause deadlock when using sleep() or reboot()
//The bug won't be fixed because debugging is only useful for driver development
///\internal Debug macro, for normal conditions
//#define DBG iprintf
#define DBG(x,...) do {} while(0)
///\internal Debug macro, for errors only
//#define DBGERR iprintf
#define DBGERR(x,...) do {} while(0)

namespace miosix {

///\internal Type of card (1<<0)=MMC (1<<1)=SDv1 (1<<2)=SDv2 (1<<2)|(1<<3)=SDHC
static unsigned char cardType=0;

/*
 * Definitions for MMC/SDC command.
 * A command has the following format:
 * - 1  bit @ 0 (start bit)
 * - 1  bit @ 1 (transmission bit)
 * - 6  bit which identify command index (CMD0..CMD63)
 * - 32 bit command argument
 * - 7 bit CRC
 * - 1 bit @ 1 (end bit)
 * In addition, ACMDxx are the sequence of two commands, CMD55 and CMDxx
 * These constants have the following meaninig:
 * - bit #7 @ 1 indicates that it is an ACMD. send_cmd() will send CMD55, then
 *   clear this bit and send the command with this bit @ 0 (which is start bit)
 * - bit #6 always @ 1, because it is the transmission bit
 * - remaining 6 bit represent command index
 */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define	ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define CMD13   (0x40+13)   /* SEND_STATUS */
#define ACMD13	(0xC0+13)	/* SD_STATUS (SDC) */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0xC0+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD42   (0x40+42)   /* LOCK_UNLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */

// SSPCR0  Bit-Definitions
#define CPOL    6
#define CPHA    7
// SSPCR1  Bit-Defintions
#define SSE     1
#define MS      2
#define SCR     8
// SSPSR  Bit-Definitions
#define TNF     1
#define RNE     2
#define BSY	    4

#define SPI_SCK_PIN    17   // SCK    P0.17  out
#define SPI_MISO_PIN   18   // MISO   P0.18  in
#define SPI_MOSI_PIN   19   // MOSI   P0.19  out
#define SPI_SS_PIN     20   // CS     p0.20  out

#define SPI_SCK_FUNCBIT   2
#define SPI_MISO_FUNCBIT  4
#define SPI_MOSI_FUNCBIT  6
#define SPI_SS_FUNCBIT    8

///\internal Maximum speed 14745600/2=7372800
#define SPI_PRESCALE_MIN  2

///\internal Select/unselect card
#define CS_LOW()    IOCLR0 = (1<<SPI_SS_PIN)
#define CS_HIGH()   IOSET0 = (1<<SPI_SS_PIN)

//Function prototypes
static unsigned char send_cmd(unsigned char cmd, unsigned int arg);

/**
 * \internal
 * Initialize SPI
 */
static void spi_1_init()
{
    unsigned char incoming;
    PCONP|=(1<<10);//Enable SPI1
    // setup GPIO
    IODIR0 |= (1<<SPI_SCK_PIN)|(1<<SPI_MOSI_PIN)|(1<<SPI_SS_PIN);
    IODIR0 &= ~(1<<SPI_MISO_PIN);
    // Unselect card
    CS_HIGH();
    // Set GPIO mode
    PINSEL1 &= ~( (3<<SPI_SCK_FUNCBIT) | (3<<SPI_MISO_FUNCBIT) |
            (3<<SPI_MOSI_FUNCBIT) | (3<<SPI_SS_FUNCBIT) );

    // setup Pin-Functions - keep automatic CS disabled during init
    PINSEL1 |= ( (2<<SPI_SCK_FUNCBIT) | (2<<SPI_MISO_FUNCBIT) |
            (2<<SPI_MOSI_FUNCBIT) );
    // enable SPI-Master - slowest speed
    SSPCR0 = ((8-1)<<0) | (0<<CPOL) | (0x20<<SCR);
    SSPCR1 = (1<<SSE);
    // low speed during init
    SSPCPSR=254;

    // Send 20 spi commands with card not selected
    for(int i=0;i<20;i++)
    {
        while( !(SSPSR & (1<<TNF)) ) ; //Wait
        SSPDR=0xff;
        while( !(SSPSR & (1<<RNE)) ) ; //Wait
        incoming=SSPDR;
        (void)incoming;
    }
}

/**
 * \internal
 * Send and receive one byte through SPI
 */
static unsigned char spi_1_send(unsigned char outgoing)
{
    while(!(SSPSR & (1<<TNF))) ;
    SSPDR=outgoing;
    while(!(SSPSR & (1<<RNE))) ;
    return SSPDR;
}

/**
 * \internal
 * Used for debugging, print 8 bit error code from SD card
 */
static void print_error_code(unsigned char value)
{
    switch(value)
    {
        case 0x40:
            DBGERR("Parameter error\n");
            break;
        case 0x20:
            DBGERR("Address error\n");
            break;
        case 0x10:
            DBGERR("Erase sequence error\n");
            break;
        case 0x08:
            DBGERR("CRC error\n");
            break;
        case 0x04:
            DBGERR("Illegal command\n");
            break;
        case 0x02:
            DBGERR("Erase reset\n");
            break;
        case 0x01:
            DBGERR("Card is initializing\n");
            break;
        default:
            DBGERR("Unknown error 0x%x\n",value);
            break;
    }
}

/**
 * \internal
 * Return 1 if card is OK, otherwise print 16 bit error code from SD card
 */
static char sd_status()
{
    short value=send_cmd(CMD13,0);
    value<<=8;
    value|=spi_1_send(0xff);

    switch(value)
    {
        case 0x0000:
            return 1;
        case 0x0001:
            DBGERR("Card is Locked\n");
            /*//Try to fix the problem by erasing all the SD card.
            char e=send_cmd(CMD16,1);
            if(e!=0) print_error_code(e);
            e=send_cmd(CMD42,0);
            if(e!=0) print_error_code(e);
            spi_1_send(0xfe); // Start block
            spi_1_send(1<<3); //Erase all disk command
            spi_1_send(0xff); // Checksum part 1
            spi_1_send(0xff); // Checksum part 2
            e=spi_1_send(0xff);
            iprintf("Reached here 0x%x\n",e);//Should return 0xe5
            while(spi_1_send(0xff)!=0xff);*/
            break;
        case 0x0002:
            DBGERR("WP erase skip or lock/unlock cmd failed\n");
            break;
        case 0x0004:
            DBGERR("General error\n");
            break;
        case 0x0008:
            DBGERR("Internal card controller error\n");
            break;
        case 0x0010:
            DBGERR("Card ECC failed\n");
            break;
        case 0x0020:
            DBGERR("Write protect violation\n");
            break;
        case 0x0040:
            DBGERR("Invalid selection for erase\n");
            break;
        case 0x0080:
            DBGERR("Out of range or CSD overwrite\n");
            break;
        default:
            if(value>0x00FF)
                print_error_code((unsigned char)(value>>8));
            else
                DBGERR("Unknown error 0x%x\n",value);
            break;
    }
    return -1;
}

/**
 * \internal
 * Wait until card is ready
 */
static unsigned char wait_ready()
{
    unsigned char result;
    spi_1_send(0xff);
    for(int i=0;i<460800;i++)//Timeout ~500ms
    {
        result=spi_1_send(0xff);
        if(result==0xff) return 0xff;
        if(i%500==0) DBG("*\n");
    }
    DBGERR("Error: wait_ready()\n");
    return result;
}

/**
 * \internal
 * Send a command to the SD card
 * \param cmd one among the #define'd commands
 * \param arg command's argument
 * \return command's r1 response. If command returns a longer response, the user
 * can continue reading the response with spi_1_send(0xff)
 */
static unsigned char send_cmd(unsigned char cmd, unsigned int arg)
{
    unsigned char n, res;
    if(cmd & 0x80)
    {	// ACMD<n> is the command sequence of CMD55-CMD<n>
        cmd&=0x7f;
        res=send_cmd(CMD55,0);
        if(res>1) return res;
    }

    // Select the card and wait for ready
    CS_HIGH();
    CS_LOW();

    if(cmd==CMD0)
    {
        //wait_ready on CMD0 seems to cause infinite loop
        spi_1_send(0xff);
    } else {
        if(wait_ready()!=0xff) return 0xff;
    }
    // Send command
    spi_1_send(cmd);			            // Start + Command index
    spi_1_send((unsigned char)(arg >> 24));	// Argument[31..24]
    spi_1_send((unsigned char)(arg >> 16));	// Argument[23..16]
    spi_1_send((unsigned char)(arg >> 8));	// Argument[15..8]
    spi_1_send((unsigned char)arg);		    // Argument[7..0]
    n=0x01;                // Dummy CRC + Stop
    if (cmd==CMD0) n=0x95; // Valid CRC for CMD0(0)
    if (cmd==CMD8) n=0x87; // Valid CRC for CMD8(0x1AA)
    spi_1_send(n);
    // Receive response
    if (cmd==CMD12) spi_1_send(0xff);   // Skip a stuff byte when stop reading
    n=10; // Wait response, try 10 times
    do
        res=spi_1_send(0xff);
    while ((res & 0x80) && --n);
    return res; // Return with the response value
}

/**
 * \internal
 * Receive a data packet from the SD card
 * \param buf data buffer to store received data
 * \param byte count (must be multiple of 4)
 * \return true on success, false on failure
 */
static bool rx_datablock(unsigned char *buf, unsigned int btr)
{
    unsigned char token;
    for(int i=0;i<0xffff;i++)
    {
        token=spi_1_send(0xff);
        if(token!=0xff) break;
    }
    if(token!=0xfe) return false; // If not valid data token, retutn error

    do { // Receive the data block into buffer
        *buf=spi_1_send(0xff); buf++;
        *buf=spi_1_send(0xff); buf++;
        *buf=spi_1_send(0xff); buf++;
        *buf=spi_1_send(0xff); buf++;
    } while(btr-=4);
    spi_1_send(0xff); // Discard CRC
    spi_1_send(0xff);

    return true; // Return success
}

/**
 * \internal
 * Send a data packet to the SD card
 * \param buf 512 byte data block to be transmitted
 * \param token data start/stop token
 * \return true on success, false on failure
 */
static bool tx_datablock (const unsigned char *buf, unsigned char token)
{
    unsigned char resp;
    if(wait_ready()!=0xff) return false;

    spi_1_send(token);      // Xmit data token
    if (token!=0xfd)
    {                       // Is data token
        for(int i=0;i<256;i++)
        {                   // Xmit the 512 byte data block
            spi_1_send(*buf); buf++;
            spi_1_send(*buf); buf++;
        }
        spi_1_send(0xff);		// CRC (Dummy)
        spi_1_send(0xff);
        resp=spi_1_send(0xff);          // Receive data response
        if((resp & 0x1f)!=0x05) 	// If not accepted, return error
        return false;
    }
    return true;
}

//
// class SPISDDriver
//

intrusive_ref_ptr<SPISDDriver> SPISDDriver::instance()
{
    static FastMutex m;
    static intrusive_ref_ptr<SPISDDriver> instance;
    Lock<FastMutex> l(m);
    if(!instance) instance=new SPISDDriver();
    return instance;
}

ssize_t SPISDDriver::readBlock(void* buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    unsigned char *buf=reinterpret_cast<unsigned char*>(buffer);
    Lock<FastMutex> l(mutex);
    DBG("SPISDDriver::readBlock(): nSectors=%d\n",nSectors);
    if(!(cardType & 8)) lba*=512;	// Convert to byte address if needed
    unsigned char result;
    if(nSectors==1)
    {           // Single block read
        result=send_cmd(CMD17,lba);	// READ_SINGLE_BLOCK
        if(result!=0)
        {
            print_error_code(result);
            CS_HIGH();
            return -EBADF;
        }
        if(rx_datablock(buf,512)==false)
        {
            DBGERR("Block read error\n");
            CS_HIGH();
            return -EBADF;
        }
    } else {	// Multiple block read
        //DBGERR("Mbr\n");//debug only
        result=send_cmd(CMD18,lba);   // READ_MULTIPLE_BLOCK
        if(result!=0)
        {
            print_error_code(result);
            CS_HIGH();
            return -EBADF;
        }
        do {
            if(!rx_datablock(buf,512)) break;
            buf+=512;
        } while(--nSectors);
        send_cmd(CMD12,0);			// STOP_TRANSMISSION
        if(nSectors!=0)
        {
            DBGERR("Multiple block read error\n");
            CS_HIGH();
            return -EBADF;
        }
    }
    CS_HIGH();
    return size;
}

ssize_t SPISDDriver::writeBlock(const void* buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    const unsigned char *buf=reinterpret_cast<const unsigned char*>(buffer);
    Lock<FastMutex> l(mutex);
    DBG("SPISDDriver::writeBlock(): nSectors=%d\n",nSectors);
    if(!(cardType & 8)) lba*=512;	// Convert to byte address if needed
    unsigned char result;
    if(nSectors==1)
    {           // Single block write
        result=send_cmd(CMD24,lba);         // WRITE_BLOCK
        if(result!=0)
        {
            print_error_code(result);
            CS_HIGH();
            return -EBADF;
        }
        if(tx_datablock(buf,0xfe)==false)    // WRITE_BLOCK
        {
            DBGERR("Block write error\n");
            CS_HIGH();
            return -EBADF;
        }
    } else {	// Multiple block write
        //DBGERR("Mbw\n");//debug only
        if(cardType & 6) send_cmd(ACMD23,nSectors);//Only if it is SD card
        result=send_cmd(CMD25,lba);          // WRITE_MULTIPLE_BLOCK
        if(result!=0)
        {
            print_error_code(result);
            CS_HIGH();
            return -EBADF;
        }
        do {
            if(!tx_datablock(buf,0xfc)) break;
            buf+=512;
        } while(--nSectors);
        if(!tx_datablock(0,0xfd))    // STOP_TRAN token
        {
            DBGERR("Multiple block write error\n");
            CS_HIGH();
            return -EBADF;
        }
    }
    CS_HIGH();
    return size;
}

int SPISDDriver::ioctl(int cmd, void* arg)
{
    DBG("SPISDDriver::ioctl()\n");
    if(cmd!=IOCTL_SYNC) return -ENOTTY;
    Lock<FastMutex> l(mutex);
    CS_LOW();
    unsigned char result=wait_ready();
    CS_HIGH();
    if(result==0xff) return 0;
    else return -EFAULT;
}

SPISDDriver::SPISDDriver() : Device(Device::BLOCK)
{
    const int MAX_RETRY=20;//Maximum command retry before failing
    spi_1_init(); /* init at low speed */
    unsigned char resp;
    int i;
    for(i=0;i<MAX_RETRY;i++)
    {
        resp=send_cmd(CMD0,0);
        if(resp==1) break;
    }
    DBG("CMD0 required %d commands\n",i+1);
    
    if(resp!=1)
    {
        print_error_code(resp);
        DBGERR("Init failed\n");
        CS_HIGH();
        return; //Error
    }
    unsigned char n, cmd, ty=0, ocr[4];
    // Enter Idle state
    if(send_cmd(CMD8,0x1aa)==1)
    {   /* SDHC */
        for(n=0;n<4;n++) ocr[n]=spi_1_send(0xff);// Get return value of R7 resp
        if((ocr[2]==0x01)&&(ocr[3]==0xaa))
        {   // The card can work at vdd range of 2.7-3.6V
            for(i=0;i<MAX_RETRY;i++)
            {
                resp=send_cmd(ACMD41, 1UL << 30);
                if(resp==0)
                {
                    if(send_cmd(CMD58,0)==0)
                    {   // Check CCS bit in the OCR
                        for(n=0;n<4;n++) ocr[n]=spi_1_send(0xff);
                        if(ocr[0] & 0x40)
                        {
                            ty=12;
                            DBG("SDHC, block addressing supported\n");
                        } else {
                            ty=4;
                            DBG("SDHC, no block addressing\n");
                        }
                    } else DBGERR("CMD58 failed\n");

                    break; //Exit from for
                } else print_error_code(resp);
            }
            DBG("ACMD41 required %d commands\n",i+1);
        } else DBGERR("CMD8 failed\n");
    } else { /* SDSC or MMC */
        if(send_cmd(ACMD41,0)<=1)
        {
            ty=2;
            cmd=ACMD41; /* SDSC */
            DBG("SD card\n");
        } else {
            ty=1;
            cmd=CMD1;   /* MMC */
            DBG("MMC card\n");
        }
        for(i=0;i<MAX_RETRY;i++)
        {
            resp=send_cmd(cmd,0);
            if(resp==0)
            {
                if(send_cmd(CMD16,512)!=0)
                {
                    ty=0;
                    DBGERR("CMD16 failed\n");
                }
                break; //Exit from for
            } else print_error_code(resp);
        }
        DBG("CMD required %d commands\n",i+1);
    }

    if(ty==0)
    {
        CS_HIGH();
        return; //Error
    }
    cardType=ty;

    if(sd_status()<0)
    {
        DBGERR("Status error\n");
        CS_HIGH();
        return; //Error
    }

    CS_HIGH();
    //Configure the SPI interface to use the 7.4MHz high speed mode
    SSPCR0=((8-1)<<0) | (0<<CPOL);
    SSPCPSR=SPI_PRESCALE_MIN;

    DBG("Init done...\n");
}

} //namespace miosix
