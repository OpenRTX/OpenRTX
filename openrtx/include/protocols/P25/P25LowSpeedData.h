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

#if !defined(P25LowSpeedData_H)
#define	P25LowSpeedData_H

class CP25LowSpeedData {
public:
	CP25LowSpeedData();
	~CP25LowSpeedData();

	void process(unsigned char* data);

	void encode(unsigned char* data) const;

	unsigned char getLSD1() const;
	void setLSD1(unsigned char lsd1);

	unsigned char getLSD2() const;
	void setLSD2(unsigned char lsd2);

private:
	unsigned char m_lsd1;
	unsigned char m_lsd2;

	unsigned char encode(const unsigned char in) const;
};

#endif
