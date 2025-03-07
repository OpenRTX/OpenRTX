
/*******************************************************************************
This header file has been taken from:
"A Collection of Useful C++ Classes for Digital Signal Processing"
By Vinnie Falco 
Bernd Porr adapted it for Linux and turned it into a filter using
fixed point arithmetic.
--------------------------------------------------------------------------------
License: MIT License (http://www.opensource.org/licenses/mit-license.php)
Copyright (c) 2009 by Vinnie Falco
Copyright (C) 2013-2017, Bernd Porr, mail@berndporr.me.uk
Copyright (C) 2020 , Mario Molitor , mario_molitor@web.de
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*******************************************************************************/

// based on https://raw.githubusercontent.com/berndporr/iir_fixed_point/master/DirectFormI.h

//#if defined(MODE_FM)

#include <cstdint>

#ifndef DIRECTFORMI_H_
#define DIRECTFORMI_H_
class CFMDirectFormI
{
public:

	// convenience function which takes the a0 argument but ignores it!
	CFMDirectFormI(const q31_t b0, const q31_t b1, const q31_t b2, const q31_t, const q31_t a1, const q31_t a2)
	{
		// FIR coefficients
		c_b0 = b0;
		c_b1 = b1;
		c_b2 = b2;
		// IIR coefficients
		c_a1 = a1;
		c_a2 = a2;
		reset();
	}

	CFMDirectFormI(const CFMDirectFormI &my)
	{
		// delay line
		m_x2 = my.m_x2; // x[n-2]
		m_y2 = my.m_y2; // y[n-2]
		m_x1 = my.m_x1; // x[n-1]
		m_y1 = my.m_y1; // y[n-1]

		// coefficients
		c_b0 = my.c_b0;
		c_b1 = my.c_b1;
		c_b2 = my.c_b2; // FIR
		c_a1 = my.c_a1;
		c_a2 = my.c_a2; // IIR
	}

	void reset()
	{
		m_x1 = 0;
		m_x2 = 0;
		m_y1 = 0;
		m_y2 = 0;
	}

	// filtering operation: one sample in and one out
	inline q15_t filter(const q15_t in)
	{
		// calculate the output
		q31_t out_upscaled = c_b0 * in + c_b1 * m_x1 + c_b2 * m_x2 - c_a1 * m_y1 - c_a2 * m_y2;

		q15_t out = __SSAT(out_upscaled >> 15, 15);

		// update the delay lines
		m_x2 = m_x1;
		m_y2 = m_y1;
		m_x1 = in;
		m_y1 = out;

		return out;
	}

private:
	// delay line
	q31_t m_x2; // x[n-2]
	q31_t m_y2; // y[n-2]
	q31_t m_x1; // x[n-1]
	q31_t m_y1; // y[n-1]

	// coefficients
	q31_t c_b0,c_b1,c_b2; // FIR
	q31_t c_a1,c_a2; // IIR
};

#endif /* DIRECTFORMI_H */

//#endif

