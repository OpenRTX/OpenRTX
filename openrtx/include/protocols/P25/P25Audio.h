/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#if !defined(P25Audio_H)
#define  P25Audio_H

#include "AMBEFEC.h"

class CP25Audio {
public:
	CP25Audio();
	~CP25Audio();

	unsigned int process(unsigned char* data);

	void encode(unsigned char* data, const unsigned char* imbe, unsigned int n);

	void decode(const unsigned char* data, unsigned char* imbe, unsigned int n);

private:
	CAMBEFEC m_fec;
};

#endif
