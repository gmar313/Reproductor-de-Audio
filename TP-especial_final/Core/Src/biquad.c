/**
 * @file biquad.c
 *
 * Simple implementation of Biquad filters -- Tom St Denis
 *
 * Based on the work
 *
 *   Cookbook formulae for audio EQ biquad filter coefficients
 *   ---------------------------------------------------------
 *   by Robert Bristow-Johnson, pbjrbj@viconet.com  a.k.a. robert@audioheads.com
 *
 * Available on the web at
 *    http://www.musicdsp.org/files/biquad.c
 *
 * Enjoy.
 *
 * This work is hereby placed in the public domain for all purposes, whether
 * commercial, free [as in speech] or educational, etc.  Use the code and please
 * give me credit if you wish.
 *
 * Tom St Denis -- http://tomstdenis.home.dhs.org
 *
 * See also: http://musicweb.ucsd.edu/~tre/biquad.pdf
 *
 */
#include "biquad.h"

biquad filtro_LP={
		 .a0=0.03341f,
		 .a1=0.06683f,
		 .a2=0.03341f,
		 .a3=-1.62228f,
		 .a4=0.75594f,
		.x1=0,
		.x2=0,
		.y1=0,
		.y2=0
};

biquad filtro_HP={
		.a0=0.48627f,
		 .a1=-0.97254f,
		 .a2=0.48627f,
		 .a3=-0.53833f,
		 .a4=0.40674f,
		.x1=0,
		.x2=0,
		.y1=0,
		.y2=0
};

/* Computes a BiQuad filter on a sample */
smp_type BiQuad(const smp_type sample, biquad* const b)
{
  smp_type result;

  /* compute result */
  result = b->a0 * sample + b->a1 * b->x1 + b->a2 * b->x2 -
    b->a3 * b->y1 - b->a4 * b->y2;

  /* shift x1 to x2, sample to x1 */
  b->x2 = b->x1;
  b->x1 = sample;

  /* shift y1 to y2, result to y1 */
  b->y2 = b->y1;
  b->y1 = result;

  return result;
}



