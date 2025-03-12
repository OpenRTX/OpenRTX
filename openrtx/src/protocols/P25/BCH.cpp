/*
* File:    bch3.c
* Title:   Encoder/decoder for binary BCH codes in C (Version 3.1)
* Author:  Robert Morelos-Zaragoza
* Date:    August 1994
* Revised: June 13, 1997
*
* ===============  Encoder/Decoder for binary BCH codes in C =================
*
* Version 1:   Original program. The user provides the generator polynomial
*              of the code (cumbersome!).
* Version 2:   Computes the generator polynomial of the code.
* Version 3:   No need to input the coefficients of a primitive polynomial of
*              degree m, used to construct the Galois Field GF(2**m). The
*              program now works for any binary BCH code of length such that:
*              2**(m-1) - 1 < length <= 2**m - 1
*
* Note:        You may have to change the size of the arrays to make it work.
*
* The encoding and decoding methods used in this program are based on the
* book "Error Control Coding: Fundamentals and Applications", by Lin and
* Costello, Prentice Hall, 1983.
*
* Thanks to Patrick Boyle (pboyle@era.com) for his observation that 'bch2.c'
* did not work for lengths other than 2**m-1 which led to this new version.
* Portions of this program are from 'rs.c', a Reed-Solomon encoder/decoder
* in C, written by Simon Rockliff (simon@augean.ua.oz.au) on 21/9/89. The
* previous version of the BCH encoder/decoder in C, 'bch2.c', was written by
* Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu) on 5/19/92.
*
* NOTE:
*          The author is not responsible for any malfunctioning of
*          this program, nor for any damage caused by it. Please include the
*          original program along with these comments in any redistribution.
*
*  For more information, suggestions, or other ideas on implementing error
*  correcting codes, please contact me at:
*
*                           Robert Morelos-Zaragoza
*                           5120 Woodway, Suite 7036
*                           Houston, Texas 77056
*
*                    email: r.morelos-zaragoza@ieee.org
*
* COPYRIGHT NOTICE: This computer program is free for non-commercial purposes.
* You may implement this program for any non-commercial application. You may
* also implement this program for commercial purposes, provided that you
* obtain my written permission. Any modification of this program is covered
* by this copyright.
*
* == Copyright (c) 1994-7,  Robert Morelos-Zaragoza. All rights reserved.  ==
*
* m = order of the Galois field GF(2**m)
* n = 2**m - 1 = size of the multiplicative group of GF(2**m)
* length = length of the BCH code
* t = error correcting capability (max. no. of errors the code corrects)
* d = 2*t + 1 = designed min. distance = no. of consecutive roots of g(x) + 1
* k = n - deg(g(x)) = dimension (no. of information bits/codeword) of the code
* p[] = coefficients of a primitive polynomial used to generate GF(2**m)
* g[] = coefficients of the generator polynomial, g(x)
* alpha_to [] = log table of GF(2**m)
* index_of[] = antilog table of GF(2**m)
* data[] = information bits = coefficients of data polynomial, i(x)
* bb[] = coefficients of redundancy polynomial x^(length-k) i(x) modulo g(x)
* numerr = number of errors
* errpos[] = error positions
* recd[] = coefficients of the received polynomial
* decerror = number of decoding errors (in _message_ positions)
*
*/

#include <P25/BCH.h>

#include <cmath>
#include <cstdio>
#include <cassert>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const int length = 63;
const int k = 16;

const int g[] = {1, 1, 0,  0, 1, 1,  0, 1, 1,  0, 0, 1,  0, 0, 1,  1, 0, 0,  0, 0, 1,  0, 1, 1,
				 1, 1, 0,  1, 1, 1,  0, 1, 0,  0, 1, 1,  1, 0, 1,  1, 0, 0,  1, 0, 1,  0, 1, 1};

CBCH::CBCH()
{
}

CBCH::~CBCH()
{
}

void CBCH::encode(const int* data, int* bb)
/*
* Compute redundacy bb[], the coefficients of b(x). The redundancy
* polynomial b(x) is the remainder after dividing x^(length-k)*data(x)
* by the generator polynomial g(x).
*/
{
	for (int i = 0; i < length - k; i++)
		bb[i] = 0;

	for (int i = k - 1; i >= 0; i--) {
		int feedback = data[i] ^ bb[length - k - 1];
		if (feedback != 0) {
			for (int j = length - k - 1; j > 0; j--)
				if (g[j] != 0)
					bb[j] = bb[j - 1] ^ feedback;
				else
					bb[j] = bb[j - 1];
			bb[0] = g[0] && feedback;
		} else {
			for (int j = length - k - 1; j > 0; j--)
				bb[j] = bb[j - 1];
			bb[0] = 0;
		}
	}
}

void CBCH::encode(unsigned char* nid)
{
	assert(nid != NULL);

	int data[16];
	for (int i = 0; i < 16; i++)
		data[i] = READ_BIT(nid, i) ? 1 : 0;

	int bb[63];
	encode(data, bb);

	for (int i = 0; i < (length - k); i++) {
		bool b = bb[i] == 1;
		WRITE_BIT(nid, i + 16U, b);
	}
}
