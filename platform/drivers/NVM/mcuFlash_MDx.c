/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include "mcuFlash.h"

inline bool unlock()
{
	// Flash already unlocked
	if((FLASH->CR & FLASH_CR_LOCK) == 0)
	{
		return true;
	}

	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;

	// Succesful unlock
	if((FLASH->CR & FLASH_CR_LOCK) == 0)
	{
		return true;
	}

	return false;
}

bool mcuFlash_eraseSector(const uint8_t secNum)
{
	if(secNum > 11) return false;
	if(unlock() == false) return false;

	// Flash busy, wait until previous operation finishes
	while((FLASH->SR & FLASH_SR_BSY) != 0) ;

	FLASH->CR |= FLASH_CR_PSIZE_1	// 32-bit program parallelism
	          |  (secNum << 3)		// Sector number
	          |  FLASH_CR_SER		// Sector erase
	          |  FLASH_CR_STRT;     // Start erase

	// Wait until erase ends
	while((FLASH->SR & FLASH_SR_BSY) != 0) ;

	return true;
}

void mcuFlash_write(const uint32_t address, const void *data, const size_t len)
{
	if((data == NULL) || (len == 0)) return;

	// Flash busy, wait until previous operation finishes
	while((FLASH->SR & FLASH_SR_BSY) != 0) ;

	// Request programming with 8-bit parallelism
	FLASH->CR = FLASH_CR_PG;

	const uint8_t *buf = ((uint8_t *) data);
	const uint8_t *mem = ((uint8_t *) address);
	for(size_t i = 0; i < len; i++)
	{
		*mem = buf[i];
		mem++;
	}

	// Wait until the end of the write operation
	while((FLASH->SR & FLASH_SR_BSY) != 0) ;
}