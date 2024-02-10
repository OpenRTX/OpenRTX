/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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

#ifndef MCP4551_H
#define MCP4551_H

#include <stdint.h>
#include <stdbool.h>
#include <datatypes.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

// Common WIPER values
#define MCP4551_WIPER_MID	0x080
#define MCP4551_WIPER_A		0x100
#define MCP4551_WIPER_B		0x000

// Command definitions (sent to WIPER register)
#define MCP4551_CMD_WRITE	0x00
#define MCP4551_CMD_INC		0x04
#define MCP4551_CMD_DEC		0x08
#define MCP4551_CMD_READ	0x0C

error_t mcp4551_init(uint8_t addr);
error_t mcp4551_setWiper(uint8_t devAddr, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* MCP4551_H */
