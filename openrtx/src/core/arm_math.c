/* ----------------------------------------------------------------------
* Copyright (C) 2010-2014 ARM Limited. All rights reserved.
*
* $Date:        19. March 2015
* $Revision: 	V.1.4.5
*
* Project: 	    CMSIS DSP Library
* Title:        arm_fir_fast_q15.c
*
* Description:  Q15 Fast FIR filter processing function.
*
* Target Processor: Cortex-M4/Cortex-M3
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*   - Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   - Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.
*   - Neither the name of ARM LIMITED nor the names of its contributors
*     may be used to endorse or promote products derived from this
*     software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------- */

#include <DSTAR/arm_math.h>

void arm_fir_fast_q15(
  const arm_fir_instance_q15 * S,
  q15_t * pSrc,
  q15_t * pDst,
  uint32_t blockSize)
{
  q15_t *pState = S->pState;                     /* State pointer */
  q15_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer */
  q15_t *pStateCurnt;                            /* Points to the current sample of the state */
  q31_t acc0, acc1, acc2, acc3;                  /* Accumulators */
  q15_t *pb;                                     /* Temporary pointer for coefficient buffer */
  q15_t *px;                                     /* Temporary q31 pointer for SIMD state buffer accesses */
  q31_t x0, x1, x2, c0;                          /* Temporary variables to hold SIMD state and coefficient values */
  uint32_t numTaps = S->numTaps;                 /* Number of taps in the filter */
  uint32_t tapCnt, blkCnt;                       /* Loop counters */


  /* S->pState points to state array which contains previous frame (numTaps - 1) samples */
  /* pStateCurnt points to the location where the new input data should be written */
  pStateCurnt = &(S->pState[(numTaps - 1u)]);

  /* Apply loop unrolling and compute 4 output values simultaneously.
   * The variables acc0 ... acc3 hold output values that are being computed:
   *
   *    acc0 =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0]
   *    acc1 =  b[numTaps-1] * x[n-numTaps] +   b[numTaps-2] * x[n-numTaps-1] + b[numTaps-3] * x[n-numTaps-2] +...+ b[0] * x[1]
   *    acc2 =  b[numTaps-1] * x[n-numTaps+1] + b[numTaps-2] * x[n-numTaps] +   b[numTaps-3] * x[n-numTaps-1] +...+ b[0] * x[2]
   *    acc3 =  b[numTaps-1] * x[n-numTaps+2] + b[numTaps-2] * x[n-numTaps+1] + b[numTaps-3] * x[n-numTaps]   +...+ b[0] * x[3]
   */

  blkCnt = blockSize >> 2;
 // blkCnt = blockSize;
  /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while(blkCnt > 0u)
  {
    /* Copy four new input samples into the state buffer.
     ** Use 32-bit SIMD to move the 16-bit data.  Only requires two copies. */
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;

    /* Set all accumulators to zero */
    acc0 = 0;
    acc1 = 0;
    acc2 = 0;
    acc3 = 0;

    /* Typecast q15_t pointer to q31_t pointer for state reading in q31_t */
    px = pState;

    /* Typecast q15_t pointer to q31_t pointer for coefficient reading in q31_t */
    pb = pCoeffs;

    /* Read the first two samples from the state buffer:  x[n-N], x[n-N-1] */
    x0 = *__SIMD32(px)++;

    /* Read the third and forth samples from the state buffer: x[n-N-2], x[n-N-3] */
    x2 = *__SIMD32(px)++;

    /* Loop over the number of taps.  Unroll by a factor of 4.
     ** Repeat until we've computed numTaps-(numTaps%4) coefficients. */
    tapCnt = numTaps >> 2;

    while(tapCnt > 0)
    {
      /* Read the first two coefficients using SIMD:  b[N] and b[N-1] coefficients */
      c0 = *__SIMD32(pb)++;

      /* acc0 +=  b[N] * x[n-N] + b[N-1] * x[n-N-1] */
      acc0 = __SMLAD(x0, c0, acc0);

      /* acc2 +=  b[N] * x[n-N-2] + b[N-1] * x[n-N-3] */
      acc2 = __SMLAD(x2, c0, acc2);

      /* pack  x[n-N-1] and x[n-N-2] */
#ifndef ARM_MATH_BIG_ENDIAN
      x1 = __PKHBT(x2, x0, 0);
#else
      x1 = __PKHBT(x0, x2, 0);
#endif

      /* Read state x[n-N-4], x[n-N-5] */
      x0 = _SIMD32_OFFSET(px);

      /* acc1 +=  b[N] * x[n-N-1] + b[N-1] * x[n-N-2] */
      acc1 = __SMLADX(x1, c0, acc1);

      /* pack  x[n-N-3] and x[n-N-4] */
#ifndef ARM_MATH_BIG_ENDIAN
      x1 = __PKHBT(x0, x2, 0);
#else
      x1 = __PKHBT(x2, x0, 0);
#endif

      /* acc3 +=  b[N] * x[n-N-3] + b[N-1] * x[n-N-4] */
      acc3 = __SMLADX(x1, c0, acc3);

      /* Read coefficients b[N-2], b[N-3] */
      c0 = *__SIMD32(pb)++;

      /* acc0 +=  b[N-2] * x[n-N-2] + b[N-3] * x[n-N-3] */
      acc0 = __SMLAD(x2, c0, acc0);

      /* Read state x[n-N-6], x[n-N-7] with offset */
      x2 = _SIMD32_OFFSET(px + 2u);

      /* acc2 +=  b[N-2] * x[n-N-4] + b[N-3] * x[n-N-5] */
      acc2 = __SMLAD(x0, c0, acc2);

      /* acc1 +=  b[N-2] * x[n-N-3] + b[N-3] * x[n-N-4] */
      acc1 = __SMLADX(x1, c0, acc1);

      /* pack  x[n-N-5] and x[n-N-6] */
#ifndef ARM_MATH_BIG_ENDIAN
      x1 = __PKHBT(x2, x0, 0);
#else
      x1 = __PKHBT(x0, x2, 0);
#endif

      /* acc3 +=  b[N-2] * x[n-N-5] + b[N-3] * x[n-N-6] */
      acc3 = __SMLADX(x1, c0, acc3);

      /* Update state pointer for next state reading */
      px += 4u;

      /* Decrement tap count */
      tapCnt--;

    }

    /* If the filter length is not a multiple of 4, compute the remaining filter taps.
     ** This is always be 2 taps since the filter length is even. */
    if((numTaps & 0x3u) != 0u)
    {

      /* Read last two coefficients */
      c0 = *__SIMD32(pb)++;

      /* Perform the multiply-accumulates */
      acc0 = __SMLAD(x0, c0, acc0);
      acc2 = __SMLAD(x2, c0, acc2);

      /* pack state variables */
#ifndef ARM_MATH_BIG_ENDIAN
      x1 = __PKHBT(x2, x0, 0);
#else
      x1 = __PKHBT(x0, x2, 0);
#endif

      /* Read last state variables */
      x0 = *__SIMD32(px);

      /* Perform the multiply-accumulates */
      acc1 = __SMLADX(x1, c0, acc1);

      /* pack state variables */
#ifndef ARM_MATH_BIG_ENDIAN
      x1 = __PKHBT(x0, x2, 0);
#else
      x1 = __PKHBT(x2, x0, 0);
#endif

      /* Perform the multiply-accumulates */
      acc3 = __SMLADX(x1, c0, acc3);
    }

    /* The results in the 4 accumulators are in 2.30 format.  Convert to 1.15 with saturation.
     ** Then store the 4 outputs in the destination buffer. */

#ifndef ARM_MATH_BIG_ENDIAN

    *__SIMD32(pDst)++ =
      __PKHBT(__SSAT((acc0 >> 15), 16), __SSAT((acc1 >> 15), 16), 16);

    *__SIMD32(pDst)++ =
      __PKHBT(__SSAT((acc2 >> 15), 16), __SSAT((acc3 >> 15), 16), 16);

#else

    *__SIMD32(pDst)++ =
      __PKHBT(__SSAT((acc1 >> 15), 16), __SSAT((acc0 >> 15), 16), 16);

    *__SIMD32(pDst)++ =
      __PKHBT(__SSAT((acc3 >> 15), 16), __SSAT((acc2 >> 15), 16), 16);


#endif /*      #ifndef ARM_MATH_BIG_ENDIAN       */

    /* Advance the state pointer by 4 to process the next group of 4 samples */
    pState = pState + 4u;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize % 0x4u;
  while(blkCnt > 0u)
  {
    /* Copy two samples into state buffer */
    *pStateCurnt++ = *pSrc++;

    /* Set the accumulator to zero */
    acc0 = 0;

    /* Use SIMD to hold states and coefficients */
    px = pState;
    pb = pCoeffs;

    tapCnt = numTaps >> 1u;

    do
    {

      acc0 += (q31_t) * px++ * *pb++;
	  acc0 += (q31_t) * px++ * *pb++;

      tapCnt--;
    }
    while(tapCnt > 0u);

    /* The result is in 2.30 format.  Convert to 1.15 with saturation.
     ** Then store the output in the destination buffer. */
    *pDst++ = (q15_t) (__SSAT((acc0 >> 15), 16));

    /* Advance state pointer by 1 for the next sample */
    pState = pState + 1u;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* Processing is complete.
   ** Now copy the last numTaps - 1 samples to the satrt of the state buffer.
   ** This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCurnt = S->pState;

  /* Calculation of count for copying integer writes */
  tapCnt = (numTaps - 1u) >> 2;

  while(tapCnt > 0u)
  {
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;

    tapCnt--;

  }

  /* Calculation of count for remaining q15_t data */
  tapCnt = (numTaps - 1u) % 0x4u;

  /* copy remaining data */
  while(tapCnt > 0u)
  {
    *pStateCurnt++ = *pState++;

    /* Decrement the loop counter */
    tapCnt--;
  }

}

void arm_fir_interpolate_q15(
  const arm_fir_interpolate_instance_q15 * S,
  q15_t * pSrc,
  q15_t * pDst,
  uint32_t blockSize)
{
  q15_t *pState = S->pState;                     /* State pointer                                            */
  q15_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer                                      */
  q15_t *pStateCurnt;                            /* Points to the current sample of the state                */
  q15_t *ptr1, *ptr2;                            /* Temporary pointers for state and coefficient buffers     */
  q63_t sum0;                                    /* Accumulators                                             */
  q15_t x0, c0;                                  /* Temporary variables to hold state and coefficient values */
  uint32_t i, blkCnt, j, tapCnt;                 /* Loop counters                                            */
  uint16_t phaseLen = S->phaseLength;            /* Length of each polyphase filter component */
  uint32_t blkCntN2;
  q63_t acc0, acc1;
  q15_t x1;

  /* S->pState buffer contains previous frame (phaseLen - 1) samples */
  /* pStateCurnt points to the location where the new input data should be written */
  pStateCurnt = S->pState + ((q31_t) phaseLen - 1);

  /* Initialise  blkCnt */
  blkCnt = blockSize / 2;
  blkCntN2 = blockSize - (2 * blkCnt);

  /* Samples loop unrolled by 2 */
  while(blkCnt > 0u)
  {
    /* Copy new input sample into the state buffer */
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;

    /* Address modifier index of coefficient buffer */
    j = 1u;

    /* Loop over the Interpolation factor. */
    i = (S->L);

    while(i > 0u)
    {
      /* Set accumulator to zero */
      acc0 = 0;
      acc1 = 0;

      /* Initialize state pointer */
      ptr1 = pState;

      /* Initialize coefficient pointer */
      ptr2 = pCoeffs + (S->L - j);

      /* Loop over the polyPhase length. Unroll by a factor of 4.
       ** Repeat until we've computed numTaps-(4*S->L) coefficients. */
      tapCnt = phaseLen >> 2u;

      x0 = *(ptr1++);

      while(tapCnt > 0u)
      {

        /* Read the input sample */
        x1 = *(ptr1++);

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x0 *c0;
        acc1 += (q63_t) x1 *c0;


        /* Read the coefficient */
        c0 = *(ptr2 + S->L);

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x1 *c0;
        acc1 += (q63_t) x0 *c0;


        /* Read the coefficient */
        c0 = *(ptr2 + S->L * 2);

        /* Read the input sample */
        x1 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x0 *c0;
        acc1 += (q63_t) x1 *c0;

        /* Read the coefficient */
        c0 = *(ptr2 + S->L * 3);

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x1 *c0;
        acc1 += (q63_t) x0 *c0;


        /* Upsampling is done by stuffing L-1 zeros between each sample.
         * So instead of multiplying zeros with coefficients,
         * Increment the coefficient pointer by interpolation factor times. */
        ptr2 += 4 * S->L;

        /* Decrement the loop counter */
        tapCnt--;
      }

      /* If the polyPhase length is not a multiple of 4, compute the remaining filter taps */
      tapCnt = phaseLen % 0x4u;

      while(tapCnt > 0u)
      {

        /* Read the input sample */
        x1 = *(ptr1++);

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x0 *c0;
        acc1 += (q63_t) x1 *c0;

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* update states for next sample processing */
        x0 = x1;

        /* Decrement the loop counter */
        tapCnt--;
      }

      /* The result is in the accumulator, store in the destination buffer. */
      *pDst = (q15_t) (__SSAT((acc0 >> 15), 16));
      *(pDst + S->L) = (q15_t) (__SSAT((acc1 >> 15), 16));

      pDst++;

      /* Increment the address modifier index of coefficient buffer */
      j++;

      /* Decrement the loop counter */
      i--;
    }

    /* Advance the state pointer by 1
     * to process the next group of interpolation factor number samples */
    pState = pState + 2;

    pDst += S->L;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 2, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blkCntN2;

  /* Loop over the blockSize. */
  while(blkCnt > 0u)
  {
    /* Copy new input sample into the state buffer */
    *pStateCurnt++ = *pSrc++;

    /* Address modifier index of coefficient buffer */
    j = 1u;

    /* Loop over the Interpolation factor. */
    i = S->L;
    while(i > 0u)
    {
      /* Set accumulator to zero */
      sum0 = 0;

      /* Initialize state pointer */
      ptr1 = pState;

      /* Initialize coefficient pointer */
      ptr2 = pCoeffs + (S->L - j);

      /* Loop over the polyPhase length. Unroll by a factor of 4.
       ** Repeat until we've computed numTaps-(4*S->L) coefficients. */
      tapCnt = phaseLen >> 2;
      while(tapCnt > 0u)
      {

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Upsampling is done by stuffing L-1 zeros between each sample.
         * So instead of multiplying zeros with coefficients,
         * Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        sum0 += (q63_t) x0 *c0;

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        sum0 += (q63_t) x0 *c0;

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        sum0 += (q63_t) x0 *c0;

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        sum0 += (q63_t) x0 *c0;

        /* Decrement the loop counter */
        tapCnt--;
      }

      /* If the polyPhase length is not a multiple of 4, compute the remaining filter taps */
      tapCnt = phaseLen & 0x3u;

      while(tapCnt > 0u)
      {
        /* Read the coefficient */
        c0 = *(ptr2);

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        sum0 += (q63_t) x0 *c0;

        /* Decrement the loop counter */
        tapCnt--;
      }

      /* The result is in the accumulator, store in the destination buffer. */
      *pDst++ = (q15_t) (__SSAT((sum0 >> 15), 16));

      j++;

      /* Decrement the loop counter */
      i--;
    }

    /* Advance the state pointer by 1
     * to process the next group of interpolation factor number samples */
    pState = pState + 1;

    /* Decrement the loop counter */
    blkCnt--;
  }


  /* Processing is complete.
   ** Now copy the last phaseLen - 1 samples to the satrt of the state buffer.
   ** This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCurnt = S->pState;

  i = ((uint32_t) phaseLen - 1u) >> 2u;

  /* copy data */
  while(i > 0u)
  {
#ifndef UNALIGNED_SUPPORT_DISABLE

    *__SIMD32(pStateCurnt)++ = *__SIMD32(pState)++;
    *__SIMD32(pStateCurnt)++ = *__SIMD32(pState)++;

#else

    *pStateCurnt++ = *pState++;
	*pStateCurnt++ = *pState++;
	*pStateCurnt++ = *pState++;
	*pStateCurnt++ = *pState++;

#endif	/*	#ifndef UNALIGNED_SUPPORT_DISABLE	*/

	/* Decrement the loop counter */
    i--;
  }

  i = ((uint32_t) phaseLen - 1u) % 0x04u;

  while(i > 0u)
  {
    *pStateCurnt++ = *pState++;

    /* Decrement the loop counter */
    i--;
  }
}

