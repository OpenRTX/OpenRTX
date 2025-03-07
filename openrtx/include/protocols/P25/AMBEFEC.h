/*
 *   Copyright (C) 2010,2014,2016 by Jonathan Naylor G4KLX
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

#if !defined(AMBEFEC_H)
#define	AMBEFEC_H

class CAMBEFEC {
public:
	CAMBEFEC();
	~CAMBEFEC();

	unsigned int regenerateDMR(unsigned char* bytes) const;

	unsigned int regenerateDStar(unsigned char* bytes) const;

	unsigned int regenerateYSFDN(unsigned char* bytes) const;

	unsigned int regenerateIMBE(unsigned char* bytes) const;

private:
	unsigned int regenerate(unsigned int& a, unsigned int& b, unsigned int& c, bool b23) const;
};

#endif
