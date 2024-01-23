/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <interfaces/nvmem.h>

#include "flash_zephyr.h"

ZEPHYR_FLASH_DEVICE_DEFINE(extFlash, flash);

static const struct nvmArea areas[] =
{
    {
        .name       = "External flash",
        .dev        = &extFlash,
        .startAddr  = FIXED_PARTITION_OFFSET(storage_partition),
        .size       = FIXED_PARTITION_SIZE(storage_partition),
        .partitions = NULL
    }
};


void nvm_init()
{

}

void nvm_terminate()
{

}

size_t nvm_getMemoryAreas(const struct nvmArea **list)
{
    *list = &areas[0];

    return (sizeof(areas) / sizeof(struct nvmArea));
}

void nvm_readCalibData(void *buf)
{
    (void) buf;
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    (void) channel;

	int res;
	const struct flash_area *my_area;
	int err;	
	err = flash_area_open(FIXED_PARTITION_ID(storage_partition)+0x1000, &my_area);
	if (err != 0) 
    {
        return -1;
    } else 
    {
        res=flash_area_read(my_area,FIXED_PARTITION_OFFSET(storage_partition)+0x1000,channel,sizeof(channel_t));
		flash_area_close(my_area);
        return 0;
    }
}

int nvm_readSettings(settings_t *settings)
{
    (void) settings;

	int res,err;
	const struct flash_area *my_area;

	err = flash_area_open(FIXED_PARTITION_ID(storage_partition), &my_area);
	if (err != 0) 
    {
        return -1;
    } else 
    {
        res=flash_area_read(my_area,FIXED_PARTITION_OFFSET(storage_partition),settings,sizeof(settings_t));
		flash_area_close(my_area);
        return 0;
    }
}

int nvm_writeSettings(const settings_t *settings)
{
    (void) settings;
	int res,err;
	const struct flash_area *my_area;

	err = flash_area_open(FIXED_PARTITION_ID(storage_partition), &my_area);
	if (err != 0)
     {
        res=flash_area_erase(my_area,FIXED_PARTITION_OFFSET(storage_partition),0x1000); // flash minimum erase size=4+96
        if (res == 0){
		    flash_area_write(my_area,FIXED_PARTITION_OFFSET(storage_partition),settings,sizeof(settings_t));
            return 0;
        }else
        {
            flash_area_close(my_area);    
            return -1;
        }
	} else 
    {	
        flash_area_close(my_area);    
        return -1;
    }
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    (void) settings;
    (void) vfo;
	int res,err;
	const struct flash_area *my_area;
	err = flash_area_open(FIXED_PARTITION_ID(storage_partition), &my_area);
	if (err == 0)
     {
        res=flash_area_erase(my_area,FIXED_PARTITION_OFFSET(storage_partition),0x2000); // flash minimum erase size=4+96
        if (res == 0){
		    res=flash_area_write(my_area,FIXED_PARTITION_OFFSET(storage_partition),settings,sizeof(settings_t));
            err=flash_area_write(my_area,FIXED_PARTITION_OFFSET(storage_partition)+0x1000,vfo,sizeof(channel_t));
            return 0;
        }else
        {
            flash_area_close(my_area);    
            return -1;
        }
	} else 
    {	
        flash_area_close(my_area);    
        return -1;
    }
}
