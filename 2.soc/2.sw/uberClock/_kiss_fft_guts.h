/*
 *  Copyright (c) 2003-2010, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _KISS_FFT_GUTS_H
#define _KISS_FFT_GUTS_H

#include "kiss_fft.h"

#include <limits.h>
#include <stdint.h>

#ifndef KISS_FFT_TMP_ALLOC
#define KISS_FFT_TMP_ALLOC(nbytes) KISS_FFT_MALLOC(nbytes)
#endif

#ifndef KISS_FFT_TMP_FREE
#define KISS_FFT_TMP_FREE(ptr) KISS_FFT_FREE(ptr)
#endif

#ifndef KISS_FFT_ERROR
#define KISS_FFT_ERROR(msg) fprintf(stderr, "%s\n", (msg))
#endif

struct kiss_fft_state {
    int nfft;
    int inverse;
    int factors[2 * 32];
    kiss_fft_cpx twiddles[1];
};

#ifdef FIXED_POINT

#if (FIXED_POINT == 32)
#define FRACBITS 31
#define SAMPPROD int64_t
#define SAMP_MAX INT32_MAX
#else
#define FRACBITS 15
#define SAMPPROD int32_t
#define SAMP_MAX INT16_MAX
#endif

#define S_MUL(a, b) ((kiss_fft_scalar)(((SAMPPROD)(a) * (SAMPPROD)(b)) >> FRACBITS))
#define C_MUL(m, a, b) \
    do { \
        (m).r = S_MUL((a).r, (b).r) - S_MUL((a).i, (b).i); \
        (m).i = S_MUL((a).r, (b).i) + S_MUL((a).i, (b).r); \
    } while (0)
#define C_FIXDIV(c, div) \
    do { \
        (c).r /= (div); \
        (c).i /= (div); \
    } while (0)
#define C_MULBYSCALAR(c, s) \
    do { \
        (c).r = S_MUL((c).r, (s)); \
        (c).i = S_MUL((c).i, (s)); \
    } while (0)
#define HALF_OF(x) ((x) >> 1)

static inline void kf_cexp(kiss_fft_cpx *x, double phase)
{
    x->r = (kiss_fft_scalar)(SAMP_MAX * cos(phase));
    x->i = (kiss_fft_scalar)(SAMP_MAX * sin(phase));
}

#else

#define S_MUL(a, b) ((a) * (b))
#define C_MUL(m, a, b) \
    do { \
        (m).r = (a).r * (b).r - (a).i * (b).i; \
        (m).i = (a).r * (b).i + (a).i * (b).r; \
    } while (0)
#define C_FIXDIV(c, div)
#define C_MULBYSCALAR(c, s) \
    do { \
        (c).r *= (s); \
        (c).i *= (s); \
    } while (0)
#define HALF_OF(x) ((x) * .5f)

static inline void kf_cexp(kiss_fft_cpx *x, double phase)
{
    x->r = (kiss_fft_scalar)cos(phase);
    x->i = (kiss_fft_scalar)sin(phase);
}

#endif

#define C_ADD(res, a, b) \
    do { \
        (res).r = (a).r + (b).r; \
        (res).i = (a).i + (b).i; \
    } while (0)
#define C_SUB(res, a, b) \
    do { \
        (res).r = (a).r - (b).r; \
        (res).i = (a).i - (b).i; \
    } while (0)
#define C_ADDTO(res, a) \
    do { \
        (res).r += (a).r; \
        (res).i += (a).i; \
    } while (0)

#endif
