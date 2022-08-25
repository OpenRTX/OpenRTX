/*
 * Integration of FatFs filesystem module in Miosix by Terraneo Federico
 * based on original files diskio.c and mmc.c by ChaN
 */

#include "diskio.h"
#include "filesystem/ioctl.h"
#include "config/miosix_settings.h"

#ifdef WITH_FILESYSTEM

using namespace miosix;

// #ifdef __cplusplus
// extern "C" {
// #endif

///**
// * \internal
// * Initializes drive.
// */
//DSTATUS disk_initialize (
//    intrusive_ref_ptr<FileBase> pdrv		/* Physical drive nmuber (0..) */
//)
//{
//    if(Disk::isAvailable()==false) return STA_NODISK;
//    Disk::init();
//    if(Disk::isInitialized()) return RES_OK;
//    else return STA_NOINIT;
//}

///**
// * \internal
// * Return status of drive.
// */
//DSTATUS disk_status (
//    intrusive_ref_ptr<FileBase> pdrv		/* Physical drive nmuber (0..) */
//)
//{
//    if(Disk::isInitialized()) return RES_OK;
//    else return STA_NOINIT;
//}

/**
 * \internal
 * Read one or more sectors from drive
 */
DRESULT disk_read (
    intrusive_ref_ptr<FileBase> pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,           /* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..255) */
)
{
    if(pdrv->lseek(static_cast<off_t>(sector)*512,SEEK_SET)<0) return RES_ERROR;
    if(pdrv->read(buff,count*512)!=static_cast<ssize_t>(count)*512) return RES_ERROR;
    return RES_OK;
}

/**
 * \internal
 * Write one or more sectors to drive
 */
DRESULT disk_write (
    intrusive_ref_ptr<FileBase> pdrv,		/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count		/* Number of sectors to write (1..255) */
)
{
    if(pdrv->lseek(static_cast<off_t>(sector)*512,SEEK_SET)<0) return RES_ERROR;
    if(pdrv->write(buff,count*512)!=static_cast<ssize_t>(count)*512) return RES_ERROR;
    return RES_OK;
}

/**
 * \internal
 * To perform disk functions other thar read/write
 */
DRESULT disk_ioctl (
    intrusive_ref_ptr<FileBase> pdrv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    switch(ctrl)
    {
        case CTRL_SYNC:
            if(pdrv->ioctl(IOCTL_SYNC,0)==0) return RES_OK; else return RES_ERROR;
        case GET_SECTOR_COUNT:
            return RES_ERROR; //unimplemented, so f_mkfs() does not work
        case GET_BLOCK_SIZE:
            return RES_ERROR; //unimplemented, so f_mkfs() does not work
        default:
            return RES_PARERR;
    }
}

/**
 * \internal
 * Return current time, used to save file creation time
 */
 DWORD get_fattime()
 {
     return 0x210000;//TODO: this stub just returns date 01/01/1980 0.00.00
 }

// #ifdef __cplusplus
// }
// #endif

#endif //WITH_FILESYSTEM
