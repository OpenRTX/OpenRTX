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

#if !defined(P25NID_H)
#define  P25NID_H

class CP25NID {
public:
	CP25NID(unsigned int nac);
	~CP25NID();

	bool decode(const unsigned char* data);

	unsigned char getDUID() const;

	void encode(unsigned char* data, unsigned char duid) const;

private:
	unsigned char  m_duid;
	unsigned char* m_hdr;
	unsigned char* m_ldu1;
	unsigned char* m_ldu2;
	unsigned char* m_termlc;
	unsigned char* m_term;
};

#endif
