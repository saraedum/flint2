/*
    Copyright (C) 2022 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <https://www.gnu.org/licenses/>.
*/

#ifndef FFT_SMALL_H
#define FFT_SMALL_H

#undef ulong
#define ulong ulongxx /* interferes with system includes */
#include <stdlib.h>
#include <stdio.h>
#undef ulong

#include <gmp.h>
#define ulong mp_limb_t
#include "flint.h"
#include "mpn_extras.h"
#include "machine_vectors.h"
#include "nmod.h"

#define LG_BLK_SZ 8
#define BLK_SZ 256
#define BLK_SHIFT 10


#ifdef __cplusplus
 extern "C" {
#endif

FLINT_INLINE ulong n_pow2(int k)
{
    return UWORD(1) << k;
}

FLINT_INLINE ulong n_min(ulong a, ulong b)
{
    return FLINT_MIN(a, b);
}

FLINT_INLINE ulong n_max(ulong a, ulong b)
{
    return FLINT_MAX(a, b);
}

FLINT_INLINE ulong n_cdiv(ulong a, ulong b)
{
    return (a + b - 1)/b;
}

FLINT_INLINE ulong n_round_up(ulong a, ulong b)
{
    return n_cdiv(a, b)*b;
}

FLINT_INLINE ulong n_round_down(ulong a, ulong b)
{
    return a/b*b;
}

/* 0 -> 0, 1 -> 1, [2,3] -> 3, [4,7] -> 7, [8,15] -> 15, ... */
FLINT_INLINE ulong n_next_pow2m1(ulong a)
{
    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;
#if FLINT64
    a |= a >> 32;
#endif
    return a;
}

#if 0
FLINT_INLINE ulong n_clog2(ulong x) {
    if (x <= 2)
        return x == 2;

   ulong zeros = FLINT_BITS;
   count_leading_zeros(zeros, x - 1);
   return FLINT_BITS - zeros;
}
#endif


FLINT_INLINE ulong n_leading_zeros(ulong x) {
    return __builtin_clzll(x);
}

FLINT_INLINE ulong n_trailing_zeros(ulong x) {
    return __builtin_ctzll(x);
}

FLINT_INLINE ulong n_nbits(ulong x) {
    return 64 - n_leading_zeros(x);
}

FLINT_INLINE ulong n_clog2(ulong x) {
    return (x <= 2) ? (x == 2) : 64 - __builtin_clzll(x - 1);
}


FLINT_DLL void* flint_aligned_alloc(ulong alignment, ulong size);

FLINT_DLL void flint_aligned_free(void* p);

/*
    The twiddle factors are split across FLINT_BITS tables:

        [0] = {e(1)}                                original index 0
        [1] = {e(1/4)}                              original index 1
        [2] = {e(1/8), e(3/8)}                      original index 2,3
        [3] = {e(1/16), e(5/16), e(3/16), e(7/16)}  original index 4,5,6,7
        ...

    The unallocated ones start out as NULL, and once they are filled in, they
    never have to move. This simplifies thread safe enlargement but complicates
    random access into the original table. If j is the index into the original
    table, the new indicies are

        [j_bits][j_r]  where j_bits = nbits(j), j_r = j - 2^(j_bits-1)

    with the special case j_bits = j_r = 0 for j = 0.
    The first SD_FFT_CTX_INIT_DEPTH tables are stored consecutively to ease the
    lookup of small indices, which must currently be at least 4.
*/

#define SD_FFT_CTX_INIT_DEPTH 12

/* This context is the one expected to sit in a global position */
typedef struct {
    double p;
    double pinv;
    nmod_t mod;
    ulong primitive_root;
    ulong blk_sz;
    volatile ulong w2tab_depth;
    double* w2tab[FLINT_BITS];
} sd_fft_ctx_struct;

typedef sd_fft_ctx_struct sd_fft_ctx_t[1];

/* The local context is expected to be copied and passed to the calculations. */
typedef struct {
    double* data;
    double p;
    double pinv;
    const double* w2tab[50];
} sd_fft_lctx_struct;

typedef sd_fft_lctx_struct sd_fft_lctx_t[1];


FLINT_INLINE ulong sd_fft_ctx_blk_offset(ulong I)
{
    return (I << LG_BLK_SZ) + 4*(I >> (BLK_SHIFT+2));
}

FLINT_INLINE ulong sd_fft_ctx_data_size(ulong depth)
{
    return sd_fft_ctx_blk_offset(n_pow2(depth - LG_BLK_SZ));
}

FLINT_INLINE double* sd_fft_ctx_blk_index(double* d, ulong I)
{
    return d + sd_fft_ctx_blk_offset(I);
}

FLINT_INLINE double* sd_fft_lctx_blk_index(const sd_fft_lctx_t Q, ulong I)
{
    return Q->data + sd_fft_ctx_blk_offset(I);
}

FLINT_INLINE void sd_fft_ctx_set_index(double* d, ulong i, double x)
{
    sd_fft_ctx_blk_index(d, i/BLK_SZ)[i%BLK_SZ] = x;
}

FLINT_INLINE double sd_fft_ctx_get_index(double* d, ulong i)
{
    return sd_fft_ctx_blk_index(d, i/BLK_SZ)[i%BLK_SZ];
}

/* slightly-worse-than-bit-reversed order */
FLINT_INLINE double sd_fft_ctx_get_fft_index(double* d, ulong i)
{
    ulong j = i&(BLK_SZ-16);
    FLINT_ASSERT(BLK_SZ >= 16);
    j |= (i&3)<<2;
    j |= ((i>>2)&3);
    return sd_fft_ctx_blk_index(d, i/BLK_SZ)[j];
}

/* sd_fft.c */
FLINT_DLL void sd_fft_trunc(const sd_fft_lctx_t Q, ulong I, ulong S, ulong k, ulong j, ulong itrunc, ulong otrunc);

/* sd_ifft.c */
FLINT_DLL void sd_ifft_trunc(const sd_fft_lctx_t Q, ulong I, ulong S, ulong k, ulong j, ulong z, ulong n, int f);

/* sd_fft_ctx.c */
FLINT_DLL void sd_fft_ctx_clear(sd_fft_ctx_t Q);
FLINT_DLL void sd_fft_ctx_init_prime(sd_fft_ctx_t Q, ulong pp);
FLINT_DLL void sd_fft_ctx_fit_depth(sd_fft_ctx_t Q, ulong k);

/* TODO: these should probably increment/decrement a ref count */
FLINT_INLINE void sd_fft_lctx_init(sd_fft_lctx_t L, sd_fft_ctx_t Q, ulong depth)
{
    L->p = Q->p;
    L->pinv = Q->pinv;
    sd_fft_ctx_fit_depth(Q, depth);
    for (int i = 0; i < 50; i++)
        L->w2tab[i] = Q->w2tab[i];
}

FLINT_INLINE void sd_fft_lctx_clear(sd_fft_lctx_t LQ, sd_fft_ctx_t Q)
{
}

FLINT_DLL void sd_fft_lctx_point_mul(const sd_fft_lctx_t Q,
                            double* a, const double* b, ulong m_, ulong depth);

FLINT_INLINE void sd_fft_lctx_fft_trunc(sd_fft_lctx_t Q, double* d, ulong depth, ulong itrunc, ulong otrunc)
{
    FLINT_ASSERT(depth >= LG_BLK_SZ);
    FLINT_ASSERT(itrunc % BLK_SZ == 0);
    FLINT_ASSERT(otrunc % BLK_SZ == 0);
    FLINT_ASSERT(Q->w2tab[depth - 1] != NULL);
    Q->data = d;
    sd_fft_trunc(Q, 0, 1, depth - LG_BLK_SZ, 0, itrunc/BLK_SZ, otrunc/BLK_SZ);
}

FLINT_INLINE void sd_fft_lctx_ifft_trunc(sd_fft_lctx_t Q, double* d, ulong depth, ulong trunc)
{
    FLINT_ASSERT(depth >= LG_BLK_SZ);
    FLINT_ASSERT(trunc % BLK_SZ == 0);
    FLINT_ASSERT(Q->w2tab[depth - 1] != NULL);
    Q->data = d;
    sd_ifft_trunc(Q, 0, 1, depth - LG_BLK_SZ, 0, trunc/BLK_SZ, trunc/BLK_SZ, 0);
}

FLINT_INLINE void sd_fft_ctx_fft_trunc(sd_fft_ctx_t Q, double* d, ulong depth, ulong itrunc, ulong otrunc)
{
    sd_fft_lctx_t QL;
    sd_fft_lctx_init(QL, Q, depth);
    sd_fft_lctx_fft_trunc(QL, d, depth, itrunc, otrunc);
}

FLINT_INLINE void sd_fft_ctx_ifft_trunc(sd_fft_ctx_t Q, double* d, ulong depth, ulong trunc)
{
    sd_fft_lctx_t QL;
    sd_fft_lctx_init(QL, Q, depth);
    sd_fft_lctx_ifft_trunc(QL, d, depth, trunc);
}

/*
    The bit reversed table is
        w = {e(0), e(1/2), e(1/4), e(3/4), e(1/8), e(5/8), e(3/8), e(7/8), ...}
    Only the terms of even index are explicitly stored, and they are split
    among several tables.
*/

/* look up w[2*j] */
FLINT_INLINE double sd_fft_lctx_w2(const sd_fft_lctx_t Q, ulong j)
{
    ulong j_bits = n_nbits(j);
    ulong j_r = j & (n_pow2(j_bits)/2 - 1);
    return Q->w2tab[j_bits][j_r];
}

/* look up -w[2*j]^-1 */
FLINT_INLINE double sd_fft_lctx_w2inv(const sd_fft_lctx_t Q, ulong j)
{
    ulong j_bits = n_nbits(j);
    ulong j_mr = n_pow2(j_bits) - 1 - j;
    if (j == 0)
        return -1.0;
    else
        return Q->w2tab[j_bits][j_mr];
}

/* look up w[jj] */
FLINT_INLINE double sd_fft_ctx_w(const sd_fft_ctx_t Q, ulong jj)
{
    ulong j = jj/2;
    double s = (jj&1) ? -1.0 : 1.0;
    ulong j_bits = n_nbits(j);
    ulong j_r = j & (n_pow2(j_bits)/2 - 1);
    return s*Q->w2tab[j_bits][j_r];
}

typedef struct {
    ulong prime;
    ulong coeff_len;
    ulong nprimes;
    ulong* data;
} crt_data_struct;

typedef crt_data_struct crt_data_t[1];

FLINT_DLL void crt_data_init(crt_data_t C, ulong prime, ulong coeff_len, ulong nprimes);

FLINT_DLL void crt_data_clear(crt_data_t C);

/* return mpn of length C->coeff_len */
FLINT_FORCE_INLINE ulong* crt_data_co_prime(const crt_data_t C, ulong i)
{
    FLINT_ASSERT(i < C->nprimes);
    return C->data + i*C->coeff_len;
}

FLINT_FORCE_INLINE ulong* _crt_data_co_prime(const crt_data_t C, ulong i, ulong n)
{
    FLINT_ASSERT(i < C->nprimes);
    FLINT_ASSERT(n == C->coeff_len);
    return C->data + i*n;
}

/* return mpn of length C->coeff_len */
FLINT_FORCE_INLINE ulong* crt_data_prod_primes(const crt_data_t C)
{
    return C->data + C->nprimes*C->coeff_len;
}

/* the reduction of co_prime mod the i^th prime */
FLINT_FORCE_INLINE ulong* crt_data_co_prime_red(const crt_data_t C, ulong i)
{
    FLINT_ASSERT(i < C->nprimes);
    return C->data + C->nprimes*C->coeff_len + C->coeff_len + i;
}


typedef void (*to_ffts_func)(
    sd_fft_ctx_struct* Qffts, double* d, ulong dstride,
    const ulong* a_, ulong an_, ulong atrunc,
    const vec4d* two_pow,
    ulong start_easy, ulong stop_easy,
    ulong start_hard, ulong stop_hard);

typedef struct {
    ulong np;
    ulong bits;
    ulong bn_bound;
    to_ffts_func to_ffts;
} profile_entry_struct;

typedef profile_entry_struct profile_entry_t[1];

#define MPN_CTX_NCRTS 8
#define MAX_NPROFILES 20
#define VEC_SZ 4

/*
    The tables for powers of two each have this fixed length. This has to go up
    linearly with the max number of primes MPN_CTX_NCRTS involved in chinese
    remaindering. This length is checked with asserts in the code.
*/
#define MPN_CTX_TWO_POWER_TAB_SIZE 256

typedef struct {
    sd_fft_ctx_struct ffts[MPN_CTX_NCRTS];
    crt_data_struct crts[MPN_CTX_NCRTS];

    /*
        For each table of tables of powers of two, the whole collection is held
        in one big buffer and the table is an array of pointer into it.
    */
    vec4d* vec_two_pow_tab[(MPN_CTX_NCRTS + VEC_SZ - 1)/VEC_SZ];
    vec4d* vec_two_pow_buffer;
    double* slow_two_pow_tab[MPN_CTX_NCRTS];
    double* slow_two_pow_buffer;

    profile_entry_struct profiles[MAX_NPROFILES];
    ulong profiles_size;
    void* buffer;
    ulong buffer_alloc;
} mpn_ctx_struct;

typedef mpn_ctx_struct mpn_ctx_t[1];

void mpn_ctx_init(mpn_ctx_t R, ulong p);
void mpn_ctx_clear(mpn_ctx_t R);
void* mpn_ctx_fit_buffer(mpn_ctx_t R, ulong n);
void mpn_ctx_mpn_mul(mpn_ctx_t R, ulong* z, ulong* a, ulong an, ulong* b, ulong bn);

void _mpn_ctx_nmod_poly_mul(
    mpn_ctx_t R,
    ulong* z,
    ulong* a, ulong an,
    ulong* b, ulong bn,
    nmod_t mod);

int flint_mpn_cmp_ui_2exp(const ulong* a, ulong an, ulong b, ulong e);

#ifdef __cplusplus
}
#endif

#endif

