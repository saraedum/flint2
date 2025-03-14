/*
    Copyright (C) 2016-2017 William Hart
    Copyright (C) 2017,2018 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#ifndef FMPZ_MPOLY_H
#define FMPZ_MPOLY_H

#ifdef FMPZ_MPOLY_INLINES_C
#define FMPZ_MPOLY_INLINE FLINT_DLL
#else
#define FMPZ_MPOLY_INLINE static __inline__
#endif

#undef ulong
#define ulong ulongxx /* interferes with system includes */
#include <stdio.h>
#undef ulong

#include <gmp.h>
#define ulong mp_limb_t

#include "flint.h"
#include "fmpz.h"
#include "fmpz_vec.h"
#include "fmpz_poly.h"
#include "mpoly.h"
#include "nmod_mpoly.h"
#include "fmpz_mod.h"

#ifdef __cplusplus
 extern "C" {
#endif

/* Type definitions **********************************************************/

/*
    context object for fmpz_mpoly
*/
typedef struct
{
    mpoly_ctx_t minfo;
} fmpz_mpoly_ctx_struct;

typedef fmpz_mpoly_ctx_struct fmpz_mpoly_ctx_t[1];

/*
    fmpz_mpoly_t
    sparse multivariates with fmpz coeffs
*/
typedef struct
{
   fmpz * coeffs; /* alloc fmpzs */
   ulong * exps;
   slong alloc;
   slong length;
   flint_bitcnt_t bits;     /* number of bits per exponent */
} fmpz_mpoly_struct;

typedef fmpz_mpoly_struct fmpz_mpoly_t[1];


/* Internal type definitions *************************************************/

/*
    fmpz_mpoly_univar_t
    sparse univariates with multivariate coefficients
*/
typedef struct
{
   fmpz_mpoly_struct * coeffs; /* multivariate coefficients */
   ulong * exps;
   slong alloc;
   slong length;
   slong var; /* univariate variable number */
} fmpz_mpoly_univar_struct;

typedef fmpz_mpoly_univar_struct fmpz_mpoly_univar_t[1];

/*
    fmpz_mpolyu_t
    sparse univariates with fmpz_mpoly_t coefficients
        with uniform bits and LEX ordering
*/
typedef struct
{
   fmpz_mpoly_struct * coeffs;
   ulong * exps;
   slong alloc;
   slong length;
   flint_bitcnt_t bits;    /* default bits to construct coeffs */
} fmpz_mpolyu_struct;

typedef fmpz_mpolyu_struct fmpz_mpolyu_t[1];

/*
    fmpz_mpolyd_t
    A dense mpoly is stored as a flat array of coeffcients.
    Suppose deg_bounds = {r0, r1, r2}. The coefficient of the monomial with
    exponents {e0, e1, e2} is stored at the coefficient of index
        e2 + r2*(e1 + r1*(e0 + r0*0))
*/
typedef struct
{
    slong nvars;
    slong degb_alloc;
    slong * deg_bounds;
    slong length;           /* usage is inconsistent currently */
    slong coeff_alloc;
    fmpz * coeffs;
} fmpz_mpolyd_struct;

typedef fmpz_mpolyd_struct fmpz_mpolyd_t[1];

/* Context object ************************************************************/

FLINT_DLL void fmpz_mpoly_ctx_init(fmpz_mpoly_ctx_t ctx, 
                                            slong nvars, const ordering_t ord);

FLINT_DLL void fmpz_mpoly_ctx_init_rand(fmpz_mpoly_ctx_t mctx, flint_rand_t state, slong max_nvars);


FLINT_DLL void fmpz_mpoly_ctx_clear(fmpz_mpoly_ctx_t ctx);

FMPZ_MPOLY_INLINE
slong fmpz_mpoly_ctx_nvars(const fmpz_mpoly_ctx_t ctx)
{
    return ctx->minfo->nvars;
}

FMPZ_MPOLY_INLINE
ordering_t fmpz_mpoly_ctx_ord(const fmpz_mpoly_ctx_t ctx)
{
    return ctx->minfo->ord;
}


/*  Memory management ********************************************************/

FLINT_DLL void fmpz_mpoly_init(fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_init2(fmpz_mpoly_t A, slong alloc, 
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_init3(fmpz_mpoly_t A, slong alloc, flint_bitcnt_t bits,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_realloc(fmpz ** Acoeff, ulong ** Aexp,
                                           slong * Aalloc, slong len, slong N);

FLINT_DLL void fmpz_mpoly_realloc(fmpz_mpoly_t A, slong alloc, 
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_fit_length(fmpz ** Acoeff,
                            ulong ** Aexp, slong * Aalloc, slong len, slong N);

FLINT_DLL void fmpz_mpoly_fit_length(fmpz_mpoly_t A, slong len, 
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_clear(fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx);

FMPZ_MPOLY_INLINE
void _fmpz_mpoly_set_length(fmpz_mpoly_t A, slong newlen, 
                                                   const fmpz_mpoly_ctx_t ctx)
{
    if (A->length > newlen)
    {
        slong i;
        for (i = newlen; i < A->length; i++)
           _fmpz_demote(A->coeffs + i); 
    }
    A->length = newlen;
}

FMPZ_MPOLY_INLINE
void fmpz_mpoly_truncate(fmpz_mpoly_t A, slong newlen, 
                                                   const fmpz_mpoly_ctx_t ctx)
{
    if (A->length > newlen)
    {
        slong i;

        for (i = newlen; i < A->length; i++)
            _fmpz_demote(A->coeffs + i);

        A->length = newlen;
    }  
}

FMPZ_MPOLY_INLINE
void fmpz_mpoly_fit_bits(fmpz_mpoly_t A,
                                  flint_bitcnt_t bits, const fmpz_mpoly_ctx_t ctx)
{
   if (A->bits < bits)
   {
      if (A->alloc != 0)
      {
         slong N = mpoly_words_per_exp(bits, ctx->minfo);
         ulong * t = (ulong *) flint_malloc(N*A->alloc*sizeof(ulong));
         mpoly_repack_monomials(t, bits, A->exps, A->bits, A->length, ctx->minfo);
         flint_free(A->exps);
         A->exps = t;
      }

      A->bits = bits;
   }
}


/* Input/output **************************************************************/

FLINT_DLL int fmpz_mpoly_set_str_pretty(fmpz_mpoly_t A, const char * str,
                                  const char ** x, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL char * _fmpz_mpoly_get_str_pretty(const fmpz * poly,
                          const ulong * exps, slong len, const char ** x, 
                                           slong bits, const mpoly_ctx_t mctx);

FLINT_DLL char * fmpz_mpoly_get_str_pretty(const fmpz_mpoly_t A,
                                  const char ** x, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int _fmpz_mpoly_fprint_pretty(FILE * file, const fmpz * poly, 
                        const ulong * exps, slong len, const char ** x_in,
                                     flint_bitcnt_t bits, const mpoly_ctx_t mctx);

FLINT_DLL int fmpz_mpoly_fprint_pretty(FILE * file, 
            const fmpz_mpoly_t A, const char ** x, const fmpz_mpoly_ctx_t ctx);

FMPZ_MPOLY_INLINE
int _fmpz_mpoly_print_pretty(const fmpz * poly, 
                       const ulong * exps, slong len, const char ** x,
                                            slong bits, const mpoly_ctx_t mctx)
{
    return _fmpz_mpoly_fprint_pretty(stdout, poly, exps, len, x, bits, mctx);
}

FMPZ_MPOLY_INLINE
int fmpz_mpoly_print_pretty(const fmpz_mpoly_t A,
                                   const char ** x, const fmpz_mpoly_ctx_t ctx)
{
   return fmpz_mpoly_fprint_pretty(stdout, A, x, ctx);
}


/*  Basic manipulation *******************************************************/

FLINT_DLL void fmpz_mpoly_gen(fmpz_mpoly_t poly, slong i,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_is_gen(const fmpz_mpoly_t poly,
                                          slong k, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_set(fmpz * poly1, ulong * exps1,
                    const fmpz * poly2, const ulong * exps2, slong n, slong N);

FLINT_DLL void fmpz_mpoly_set(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int _fmpz_mpoly_equal(fmpz * poly1, ulong * exps1,
                    const fmpz * poly2, const ulong * exps2, slong n, slong N);

FLINT_DLL int fmpz_mpoly_equal(const fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FMPZ_MPOLY_INLINE
void fmpz_mpoly_swap(fmpz_mpoly_t A, 
                                fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx)
{
   fmpz_mpoly_struct t = *A;
   *A = *B;
   *B = t;
}

FMPZ_MPOLY_INLINE
int _fmpz_mpoly_fits_small(const fmpz * poly, slong len)
{
   slong i;
   for (i = 0; i < len; i++)
   {
      if (COEFF_IS_MPZ(poly[i]))
         return 0;
   }
   return 1;
}

FMPZ_MPOLY_INLINE
slong fmpz_mpoly_max_bits(const fmpz_mpoly_t A)
{
    return _fmpz_vec_max_bits(A->coeffs, A->length);
}


/* Constants *****************************************************************/

FLINT_DLL int fmpz_mpoly_is_fmpz(const fmpz_mpoly_t A,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_get_fmpz(fmpz_t c, const fmpz_mpoly_t A,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_fmpz(fmpz_mpoly_t A,
                                   const fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_ui(fmpz_mpoly_t A,
                                          ulong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_si(fmpz_mpoly_t A,
                                          slong c, const fmpz_mpoly_ctx_t ctx);

FMPZ_MPOLY_INLINE
void fmpz_mpoly_zero(fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx)
{
   _fmpz_mpoly_set_length(A, 0, ctx);
}

FMPZ_MPOLY_INLINE
void fmpz_mpoly_one(fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx)
{
    fmpz_mpoly_set_ui(A, UWORD(1), ctx);
}

FLINT_DLL int fmpz_mpoly_equal_fmpz(const fmpz_mpoly_t A,
                                   const fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_equal_ui(const fmpz_mpoly_t A,
                                          ulong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_equal_si(const fmpz_mpoly_t A,
                                          slong c, const fmpz_mpoly_ctx_t ctx);

FMPZ_MPOLY_INLINE
int fmpz_mpoly_is_zero(const fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx)
{
   return A->length == 0;
}

FMPZ_MPOLY_INLINE
int fmpz_mpoly_is_one(const fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx)
{
   return fmpz_mpoly_equal_ui(A, UWORD(1), ctx);
}


/* Degrees *******************************************************************/

FMPZ_MPOLY_INLINE
int fmpz_mpoly_degrees_fit_si(const fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx)
{
    return A->bits <= FLINT_BITS ? 1
               : mpoly_degrees_fit_si(A->exps, A->length, A->bits, ctx->minfo);
}

FMPZ_MPOLY_INLINE
void fmpz_mpoly_degrees_fmpz(fmpz ** degs, const fmpz_mpoly_t A,
                                                   const fmpz_mpoly_ctx_t ctx)
{
    mpoly_degrees_pfmpz(degs, A->exps, A->length, A->bits, ctx->minfo);
}

FMPZ_MPOLY_INLINE
void fmpz_mpoly_degrees_si(slong * degs, const fmpz_mpoly_t A,
                                                   const fmpz_mpoly_ctx_t ctx)
{
    mpoly_degrees_si(degs, A->exps, A->length, A->bits, ctx->minfo);
}

FMPZ_MPOLY_INLINE
void fmpz_mpoly_degree_fmpz(fmpz_t deg, const fmpz_mpoly_t A, slong var,
                                                   const fmpz_mpoly_ctx_t ctx)
{
    mpoly_degree_fmpz(deg, A->exps, A->length, A->bits, var, ctx->minfo);
}

FMPZ_MPOLY_INLINE
slong fmpz_mpoly_degree_si(const fmpz_mpoly_t A, slong var,
                                                   const fmpz_mpoly_ctx_t ctx)
{
    return mpoly_degree_si(A->exps, A->length, A->bits, var, ctx->minfo);
}

FMPZ_MPOLY_INLINE
int fmpz_mpoly_total_degree_fits_si(const fmpz_mpoly_t A,
                                                    const fmpz_mpoly_ctx_t ctx)
{
    return mpoly_total_degree_fits_si(A->exps, A->length, A->bits, ctx->minfo);
}

FMPZ_MPOLY_INLINE
void fmpz_mpoly_total_degree_fmpz(fmpz_t td, const fmpz_mpoly_t A,
                                                    const fmpz_mpoly_ctx_t ctx)
{
    mpoly_total_degree_fmpz(td, A->exps, A->length, A->bits, ctx->minfo);
}

FMPZ_MPOLY_INLINE
slong fmpz_mpoly_total_degree_si(const fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx)
{
    return mpoly_total_degree_si(A->exps, A->length, A->bits, ctx->minfo);
}


/* Coefficients **************************************************************/

FLINT_DLL void fmpz_mpoly_get_coeff_fmpz_monomial(fmpz_t c,
                          const fmpz_mpoly_t A, const fmpz_mpoly_t M,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_coeff_fmpz_monomial(fmpz_mpoly_t A,
                                  const fmpz_t c, const fmpz_mpoly_t M,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_get_coeff_fmpz_fmpz(fmpz_t c, const fmpz_mpoly_t A,
                               fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL ulong fmpz_mpoly_get_coeff_ui_fmpz(           const fmpz_mpoly_t A,
                               fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong fmpz_mpoly_get_coeff_si_fmpz(           const fmpz_mpoly_t A,
                               fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_get_coeff_fmpz_ui(fmpz_t c, const fmpz_mpoly_t A,
                                const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL ulong fmpz_mpoly_get_coeff_ui_ui(           const fmpz_mpoly_t A,
                                const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong fmpz_mpoly_get_coeff_si_ui(           const fmpz_mpoly_t A,
                                const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_set_coeff_fmpz_fmpz(fmpz_mpoly_t A,
                 const fmpz_t c, const fmpz * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_coeff_fmpz_fmpz(fmpz_mpoly_t A,
               const fmpz_t c, fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_coeff_ui_fmpz(fmpz_mpoly_t A,
                const ulong c, fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_coeff_si_fmpz(fmpz_mpoly_t A,
                const slong c, fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_coeff_fmpz_ui(fmpz_mpoly_t A,
                const fmpz_t c, const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_coeff_ui_ui(fmpz_mpoly_t A,
                 const ulong c, const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_coeff_si_ui(fmpz_mpoly_t A,
                 const slong c, const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_get_coeff_vars_ui(fmpz_mpoly_t C,
             const fmpz_mpoly_t A,  slong * vars, ulong * exps, slong length,
                                                   const fmpz_mpoly_ctx_t ctx);

/* comparison ****************************************************************/

FLINT_DLL int fmpz_mpoly_cmp(const fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

/* container operations ******************************************************/

FLINT_DLL int fmpz_mpoly_is_canonical(const fmpz_mpoly_t A,
                                                   const fmpz_mpoly_ctx_t ctx);

FMPZ_MPOLY_INLINE
slong fmpz_mpoly_length(const fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx)
{
    return A->length;
}

FLINT_DLL void fmpz_mpoly_resize(fmpz_mpoly_t A, slong new_length,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_get_term_coeff_fmpz(fmpz_t c, const fmpz_mpoly_t A,
                                          slong i, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL ulong fmpz_mpoly_get_term_coeff_ui(           const fmpz_mpoly_t A, 
                                          slong i, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong fmpz_mpoly_get_term_coeff_si(           const fmpz_mpoly_t A, 
                                          slong i, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_term_coeff_fmpz(fmpz_mpoly_t A, 
                          slong i, const fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_term_coeff_ui(fmpz_mpoly_t A,
                                 slong i, ulong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_term_coeff_si(fmpz_mpoly_t A,
                                 slong i, slong c, const fmpz_mpoly_ctx_t ctx);

FMPZ_MPOLY_INLINE
int fmpz_mpoly_term_exp_fits_ui(const fmpz_mpoly_t A, slong i,
                                                    const fmpz_mpoly_ctx_t ctx)
{
    return A->bits <= FLINT_BITS ? 1
                     : mpoly_term_exp_fits_ui(A->exps, A->bits, i, ctx->minfo);
}

FMPZ_MPOLY_INLINE
int fmpz_mpoly_term_exp_fits_si(const fmpz_mpoly_t A, slong i,
                                                    const fmpz_mpoly_ctx_t ctx)
{
    return A->bits <= FLINT_BITS ? 1
                     : mpoly_term_exp_fits_si(A->exps, A->bits, i, ctx->minfo);
}

FLINT_DLL void fmpz_mpoly_get_term_exp_fmpz(fmpz ** exp, const fmpz_mpoly_t A, 
                                          slong i, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_get_term_exp_ui(ulong * exp, const fmpz_mpoly_t A, 
                                          slong i, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL ulong fmpz_mpoly_get_term_var_exp_ui(const fmpz_mpoly_t A, slong i,
                                        slong var, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_term_exp_fmpz(fmpz_mpoly_t A,
                      slong i, fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_term_exp_ui(fmpz_mpoly_t A,
                       slong i, const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_get_term(fmpz_mpoly_t M, const fmpz_mpoly_t A,
                                          slong i, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_get_term_monomial(fmpz_mpoly_t M, const fmpz_mpoly_t A,
                                          slong i, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_push_term_fmpz_fmpz(fmpz_mpoly_t A,
               const fmpz_t c, fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_push_term_ui_fmpz(fmpz_mpoly_t A,
                      ulong c, fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_push_term_si_fmpz(fmpz_mpoly_t A,
                      slong c, fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_push_term_fmpz_ui(fmpz_mpoly_t A,
                const fmpz_t c, const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_push_term_ui_ui(fmpz_mpoly_t A,
                       ulong c, const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_push_term_si_ui(fmpz_mpoly_t A,
                       slong c, const ulong * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_sort_terms(fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_combine_like_terms(fmpz_mpoly_t A,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_reverse(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_assert_canonical(const fmpz_mpoly_t A,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_radix_sort1(fmpz_mpoly_t A, slong left, slong right,
                              flint_bitcnt_t pos, ulong cmpmask, ulong totalmask);

FLINT_DLL void _fmpz_mpoly_radix_sort(fmpz_mpoly_t A, slong left, slong right,
                                    flint_bitcnt_t pos, slong N, ulong * cmpmask);

FLINT_DLL void _fmpz_mpoly_push_exp_ffmpz(fmpz_mpoly_t A,
                                 const fmpz * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_push_exp_pfmpz(fmpz_mpoly_t A,
                               fmpz * const * exp, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_push_exp_ui(fmpz_mpoly_t A,
                                const ulong * exp, const fmpz_mpoly_ctx_t ctx);


/* Random generation *********************************************************/

FLINT_DLL void fmpz_mpoly_randtest_bound(fmpz_mpoly_t A, flint_rand_t state,
                        slong length, flint_bitcnt_t coeff_bits, ulong exp_bound,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_randtest_bounds(fmpz_mpoly_t A, flint_rand_t state,
                     slong length, flint_bitcnt_t coeff_bits, ulong * exp_bounds,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_randtest_bits(fmpz_mpoly_t A, flint_rand_t state,
                   slong length, flint_bitcnt_t coeff_bits, flint_bitcnt_t exp_bits,
                                                   const fmpz_mpoly_ctx_t ctx);


/* Addition/Subtraction ******************************************************/

FLINT_DLL void fmpz_mpoly_add_fmpz(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                   const fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_add_ui(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                          ulong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_add_si(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                          slong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_sub_fmpz(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                   const fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_sub_ui(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                          ulong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_sub_si(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                          slong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_add(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                             const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong _fmpz_mpoly_add(fmpz * poly1, ulong * exps1,
                 const fmpz * poly2, const ulong * exps2, slong len2,
                 const fmpz * poly3, const ulong * exps3, slong len3, slong N,
                                                        const ulong * cmpmask);

FLINT_DLL void fmpz_mpoly_sub(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                             const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong _fmpz_mpoly_sub(fmpz * poly1, ulong * exps1,
                 const fmpz * poly2, const ulong * exps2, slong len2,
                 const fmpz * poly3, const ulong * exps3, slong len3, slong N,
                                                        const ulong * cmpmask);


/* Scalar operations *********************************************************/

FLINT_DLL void fmpz_mpoly_neg(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_scalar_mul_fmpz(fmpz_mpoly_t A,
             const fmpz_mpoly_t B, const fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_scalar_mul_si(fmpz_mpoly_t A,
                    const fmpz_mpoly_t B, slong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_scalar_mul_ui(fmpz_mpoly_t A,
                    const fmpz_mpoly_t B, ulong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_scalar_divexact_fmpz(fmpz_mpoly_t A,
             const fmpz_mpoly_t B, const fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_scalar_divexact_si(fmpz_mpoly_t A,
                    const fmpz_mpoly_t B, slong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_scalar_divexact_ui(fmpz_mpoly_t A,
                    const fmpz_mpoly_t B, ulong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_scalar_divides_fmpz(fmpz_mpoly_t A,
             const fmpz_mpoly_t B, const fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_scalar_divides_si(fmpz_mpoly_t A,
                    const fmpz_mpoly_t B, slong c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_scalar_divides_ui(fmpz_mpoly_t A,
                    const fmpz_mpoly_t B, ulong c, const fmpz_mpoly_ctx_t ctx);


/* Differentiation/Integration ***********************************************/

FLINT_DLL void fmpz_mpoly_derivative(fmpz_mpoly_t A,
                  const fmpz_mpoly_t B, slong var, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_integral(fmpz_mpoly_t A, fmpz_t scale,
                  const fmpz_mpoly_t B, slong var, const fmpz_mpoly_ctx_t ctx);


/* Evaluation ****************************************************************/

FLINT_DLL void fmpz_mpoly_evaluate_all_fmpz(fmpz_t ev, const fmpz_mpoly_t A,
                              fmpz * const * vals, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_evaluate_one_fmpz(fmpz_mpoly_t A,
                           const fmpz_mpoly_t B, slong var, const fmpz_t val,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_compose_fmpz_poly(fmpz_poly_t A,
                         const fmpz_mpoly_t B, fmpz_poly_struct * const * C,
                                                  const fmpz_mpoly_ctx_t ctxB);

FLINT_DLL void fmpz_mpoly_compose_fmpz_mpoly(fmpz_mpoly_t A,
                   const fmpz_mpoly_t B, fmpz_mpoly_struct * const * C,
                    const fmpz_mpoly_ctx_t ctxB, const fmpz_mpoly_ctx_t ctxAC);


/* Multiplication ************************************************************/

FLINT_DLL void fmpz_mpoly_mul(fmpz_mpoly_t A,
       const fmpz_mpoly_t B, const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_mul_threaded(fmpz_mpoly_t A,
       const fmpz_mpoly_t B, const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx,
                                                           slong thread_limit);

FLINT_DLL void fmpz_mpoly_mul_johnson(fmpz_mpoly_t A,
       const fmpz_mpoly_t B, const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_mul_heap_threaded(fmpz_mpoly_t A,
       const fmpz_mpoly_t B, const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx,
                                                           slong thread_limit);

FLINT_DLL int fmpz_mpoly_mul_array(fmpz_mpoly_t A, 
       const fmpz_mpoly_t B, const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_mul_array_threaded(fmpz_mpoly_t A,
       const fmpz_mpoly_t B, const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx,
                                                           slong thread_limit);

FLINT_DLL int fmpz_mpoly_mul_dense(fmpz_mpoly_t A, 
       const fmpz_mpoly_t B, const fmpz_mpoly_t C, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong _fmpz_mpoly_mul_johnson(fmpz ** poly1, ulong ** exp1, slong * alloc,
                 const fmpz * poly2, const ulong * exp2, slong len2,
                 const fmpz * poly3, const ulong * exp3, slong len3,
                             flint_bitcnt_t bits, slong N, const ulong * cmpmask);

FLINT_DLL void _fmpz_mpoly_mul_johnson_maxfields(fmpz_mpoly_t A,
                                 const fmpz_mpoly_t B, fmpz * maxBfields,
                                 const fmpz_mpoly_t C, fmpz * maxCfields,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_mul_heap_threaded_maxfields(fmpz_mpoly_t A,
           const fmpz_mpoly_t B, fmpz * maxBfields,
           const fmpz_mpoly_t C, fmpz * maxCfields, const fmpz_mpoly_ctx_t ctx,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL int _fmpz_mpoly_mul_array_DEG(fmpz_mpoly_t A,
                                 const fmpz_mpoly_t B, fmpz * maxBfields,
                                 const fmpz_mpoly_t C, fmpz * maxCfields,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int _fmpz_mpoly_mul_array_LEX(fmpz_mpoly_t A,
                                 const fmpz_mpoly_t B, fmpz * maxBfields,
                                 const fmpz_mpoly_t C, fmpz * maxCfields,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int _fmpz_mpoly_mul_array_threaded_DEG(fmpz_mpoly_t A,
           const fmpz_mpoly_t B, fmpz * maxBfields,
           const fmpz_mpoly_t C, fmpz * maxCfields, const fmpz_mpoly_ctx_t ctx,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL int _fmpz_mpoly_mul_array_threaded_LEX(fmpz_mpoly_t A,
           const fmpz_mpoly_t B, fmpz * maxBfields,
           const fmpz_mpoly_t C, fmpz * maxCfields, const fmpz_mpoly_ctx_t ctx,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL int _fmpz_mpoly_mul_dense(fmpz_mpoly_t P,
                                 const fmpz_mpoly_t A, fmpz * maxAfields,
                                 const fmpz_mpoly_t B, fmpz * maxBfields,
                                                   const fmpz_mpoly_ctx_t ctx);

/* Powering ******************************************************************/

FLINT_DLL void fmpz_mpoly_pow_fmpz(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                  const fmpz_t k, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_pow_ui(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                          ulong k, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_pow_fps(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                          ulong k, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong _fmpz_mpoly_pow_fps(fmpz ** poly1, ulong ** exp1,
                slong * alloc, const fmpz * poly2, const ulong * exp2, 
        slong len2, ulong k, flint_bitcnt_t bits, slong N, const ulong * cmpmask);


/* Division ******************************************************************/

FLINT_DLL int fmpz_mpoly_divides(fmpz_mpoly_t Q,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_divides_threaded(fmpz_mpoly_t Q,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                                                           slong thread_limit);

FLINT_DLL int fmpz_mpoly_divides_monagan_pearce(fmpz_mpoly_t Q,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_divides_heap_threaded(fmpz_mpoly_t Q,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                                                           slong thread_limit);

FLINT_DLL int _fmpz_mpoly_divides_heap_threaded(fmpz_mpoly_t Q,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                              thread_pool_handle * handles, slong num_handles);

FLINT_DLL slong _fmpz_mpoly_divides_array(fmpz ** poly1, ulong ** exp1,
         slong * alloc, const fmpz * poly2, const ulong * exp2, slong len2,
                        const fmpz * poly3, const ulong * exp3, slong len3,
                                         slong * mults, slong num, slong bits);

FLINT_DLL int fmpz_mpoly_divides_array(fmpz_mpoly_t poly1,
                  const fmpz_mpoly_t poly2, const fmpz_mpoly_t poly3, 
                                                   const fmpz_mpoly_ctx_t ctx);


FLINT_DLL int mpoly_divides_select_exps(fmpz_mpoly_t S, fmpz_mpoly_ctx_t zctx,
                                slong nworkers, ulong * Aexp, slong Alen,
                                   ulong * Bexp, slong Blen, flint_bitcnt_t bits);

FLINT_DLL slong _fmpz_mpoly_divides_monagan_pearce(fmpz ** poly1,
                      ulong ** exp1, slong * alloc, const fmpz * poly2,
                    const ulong * exp2, slong len2, const fmpz * poly3,
                    const ulong * exp3, slong len3, flint_bitcnt_t bits, slong N,
                                                        const ulong * cmpmask);

FLINT_DLL void fmpz_mpoly_divrem(fmpz_mpoly_t Q, fmpz_mpoly_t R,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_quasidivrem(fmpz_t scale, fmpz_mpoly_t Q,
     fmpz_mpoly_t R, const fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_div(fmpz_mpoly_t Q, const fmpz_mpoly_t A,
                             const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_quasidiv(fmpz_t scale, fmpz_mpoly_t Q,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_divrem_ideal(fmpz_mpoly_struct ** Q,
     fmpz_mpoly_t R, const fmpz_mpoly_t A, fmpz_mpoly_struct * const * B,
                                        slong len, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_quasidivrem_ideal(fmpz_t scale,
     fmpz_mpoly_struct ** Q, fmpz_mpoly_t R, const fmpz_mpoly_t A,
         fmpz_mpoly_struct * const * B, slong len, const fmpz_mpoly_ctx_t ctx);


FLINT_DLL slong _fmpz_mpoly_div_monagan_pearce(fmpz ** polyq,
           ulong ** expq, slong * allocq, const fmpz * poly2,
   const ulong * exp2, slong len2, const fmpz * poly3, const ulong * exp3, 
                       slong len3, slong bits, slong N, const ulong * cmpmask);

FLINT_DLL void fmpz_mpoly_div_monagan_pearce(fmpz_mpoly_t q,
                     const fmpz_mpoly_t poly2, const fmpz_mpoly_t poly3,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong _fmpz_mpoly_divrem_monagan_pearce(slong * lenr,
  fmpz ** polyq, ulong ** expq, slong * allocq, fmpz ** polyr,
                  ulong ** expr, slong * allocr, const fmpz * poly2,
   const ulong * exp2, slong len2, const fmpz * poly3, const ulong * exp3, 
                       slong len3, slong bits, slong N, const ulong * cmpmask);

FLINT_DLL void fmpz_mpoly_divrem_monagan_pearce(fmpz_mpoly_t q, fmpz_mpoly_t r,
                  const fmpz_mpoly_t poly2, const fmpz_mpoly_t poly3,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong _fmpz_mpoly_divrem_array(slong * lenr,
       fmpz ** polyq, ulong ** expq, slong * allocq,
              fmpz ** polyr, ulong ** expr, slong * allocr, 
                const fmpz * poly2, const ulong * exp2, slong len2, 
        const fmpz * poly3, const ulong * exp3, slong len3, slong * mults, 
                                                        slong num, slong bits);

FLINT_DLL int fmpz_mpoly_divrem_array(fmpz_mpoly_t q, fmpz_mpoly_t r,
                    const fmpz_mpoly_t poly2, const fmpz_mpoly_t poly3, 
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_quasidivrem_heap(fmpz_t scale,
                        fmpz_mpoly_t q, fmpz_mpoly_t r,
                  const fmpz_mpoly_t poly2, const fmpz_mpoly_t poly3,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_quasidiv_heap(fmpz_t scale, fmpz_mpoly_t q,
                  const fmpz_mpoly_t poly2, const fmpz_mpoly_t poly3,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong
_fmpz_mpoly_divrem_ideal_monagan_pearce(fmpz_mpoly_struct ** polyq,
       fmpz ** polyr, ulong ** expr, slong * allocr, const fmpz * poly2,
          const ulong * exp2, slong len2, fmpz_mpoly_struct * const * poly3,
                        ulong * const * exp3, slong len, slong N, slong bits,
                            const fmpz_mpoly_ctx_t ctx, const ulong * cmpmask);

FLINT_DLL void
fmpz_mpoly_divrem_ideal_monagan_pearce(fmpz_mpoly_struct ** q, fmpz_mpoly_t r,
    const fmpz_mpoly_t poly2, fmpz_mpoly_struct * const * poly3, slong len,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void
fmpz_mpoly_quasidivrem_ideal_heap(fmpz_t scale,
                                 fmpz_mpoly_struct ** q, fmpz_mpoly_t r,
                const fmpz_mpoly_t poly2, fmpz_mpoly_struct * const * poly3,
                                        slong len, const fmpz_mpoly_ctx_t ctx);

/* GCD ***********************************************************************/

FLINT_DLL void fmpz_mpoly_term_content(fmpz_mpoly_t M, const fmpz_mpoly_t A,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_gcd(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_gcd_threaded(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                                                           slong thread_limit);

FLINT_DLL int _fmpz_mpoly_gcd(fmpz_mpoly_t G, flint_bitcnt_t Gbits,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL int _fmpz_mpoly_gcd_monomial(fmpz_mpoly_t G, flint_bitcnt_t Gbits,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int _fmpz_mpoly_gcd_monomial_cofactors_sp(
                         fmpz_mpoly_t G, flint_bitcnt_t Gbits,
         const fmpz_mpoly_t A, const ulong * Amax_exp , const ulong * Amin_exp,
         const fmpz_mpoly_t B, const ulong * Bmax_exp , const ulong * Bmin_exp,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int _fmpz_mpoly_gcd_monomial_cofactors(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_deflation(fmpz * shift, fmpz * stride,
                             const fmpz_mpoly_t A, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_deflate(fmpz_mpoly_t A, const fmpz_mpoly_t B,
          const fmpz * shift, const fmpz * stride, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_inflate(fmpz_mpoly_t A, const fmpz_mpoly_t B,
          const fmpz * shift, const fmpz * stride, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_gcd_prs(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_gcd_brown(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_gcd_zippel(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_gcd_berlekamp_massey(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_gcd_brown_threaded(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                                                           slong thread_limit);

FLINT_DLL int fmpz_mpoly_gcd_berlekamp_massey_threaded(fmpz_mpoly_t G,
       const fmpz_mpoly_t A, const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                                                           slong thread_limit);

/******************************************************************************

   Internal functions (guaranteed to change without notice)

******************************************************************************/

FLINT_DLL void _fmpz_mpoly_to_fmpz_poly_deflate(fmpz_poly_t A,
                         const fmpz_mpoly_t B, slong var, const ulong * Bshift,
                            const ulong * Bstride, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_from_fmpz_poly_inflate(fmpz_mpoly_t A,
       flint_bitcnt_t Abits, const fmpz_poly_t B, slong var, const ulong * Ashift,
                            const ulong * Astride, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_repack_bits(fmpz_mpoly_t A, const fmpz_mpoly_t B,
                                flint_bitcnt_t Abits, const fmpz_mpoly_ctx_t ctx);

typedef struct _fmpz_mpoly_stripe_struct
{
    char * big_mem;
    slong big_mem_alloc;
    slong N;
    flint_bitcnt_t bits;
    const ulong * cmpmask;
    slong * startidx;
    slong * endidx;
    ulong * emin;
    ulong * emax;
    flint_bitcnt_t coeff_bits;
    int upperclosed;
    int flint_small;
} fmpz_mpoly_stripe_struct;

typedef fmpz_mpoly_stripe_struct fmpz_mpoly_stripe_t[1];


/* Univariates ***************************************************************/

FLINT_DLL void fmpz_mpoly_univar_init(fmpz_mpoly_univar_t poly,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_univar_clear(fmpz_mpoly_univar_t poly,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_univar_swap(fmpz_mpoly_univar_t poly1,
                        fmpz_mpoly_univar_t poly2, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_univar_fit_length(fmpz_mpoly_univar_t poly,
                                     slong length, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_univar_print_pretty(const fmpz_mpoly_univar_t poly,
                                  const char ** x, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_univar_assert_canonical(fmpz_mpoly_univar_t poly,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_from_univar(fmpz_mpoly_t poly1,
                  const fmpz_mpoly_univar_t poly2, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_from_univar_bits(fmpz_mpoly_t poly1, flint_bitcnt_t bits1,
                  const fmpz_mpoly_univar_t poly2, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_to_univar(fmpz_mpoly_univar_t poly1,
              const fmpz_mpoly_t poly2, slong var, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_univar_equal(fmpz_mpoly_univar_t poly1,
                  const fmpz_mpoly_univar_t poly2, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_univar_add(fmpz_mpoly_univar_t poly1,
            const fmpz_mpoly_univar_t poly2, const fmpz_mpoly_univar_t poly3,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_univar_mul(fmpz_mpoly_univar_t poly1,
            const fmpz_mpoly_univar_t poly2, const fmpz_mpoly_univar_t poly3,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_to_fmpz_poly(fmpz_poly_t poly1, slong * shift1,
              const fmpz_mpoly_t poly2, slong var, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_from_fmpz_poly(fmpz_mpoly_t poly1,
        const fmpz_poly_t poly2, slong shift2, slong var,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_univar_derivative(fmpz_mpoly_univar_t poly1,
                  const fmpz_mpoly_univar_t poly2, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_univar_prem(fmpz_mpoly_univar_t polyA,
            const fmpz_mpoly_univar_t polyB, fmpz_mpoly_univar_t polyC,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_univar_pgcd(fmpz_mpoly_univar_t poly1,
            const fmpz_mpoly_univar_t polyP, const fmpz_mpoly_univar_t polyQ,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_univar_pgcd_ducos(fmpz_mpoly_univar_t poly1,
            const fmpz_mpoly_univar_t polyP, const fmpz_mpoly_univar_t polyQ,
                                                   const fmpz_mpoly_ctx_t ctx);

/* mpolyd ********************************************************************/

typedef struct
{
    slong nvars;
    slong * perm;
} fmpz_mpolyd_ctx_struct;

typedef fmpz_mpolyd_ctx_struct fmpz_mpolyd_ctx_t[1];


FLINT_DLL void fmpz_mpolyd_init(fmpz_mpolyd_t poly, slong nvars);

FLINT_DLL void fmpz_mpolyd_fit_length(fmpz_mpolyd_t poly, slong len);

FLINT_DLL void fmpz_mpolyd_clear(fmpz_mpolyd_t poly);

/* mpolyu ********************************************************************/

FLINT_DLL int fmpz_mpolyu_is_canonical(const fmpz_mpolyu_t A, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_init(fmpz_mpolyu_t A, flint_bitcnt_t bits,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_clear(fmpz_mpolyu_t A, const fmpz_mpoly_ctx_t uctx);

FLINT_DLL void fmpz_mpolyu_swap(fmpz_mpolyu_t A, fmpz_mpolyu_t B,
                                                 const fmpz_mpoly_ctx_t uctx);

FLINT_DLL void fmpz_mpolyu_zero(fmpz_mpolyu_t A, const fmpz_mpoly_ctx_t uctx);

FLINT_DLL void fmpz_mpolyu_print_pretty(const fmpz_mpolyu_t poly,
                                  const char ** x, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyuu_print_pretty(const fmpz_mpolyu_t poly,
                 const char ** x, slong nmainvars, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_fit_length(fmpz_mpolyu_t A, slong length,
                                                 const fmpz_mpoly_ctx_t uctx);

FLINT_DLL void fmpz_mpolyu_one(fmpz_mpolyu_t A, const fmpz_mpoly_ctx_t uctx);

FLINT_DLL void fmpz_mpolyu_set(fmpz_mpolyu_t A, const fmpz_mpolyu_t B,
                                                  const fmpz_mpoly_ctx_t uctx);

FLINT_DLL void fmpz_mpoly_to_mpolyu_perm_deflate(
                                fmpz_mpolyu_t A, const fmpz_mpoly_ctx_t uctx,
                            const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                                     const slong * perm, const ulong * shift,
                                 const ulong * stride, const ulong * maxexps,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL void fmpz_mpoly_from_mpolyu_perm_inflate(
               fmpz_mpoly_t A, flint_bitcnt_t Abits, const fmpz_mpoly_ctx_t ctx,
                         const fmpz_mpolyu_t B,  const fmpz_mpoly_ctx_t uctx,
                const slong * perm, const ulong * shift, const ulong * stride);

FLINT_DLL void fmpz_mpoly_to_mpolyuu_perm_deflate(
                                fmpz_mpolyu_t A, const fmpz_mpoly_ctx_t uctx,
                            const fmpz_mpoly_t B, const fmpz_mpoly_ctx_t ctx,
                                     const slong * perm, const ulong * shift,
                                 const ulong * stride, const ulong * maxexps,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL void fmpz_mpoly_from_mpolyuu_perm_inflate(
               fmpz_mpoly_t A, flint_bitcnt_t Abits, const fmpz_mpoly_ctx_t ctx,
                          const fmpz_mpolyu_t B, const fmpz_mpoly_ctx_t uctx,
                const slong * perm, const ulong * shift, const ulong * stride);

FLINT_DLL int fmpz_mpolyuu_divides(fmpz_mpolyu_t Q, const fmpz_mpolyu_t A,
          const fmpz_mpolyu_t B, slong main_nvars, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpolyuu_divides_threaded(fmpz_mpolyu_t Q, const fmpz_mpolyu_t A,
          const fmpz_mpolyu_t B, slong main_nvars, const fmpz_mpoly_ctx_t ctx,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL int fmpz_mpolyu_divides(fmpz_mpolyu_t A, fmpz_mpolyu_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_fmpz_content(fmpz_t c, fmpz_mpolyu_t A,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_mul_fmpz(fmpz_mpolyu_t A, fmpz_mpolyu_t B,
                                         fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_divexact_fmpz(fmpz_mpolyu_t A, fmpz_mpolyu_t B,
                                         fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_divexact_mpoly(fmpz_mpolyu_t A, fmpz_mpolyu_t B,
                        int saveB, fmpz_mpoly_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_mul_mpoly(fmpz_mpolyu_t A, fmpz_mpolyu_t B,
                                   fmpz_mpoly_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_shift_right(fmpz_mpolyu_t A, ulong s);

FLINT_DLL void fmpz_mpolyu_shift_left(fmpz_mpolyu_t A, ulong s);

FLINT_DLL void fmpz_mpolyu_content_fmpz(fmpz_t g, const fmpz_mpolyu_t A,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpolyu_content_mpoly(fmpz_mpoly_t g, const fmpz_mpolyu_t A,
                                                   const fmpz_mpoly_ctx_t ctx,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL void fmpz_mpolyu_height(fmpz_t max,
                            const fmpz_mpolyu_t A, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_heights(fmpz_t max, fmpz_t sum,
                            const fmpz_mpolyu_t A, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpolyu_gcd_brown(fmpz_mpolyu_t G, fmpz_mpolyu_t Abar,
                    fmpz_mpolyu_t Bbar, fmpz_mpolyu_t A, fmpz_mpolyu_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpolyu_gcd_brown_threaded(fmpz_mpolyu_t G,
    fmpz_mpolyu_t Abar, fmpz_mpolyu_t Bbar, fmpz_mpolyu_t A, fmpz_mpolyu_t B,
                                                   const fmpz_mpoly_ctx_t ctx,
                        const thread_pool_handle * handles, slong num_handles);

FLINT_DLL int fmpz_mpolyu_gcdm_zippel(fmpz_mpolyu_t G,
                 fmpz_mpolyu_t A, fmpz_mpolyu_t B, const fmpz_mpoly_ctx_t ctx,
                                mpoly_zipinfo_t zinfo, flint_rand_t randstate);

FLINT_DLL int fmpz_mpolyuu_gcd_berlekamp_massey(fmpz_mpolyu_t G,
    const fmpz_mpolyu_t A, const fmpz_mpolyu_t B, const fmpz_mpoly_t Gamma,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpolyuu_gcd_berlekamp_massey_threaded(fmpz_mpolyu_t G,
      const fmpz_mpolyu_t A, const fmpz_mpolyu_t B, const fmpz_mpoly_t Gamma,
                                                   const fmpz_mpoly_ctx_t ctx,
                        const thread_pool_handle * handles, slong num_handles);

FMPZ_MPOLY_INLINE fmpz * fmpz_mpoly_leadcoeff(const fmpz_mpoly_t A)
{
    FLINT_ASSERT(A->length > 0);
    return A->coeffs + 0;
}

FMPZ_MPOLY_INLINE fmpz * fmpz_mpolyu_leadcoeff(const fmpz_mpolyu_t A)
{
    FLINT_ASSERT(A->length > 0);
    return fmpz_mpoly_leadcoeff(A->coeffs + 0);
}

/* gcd_helper_eval_interp ****************************************************/

FLINT_DLL void fmpz_mpolyu_intp_reduce_p(
    nmod_mpolyu_t Ap,
    const nmod_mpoly_ctx_t ctxp,
    fmpz_mpolyu_t A,
    const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_intp_lift_p(
    fmpz_mpolyu_t A,
    const fmpz_mpoly_ctx_t ctx,
    nmod_mpolyu_t Ap,
    const nmod_mpoly_ctx_t ctxp);

FLINT_DLL int fmpz_mpolyu_intp_mcrt_p(
    flint_bitcnt_t * coeffbits,
    fmpz_mpolyu_t H,
    const fmpz_mpoly_ctx_t ctx,
    const fmpz_t m,
    const nmod_mpolyu_t A,
    const nmod_mpoly_ctx_t ctxp);

FLINT_DLL void fmpz_mpolyu_intp_reduce_p_mpolyun(
    nmod_mpolyun_t E,
    const nmod_mpoly_ctx_t pctx,
    const fmpz_mpolyu_t A,
    const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_intp_lift_p_mpolyun(
    fmpz_mpolyu_t A,
    const fmpz_mpoly_ctx_t ctx,
    const nmod_mpolyun_t B,
    const nmod_mpoly_ctx_t pctx);

FLINT_DLL int fmpz_mpolyu_intp_crt_p_mpolyun(
    fmpz_mpolyu_t F,
    fmpz_mpolyu_t T,
    const fmpz_mpoly_ctx_t ctx,
    fmpz_t modulus,
    nmod_mpolyun_t A,
    const nmod_mpoly_ctx_t pctx);

/* fmpz_mod_mpoly types - should be in a separate module *********************/

/*
    fmpz_mod_mpolyn_t
    sparse multivariates with fmpz_mod_poly_t coefficients
        with LEX ordering
*/
typedef struct
{
   fmpz_mod_poly_struct * coeffs;
   ulong * exps;
   slong alloc;
   slong length;
   slong bits;
} fmpz_mod_mpolyn_struct;

typedef fmpz_mod_mpolyn_struct fmpz_mod_mpolyn_t[1];

/*
    fmpz_mod_mpolyun_t
    sparse univariates with fmpz_mod_mpolyn_t coefficients
        with uniform bits and LEX ordering
*/
typedef struct
{
    fmpz_mod_mpolyn_struct * coeffs;
    ulong * exps;
    slong alloc;
    slong length;
    flint_bitcnt_t bits;   /* default bits to construct coeffs */
} fmpz_mod_mpolyun_struct;

typedef fmpz_mod_mpolyun_struct fmpz_mod_mpolyun_t[1];

FLINT_DLL void fmpz_mod_mpolyn_fit_length(fmpz_mod_mpolyn_t A,
         slong length, const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_fit_length(fmpz_mod_mpolyun_t A,
         slong length, const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_swap(fmpz_mod_mpolyun_t A, fmpz_mod_mpolyun_t B);

FLINT_DLL fmpz_mod_poly_struct * fmpz_mod_mpolyun_leadcoeff_ref(
                        fmpz_mod_mpolyun_t A,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_content_last(
                       fmpz_mod_poly_t a, const fmpz_mod_mpolyun_t B,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_divexact_last(
                        fmpz_mod_mpolyun_t A, const fmpz_mod_poly_t b,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL slong fmpz_mod_mpolyun_lastdeg(const fmpz_mod_mpolyun_t A,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_init(fmpz_mod_mpolyun_t A, flint_bitcnt_t bits,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_clear(fmpz_mod_mpolyun_t A,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL fmpz * fmpz_mod_mpolyun_leadcoeff_last_ref(const fmpz_mod_mpolyun_t A,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_one(fmpz_mod_mpolyun_t A,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_mul_last(fmpz_mod_mpolyun_t A, fmpz_mod_poly_t b,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_set_modulus(fmpz_mod_mpolyun_t A,
                                                   const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_scalar_mul_fmpz_mod(fmpz_mod_mpolyun_t A,
       const fmpz_t c, const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL int fmpz_mod_mpolyun_equal(
                    const fmpz_mod_mpolyun_t A, const fmpz_mod_mpolyun_t B,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL int fmpz_mod_mpolyun_gcd_brown_bivar(
    fmpz_mod_mpolyun_t G, fmpz_mod_mpolyun_t Abar, fmpz_mod_mpolyun_t Bbar,
    fmpz_mod_mpolyun_t A, fmpz_mod_mpolyun_t B,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_mpolyun_print_pretty(const fmpz_mod_mpolyun_t poly,
      const char ** x, const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

/* geobuckets ****************************************************************/
typedef struct fmpz_mpoly_geobucket
{
    fmpz_mpoly_struct polys[FLINT_BITS/2];
    slong length;
} fmpz_mpoly_geobucket_struct;

typedef fmpz_mpoly_geobucket_struct fmpz_mpoly_geobucket_t[1];

FLINT_DLL void fmpz_mpoly_geobucket_init(fmpz_mpoly_geobucket_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_clear(fmpz_mpoly_geobucket_t B,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_empty(fmpz_mpoly_t p,
                         fmpz_mpoly_geobucket_t B, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_print(fmpz_mpoly_geobucket_t B,
                                  const char ** x, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_fit_length(fmpz_mpoly_geobucket_t B,
                                          slong i, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_geobucket_fix(fmpz_mpoly_geobucket_t B, slong i,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_set(fmpz_mpoly_geobucket_t B,
                                   fmpz_mpoly_t p, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_add(fmpz_mpoly_geobucket_t B,
                                   fmpz_mpoly_t p, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_sub(fmpz_mpoly_geobucket_t B,
                                   fmpz_mpoly_t p, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_set_fmpz(fmpz_mpoly_geobucket_t B,
                                         fmpz_t c, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_gen(fmpz_mpoly_geobucket_t B, slong var,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_add_inplace(fmpz_mpoly_geobucket_t B1,
                        fmpz_mpoly_geobucket_t B2, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_sub_inplace(fmpz_mpoly_geobucket_t B1,
                        fmpz_mpoly_geobucket_t B2, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_neg_inplace(fmpz_mpoly_geobucket_t B1,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_mul_inplace(fmpz_mpoly_geobucket_t B1,
                        fmpz_mpoly_geobucket_t B2, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_pow_ui_inplace(fmpz_mpoly_geobucket_t B1,
                                          ulong k, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_geobucket_pow_fmpz_inplace(fmpz_mpoly_geobucket_t B1,
                                   const fmpz_t k, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpoly_geobucket_divides_inplace(fmpz_mpoly_geobucket_t B1,
                        fmpz_mpoly_geobucket_t B2, const fmpz_mpoly_ctx_t ctx);


/* Helpers for gcd_berlekamp_massey ******************************************/

typedef struct
{
    slong * degbounds;
    ulong * subdegs;
    fmpz_mod_discrete_log_pohlig_hellman_t dlogenv;
    nmod_discrete_log_pohlig_hellman_t dlogenv_sp;
} mpoly_bma_interpolate_ctx_struct;
typedef mpoly_bma_interpolate_ctx_struct mpoly_bma_interpolate_ctx_t[1];

FLINT_DLL void mpoly_bma_interpolate_ctx_init(mpoly_bma_interpolate_ctx_t Ictx,
                                                                  slong nvars);

FLINT_DLL void mpoly_bma_interpolate_ctx_clear(mpoly_bma_interpolate_ctx_t Ictx);

FLINT_DLL int nmod_mpoly_bma_get_fmpz_mpoly(fmpz_mpoly_t A,
     const fmpz_mpoly_ctx_t ctx, ulong alphashift, nmod_berlekamp_massey_t I,
              const mpoly_bma_interpolate_ctx_t Ictx, const nmodf_ctx_t fpctx);

/*
    nmod_mpoly "skeletons" - just the coefficients
*/
typedef struct
{
   mp_limb_t * coeffs;
   slong alloc;
   slong length;
} nmod_mpolyc_struct;

typedef nmod_mpolyc_struct nmod_mpolyc_t[1];

FLINT_DLL void nmod_mpolyc_init(nmod_mpolyc_t A);

FLINT_DLL void nmod_mpolyc_clear(nmod_mpolyc_t A);

FLINT_DLL void nmod_mpolyc_fit_length(nmod_mpolyc_t A, slong length);

typedef struct
{
   nmod_mpolyc_struct * coeffs;
   slong alloc;
   slong length;
} nmod_mpolycu_struct;

typedef nmod_mpolycu_struct nmod_mpolycu_t[1];

FLINT_DLL void nmod_mpolycu_init(nmod_mpolycu_t A);

FLINT_DLL void nmod_mpolycu_clear(nmod_mpolycu_t A);

FLINT_DLL void nmod_mpolycu_fit_length(nmod_mpolycu_t A, slong length);

/*
    fmpz_mod_mpoly "skeletons" - just the coefficients
*/

typedef struct
{
   fmpz * coeffs;
   slong alloc;
   slong length;
} fmpz_mpolyc_struct;

typedef fmpz_mpolyc_struct fmpz_mpolyc_t[1];

FLINT_DLL void fmpz_mpolyc_init(fmpz_mpolyc_t A);

FLINT_DLL void fmpz_mpolyc_clear(fmpz_mpolyc_t A);

FLINT_DLL void fmpz_mpolyc_fit_length(fmpz_mpolyc_t A, slong length);

typedef struct
{
   fmpz_mpolyc_struct * coeffs;
   slong alloc;
   slong length;
} fmpz_mpolycu_struct;

typedef fmpz_mpolycu_struct fmpz_mpolycu_t[1];

FLINT_DLL void fmpz_mpolycu_init(fmpz_mpolycu_t A);

FLINT_DLL void fmpz_mpolycu_clear(fmpz_mpolycu_t A);

FLINT_DLL void fmpz_mpolycu_fit_length(fmpz_mpolycu_t A, slong length);

FLINT_DLL void nmod_mpoly_bma_interpolate_alpha_powers(mp_limb_t * out,
  ulong w, const mpoly_bma_interpolate_ctx_t Ictx, const nmod_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mod_mpoly_bma_interpolate_alpha_powers(fmpz * out,
                     const fmpz_t w, const mpoly_bma_interpolate_ctx_t Ictx,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mpoly_set_skel(fmpz_mpolyc_t M, const fmpz_mpoly_t A,
   const fmpz * alpha, const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mpolyu_set_skel(fmpz_mpolycu_t M, const fmpz_mpolyu_t A,
   const fmpz * alpha, const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mpoly_copy_skel(fmpz_mpolyc_t M, const fmpz_mpolyc_t S);

FLINT_DLL void fmpz_mpolyu_copy_skel(fmpz_mpolycu_t M, const fmpz_mpolycu_t S);

FLINT_DLL void fmpz_mpoly_red_skel(fmpz_mpolyc_t Ared, const fmpz_mpoly_t A,
                                                   const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mpolyu_red_skel(fmpz_mpolycu_t Ared, const fmpz_mpolyu_t A,
                                                   const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mpoly_use_skel_mul(fmpz_t eval, fmpz_mpolyc_t Ared,
           fmpz_mpolyc_t M, const fmpz_mpolyc_t S, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mpolyuu_use_skel_mul(fmpz_mod_mpolyun_t E,
                           const fmpz_mpolyu_t A, fmpz_mpolycu_t Ared,
                           fmpz_mpolycu_t M, const fmpz_mpolycu_t S,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL int fmpz_mod_bma_get_fmpz_mpoly(fmpz_mpoly_t A,
          const fmpz_t alphashift, fmpz_mod_berlekamp_massey_t I,
          const mpoly_bma_interpolate_ctx_t Ictx, const fmpz_mpoly_ctx_t ctx,
                                                   const fmpz_mod_ctx_t fpctx);

typedef struct {
    nmod_berlekamp_massey_struct * coeffs;
    ulong * exps;
    slong length;
    slong alloc;
    slong pointcount;
} nmod_bma_mpoly_struct;

typedef nmod_bma_mpoly_struct nmod_bma_mpoly_t[1];

FLINT_DLL void nmod_bma_mpoly_init(nmod_bma_mpoly_t A);

FLINT_DLL void nmod_bma_mpoly_reset_prime(nmod_bma_mpoly_t A,
                                                      const nmodf_ctx_t fpctx);

FLINT_DLL void nmod_bma_mpoly_clear(nmod_bma_mpoly_t A);

FLINT_DLL void nmod_bma_mpoly_print(const nmod_bma_mpoly_t A);

FLINT_DLL void nmod_bma_mpoly_fit_length(nmod_bma_mpoly_t A, slong length,
                                                      const nmodf_ctx_t fpctx);

FLINT_DLL void nmod_bma_mpoly_zero(nmod_bma_mpoly_t L);

FLINT_DLL int nmod_bma_mpoly_reduce(nmod_bma_mpoly_t L);

FLINT_DLL void nmod_bma_mpoly_add_point(nmod_bma_mpoly_t L,
                              const nmod_mpolyun_t A, const nmodf_ctx_t fpctx);

FLINT_DLL int nmod_bma_mpoly_get_fmpz_mpolyu(fmpz_mpolyu_t A,
      const fmpz_mpoly_ctx_t ctx, ulong alphashift, const nmod_bma_mpoly_t L,
              const mpoly_bma_interpolate_ctx_t Ictx, const nmodf_ctx_t fpctx);

typedef struct {
    fmpz_mod_berlekamp_massey_struct * coeffs;
    ulong * exps;
    slong length;
    slong alloc;
    slong pointcount;
} fmpz_mod_bma_mpoly_struct;

typedef fmpz_mod_bma_mpoly_struct fmpz_mod_bma_mpoly_t[1];

FLINT_DLL void fmpz_mod_bma_mpoly_init(fmpz_mod_bma_mpoly_t A);

FLINT_DLL void fmpz_mod_bma_mpoly_reset_prime(
    fmpz_mod_bma_mpoly_t A,
    const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_bma_mpoly_clear(fmpz_mod_bma_mpoly_t A);

FLINT_DLL void fmpz_mod_bma_mpoly_print(
    fmpz_mod_bma_mpoly_t A,
    const mpoly_bma_interpolate_ctx_t Ictx);

FLINT_DLL void fmpz_mod_bma_mpoly_fit_length(
    fmpz_mod_bma_mpoly_t A,
    slong length,
    const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mod_bma_mpoly_zero(fmpz_mod_bma_mpoly_t L);

FLINT_DLL int fmpz_mod_bma_mpoly_reduce(fmpz_mod_bma_mpoly_t L);

FLINT_DLL void fmpz_mod_bma_mpoly_add_point(fmpz_mod_bma_mpoly_t L,
                    const fmpz_mod_mpolyun_t A, const fmpz_mpoly_ctx_t ctx,
                                                   const fmpz_mod_ctx_t fpctx);

FLINT_DLL int fmpz_mod_bma_mpoly_get_fmpz_mpolyu(fmpz_mpolyu_t A,
            const fmpz_t alphashift, const fmpz_mod_bma_mpoly_t L,
          const mpoly_bma_interpolate_ctx_t Ictx, const fmpz_mpoly_ctx_t ctx,
                                                   const fmpz_mod_ctx_t fpctx);

FLINT_DLL ulong fmpz_mod_mpolyun_bidegree(const fmpz_mod_mpolyun_t A);

FLINT_DLL ulong nmod_mpolyun_bidegree(const nmod_mpolyun_t A);

FLINT_DLL void fmpz_mpoly_eval_fmpz_mod(fmpz_t eval, const fmpz_mpoly_t A,
   const fmpz * alpha, const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mpolyu_symmetrize_coeffs(fmpz_mpolyu_t A,
                       const fmpz_mpoly_ctx_t ctx, const fmpz_mod_ctx_t fpctx);

FLINT_DLL void fmpz_mpolyuu_eval_fmpz_mod(fmpz_mod_mpolyun_t E,
      const fmpz_mpolyu_t A, const fmpz * alpha, const fmpz_mpoly_ctx_t ctx,
                                                   const fmpz_mod_ctx_t fpctx);

FLINT_DLL mp_limb_t fmpz_mpoly_eval_nmod(const nmodf_ctx_t fpctx,
    const fmpz_mpoly_t A, const mp_limb_t * alpha, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyuu_eval_nmod(nmod_mpolyun_t E,
                     const nmod_mpoly_ctx_t ctx_sp, const fmpz_mpolyu_t A,
                          const mp_limb_t * alpha, const fmpz_mpoly_ctx_t ctx);

typedef struct {
    slong mlength;
    slong malloc;
    mp_limb_t * coeffs;
    mp_limb_t * monomials;
    slong ealloc;
    mp_limb_t * evals;
} nmod_zip_struct;

typedef nmod_zip_struct nmod_zip_t[1];

FLINT_DLL void nmod_zip_init(nmod_zip_t Z);

FLINT_DLL void nmod_zip_clear(nmod_zip_t Z);

FLINT_DLL void nmod_zip_set_lengths(nmod_zip_t A, slong mlength, slong elength);

FLINT_DLL void nmod_zip_print(const nmod_zip_t Z, slong elength);

typedef struct {
    nmod_zip_struct * coeffs;
    ulong * exps;
    slong length;
    slong alloc;
    slong pointcount;
} nmod_zip_mpolyu_struct;

typedef nmod_zip_mpolyu_struct nmod_zip_mpolyu_t[1];

FLINT_DLL void nmod_zip_mpolyu_init(nmod_zip_mpolyu_t Z);

FLINT_DLL void nmod_zip_mpolyu_clear(nmod_zip_mpolyu_t Z);

FLINT_DLL void nmod_zip_mpolyu_fit_length(nmod_zip_mpolyu_t A, slong length);

FLINT_DLL void nmod_zip_mpolyu_fit_poly(nmod_zip_mpolyu_t Z, fmpz_mpolyu_t H,
                                                            slong eval_length);

FLINT_DLL void nmod_mpoly_set_skel(nmod_mpolyc_t S,
                     const nmod_mpoly_ctx_t ctx_sp, const fmpz_mpoly_t A,
                          const mp_limb_t * alpha, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void nmod_zip_mpolyu_set_skel(nmod_zip_mpolyu_t Z,
                       const nmod_mpoly_ctx_t ctx_sp, const fmpz_mpolyu_t A,
                          const mp_limb_t * alpha, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void nmod_zip_mpolyuu_print(const nmod_zip_mpolyu_t A);

FLINT_DLL int nmod_zip_mpolyuu_add_point(nmod_zip_mpolyu_t L,
                                                       const nmod_mpolyun_t A);

FLINT_DLL void nmod_mpolyu_set_skel(nmod_mpolycu_t S,
                        const nmod_mpoly_ctx_t ctx_sp, const fmpz_mpolyu_t A,
                          const mp_limb_t * alpha, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void nmod_mpoly_red_skel(nmod_mpolyc_t Ared, const fmpz_mpoly_t A,
                                                      const nmodf_ctx_t fpctx);

FLINT_DLL void nmod_mpolyu_red_skel(nmod_mpolycu_t Ared, const fmpz_mpolyu_t A,
                                                      const nmodf_ctx_t fpctx);

FLINT_DLL void nmod_mpoly_copy_skel(nmod_mpolyc_t M, const nmod_mpolyc_t S);

FLINT_DLL void nmod_mpolyu_copy_skel(nmod_mpolycu_t M, const nmod_mpolycu_t S);

FLINT_DLL void nmod_mpoly_pow_skel(nmod_mpolyc_t M, const nmod_mpolyc_t S,
                                          ulong k, const nmod_mpoly_ctx_t ctx);

FLINT_DLL void nmod_mpolyu_pow_skel(nmod_mpolycu_t M, const nmod_mpolycu_t S,
                                          ulong k, const nmod_mpoly_ctx_t ctx);

FLINT_DLL mp_limb_t nmod_mpoly_use_skel_mul(const nmod_mpolyc_t Ared,
  nmod_mpolyc_t Acur, const nmod_mpolyc_t Ainc, const nmod_mpoly_ctx_t ctx_sp);

FLINT_DLL void nmod_mpolyuu_use_skel_mul(nmod_mpolyun_t E,
     const fmpz_mpolyu_t A, const nmod_mpolycu_t Ared, nmod_mpolycu_t Acur,
                     const nmod_mpolycu_t Ainc, const nmod_mpoly_ctx_t ctx_sp);

typedef enum {
    nmod_zip_find_coeffs_good,
    nmod_zip_find_coeffs_no_match,
    nmod_zip_find_coeffs_non_invertible
} nmod_zip_find_coeffs_ret_t;

FLINT_DLL nmod_zip_find_coeffs_ret_t nmod_zip_find_coeffs(nmod_zip_t Z,
               nmod_poly_t master, slong pointcount, const nmodf_ctx_t ffinfo);

FLINT_DLL nmod_zip_find_coeffs_ret_t nmod_mpolyu_zip_find_coeffs(
                           nmod_zip_mpolyu_t Z, const nmod_mpoly_ctx_t ctx_sp);

FLINT_DLL int fmpz_mpolyu_addinterp_zip(fmpz_mpolyu_t H, const fmpz_t Hmodulus,
                          const nmod_zip_mpolyu_t Z, const nmodf_ctx_t ffinfo);

FLINT_DLL int fmpz_mpoly_repack_bits_inplace(fmpz_mpoly_t A, flint_bitcnt_t Abits,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpolyu_repack_bits(fmpz_mpolyu_t A, flint_bitcnt_t Abits,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpoly_set_bits(fmpz_mpoly_t A, flint_bitcnt_t Abits,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void fmpz_mpolyu_set_bits(fmpz_mpolyu_t A, flint_bitcnt_t Abits,
                                                   const fmpz_mpoly_ctx_t ctx);

FLINT_DLL int fmpz_mpolyuu_eval_all_but_one_nmod(slong * out_deg,
                        nmod_poly_t out, const fmpz_mpolyu_t A, slong var,
                               mp_limb_t * values, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL slong fmpz_mpolyuu_gcd_degree_bound_minor(slong * Adeg, slong * Bdeg,
                      const fmpz_mpolyu_t A, const fmpz_mpolyu_t B, slong var,
                               const fmpz_mpoly_ctx_t ctx, flint_rand_t state);

FLINT_DLL void fmpz_mpoly_ksub_content(fmpz_t content, const fmpz_mpoly_t A,
                            const ulong * subdegs, const fmpz_mpoly_ctx_t ctx);

/* Helpers for array methods *************************************************/

FLINT_DLL void _fmpz_mpoly_mul_array_chunked_DEG(fmpz_mpoly_t P,
                             const fmpz_mpoly_t A, const fmpz_mpoly_t B, 
                                       ulong degb, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_mul_array_chunked_LEX(fmpz_mpoly_t P,
                             const fmpz_mpoly_t A, const fmpz_mpoly_t B, 
                              const ulong * mults, const fmpz_mpoly_ctx_t ctx);

FLINT_DLL void _fmpz_mpoly_addmul_array1_slong1(ulong * poly1, 
                 const slong * poly2, const ulong * exp2, slong len2,
                          const slong * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_addmul_array1_slong(ulong * poly1, 
                 const slong * poly2, const ulong * exp2, slong len2,
                          const slong * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_addmul_array1_slong2(ulong * poly1, 
                 const slong * poly2, const ulong * exp2, slong len2,
                          const slong * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_addmul_array1_fmpz(fmpz * poly1, 
                 const fmpz * poly2, const ulong * exp2, slong len2,
                           const fmpz * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_submul_array1_slong(ulong * poly1, 
                  const slong * poly2, const ulong * exp2, slong len2,
                          const slong * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_submul_array1_slong2(ulong * poly1, 
                  const slong * poly2, const ulong * exp2, slong len2,
                          const slong * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_submul_array1_slong1(ulong * poly1, 
                 const slong * poly2, const ulong * exp2, slong len2,
                          const slong * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_submul_array1_fmpz(fmpz * poly1, 
                 const fmpz * poly2, const ulong * exp2, slong len2,
                           const fmpz * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_submul_array1_slong_1(ulong * poly1, 
                          slong d, const ulong exp2,
                          const slong * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_submul_array1_slong2_1(ulong * poly1, 
                           slong d, const ulong exp2,
                          const slong * poly3, const ulong * exp3, slong len3);

FLINT_DLL void _fmpz_mpoly_submul_array1_fmpz_1(fmpz * poly1, 
                          const fmpz_t d, ulong exp2,
                           const fmpz * poly3, const ulong * exp3, slong len3);

FLINT_DLL void mpoly_main_variable_split_LEX(slong * ind, ulong * pexp,
             const ulong * Aexp,
             slong l1, slong Alen, const ulong * mults, slong num, slong Abits);

FLINT_DLL void mpoly_main_variable_split_DEG(slong * ind, ulong * pexp,
             const ulong * Aexp,
             slong l1, slong Alen, ulong deg, slong num, slong Abits);

FLINT_DLL slong fmpz_mpoly_append_array_sm1_LEX(fmpz_mpoly_t P,
                        slong Plen, ulong * coeff_array,
                  const ulong * mults, slong num, slong array_size, slong top);
FLINT_DLL slong fmpz_mpoly_append_array_sm2_LEX(fmpz_mpoly_t P,
                        slong Plen, ulong * coeff_array,
                  const ulong * mults, slong num, slong array_size, slong top);
FLINT_DLL slong fmpz_mpoly_append_array_sm3_LEX(fmpz_mpoly_t P,
                         slong Plen, ulong * coeff_array,
                  const ulong * mults, slong num, slong array_size, slong top);
FLINT_DLL slong fmpz_mpoly_append_array_fmpz_LEX(fmpz_mpoly_t P,
                        slong Plen, fmpz * coeff_array,
                  const ulong * mults, slong num, slong array_size, slong top);

FLINT_DLL slong fmpz_mpoly_append_array_sm1_DEGLEX(fmpz_mpoly_t P,
          slong Plen, ulong * coeff_array, slong top, slong nvars, slong degb);
FLINT_DLL slong fmpz_mpoly_append_array_sm2_DEGLEX(fmpz_mpoly_t P,
          slong Plen, ulong * coeff_array, slong top, slong nvars, slong degb);
FLINT_DLL slong fmpz_mpoly_append_array_sm3_DEGLEX(fmpz_mpoly_t P,
          slong Plen, ulong * coeff_array, slong top, slong nvars, slong degb);
FLINT_DLL slong fmpz_mpoly_append_array_fmpz_DEGLEX(fmpz_mpoly_t P,
           slong Plen, fmpz * coeff_array, slong top, slong nvars, slong degb);

FLINT_DLL slong fmpz_mpoly_append_array_sm1_DEGREVLEX(fmpz_mpoly_t P,
          slong Plen, ulong * coeff_array, slong top, slong nvars, slong degb);
FLINT_DLL slong fmpz_mpoly_append_array_sm2_DEGREVLEX(fmpz_mpoly_t P,
          slong Plen, ulong * coeff_array, slong top, slong nvars, slong degb);
FLINT_DLL slong fmpz_mpoly_append_array_sm3_DEGREVLEX(fmpz_mpoly_t P,
          slong Plen, ulong * coeff_array, slong top, slong nvars, slong degb);
FLINT_DLL slong fmpz_mpoly_append_array_fmpz_DEGREVLEX(fmpz_mpoly_t P,
           slong Plen, fmpz * coeff_array, slong top, slong nvars, slong degb);

FLINT_DLL slong _fmpz_mpoly_from_ulong_array(fmpz ** poly1,
                         ulong ** exp1, slong * alloc, ulong * poly2,
                          const slong * mults, slong num, slong bits, slong k);

FLINT_DLL slong _fmpz_mpoly_from_ulong_array2(fmpz ** poly1,
                         ulong ** exp1, slong * alloc, ulong * poly2, 
                          const slong * mults, slong num, slong bits, slong k);

FLINT_DLL slong _fmpz_mpoly_from_ulong_array1(fmpz ** poly1,
                         ulong ** exp1, slong * alloc, ulong * poly2,
                          const slong * mults, slong num, slong bits, slong k);

FLINT_DLL slong _fmpz_mpoly_from_fmpz_array(fmpz ** poly1,
                         ulong ** exp1, slong * alloc, fmpz * poly2,
                          const slong * mults, slong num, slong bits, slong k);

FLINT_DLL void _fmpz_mpoly_to_ulong_array2(ulong * p, const fmpz * coeffs,
                                                const ulong * exps, slong len);

FLINT_DLL void _fmpz_mpoly_to_ulong_array1(ulong * p, const fmpz * coeffs,
                                                const ulong * exps, slong len);

FLINT_DLL void _fmpz_mpoly_to_ulong_array(ulong * p, const fmpz * coeffs,
                                                const ulong * exps, slong len);

FLINT_DLL void _fmpz_mpoly_to_fmpz_array(fmpz * p, const fmpz * coeffs,
                                                const ulong * exps, slong len);


/* Misc arithmetic - has nothing to do with mpoly, should be moved out *******/

FMPZ_MPOLY_INLINE
void _fmpz_mpoly_sub_uiuiui_fmpz(ulong * c, const fmpz_t d)
{
   fmpz fc = *d;

   if (!COEFF_IS_MPZ(fc))
   {
        ulong f0, f1, f2;
        f0 = fc;
        f1 = f2 = FLINT_SIGN_EXT(f0);
        sub_dddmmmsss(c[2], c[1], c[0], c[2], c[1], c[0], f2, f1, f0);
   } else
   {
      slong size = fmpz_size(d);
      __mpz_struct * m = COEFF_TO_PTR(fc);
      if (fmpz_sgn(d) < 0)
         mpn_add(c, c, 3, m->_mp_d, size);
      else
         mpn_sub(c, c, 3, m->_mp_d, size);
   }
}

FMPZ_MPOLY_INLINE
void _fmpz_mpoly_add_uiuiui_fmpz(ulong * c, const fmpz_t d)
{
    fmpz fc = *d;

    if (!COEFF_IS_MPZ(fc))
    {
        ulong f0, f1, f2;
        f0 = fc;
        f1 = f2 = FLINT_SIGN_EXT(f0);
        add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], f2, f1, f0);
   } else
   {
      slong size = fmpz_size(d);
      __mpz_struct * m = COEFF_TO_PTR(fc);
      if (fmpz_sgn(d) < 0)
         mpn_sub(c, c, 3, m->_mp_d, size);
      else
         mpn_add(c, c, 3, m->_mp_d, size);
   }
}

FMPZ_MPOLY_INLINE
void _fmpz_mpoly_submul_uiuiui_fmpz(ulong * c, slong d1, slong d2)
{
    ulong p[2], p2;
    smul_ppmm(p[1], p[0], d1, d2);
    p2 = FLINT_SIGN_EXT(p[1]);
    sub_dddmmmsss(c[2], c[1], c[0], c[2], c[1], c[0], p2, p[1], p[0]);
}

FMPZ_MPOLY_INLINE
void _fmpz_mpoly_addmul_uiuiui_fmpz(ulong * c, slong d1, slong d2)
{
    ulong p[2], p2;
    smul_ppmm(p[1], p[0], d1, d2);
    p2 = FLINT_SIGN_EXT(p[1]);
    add_sssaaaaaa(c[2], c[1], c[0], c[2], c[1], c[0], p2, p[1], p[0]);
}



/******************************************************************************

   Internal consistency checks

******************************************************************************/

/*
   test that r is a valid remainder upon division by g
   this means that if c*x^a is a term of r and x^a is divisible by the leading
   monomial of g, then |c| < |leading coefficient of g|
*/
FMPZ_MPOLY_INLINE
void fmpz_mpoly_remainder_test(const fmpz_mpoly_t r, const fmpz_mpoly_t g,
                                                    const fmpz_mpoly_ctx_t ctx)
{
   slong i, N, bits;
   ulong mask = 0;
   ulong * rexp, * gexp;

   bits = FLINT_MAX(r->bits, g->bits);
   N = mpoly_words_per_exp(bits, ctx->minfo);

   if (g->length == 0 )
      flint_throw(FLINT_ERROR, "Zero denominator in remainder test");

   if (r->length == 0 )
      return;

   rexp = (ulong *) flint_malloc(N*r->length*sizeof(ulong));
   gexp = (ulong *) flint_malloc(N*1        *sizeof(ulong));
   mpoly_repack_monomials(rexp, bits, r->exps, r->bits, r->length, ctx->minfo);
   mpoly_repack_monomials(gexp, bits, g->exps, g->bits, 1,         ctx->minfo);

   /* mask with high bit set in each field of exponent vector */
   for (i = 0; i < FLINT_BITS/bits; i++)
      mask = (mask << bits) + (UWORD(1) << (bits - 1));

    for (i = 0; i < r->length; i++)
    {
        int divides;

        if (bits <= FLINT_BITS)
            divides = mpoly_monomial_divides_test(rexp + i*N, gexp + 0*N, N, mask);
        else
            divides = mpoly_monomial_divides_mp_test(rexp + i*N, gexp + 0*N, N, bits);

        if (divides && fmpz_cmpabs(g->coeffs + 0, r->coeffs + i) <= 0)
        {
            flint_printf("fmpz_mpoly_remainder_test FAILED i = %wd\n", i);
            flint_printf("rem ");fmpz_mpoly_print_pretty(r, NULL, ctx); printf("\n\n");
            flint_printf("den ");fmpz_mpoly_print_pretty(g, NULL, ctx); printf("\n\n");
            flint_abort();
        }
    }

   flint_free(rexp);
   flint_free(gexp);
}


/*
   test that r is a valid remainder upon division by g over Q
   this means that no term of r is divisible by lt(g)
*/
FMPZ_MPOLY_INLINE
void fmpz_mpoly_remainder_strongtest(const fmpz_mpoly_t r, const fmpz_mpoly_t g,
                                                    const fmpz_mpoly_ctx_t ctx)
{
    slong i, N, bits;
    ulong mask = 0;
    ulong * rexp, * gexp;

    bits = FLINT_MAX(r->bits, g->bits);
    N = mpoly_words_per_exp(bits, ctx->minfo);

    if (g->length == 0 )
        flint_throw(FLINT_ERROR, "Zero denominator in remainder test");

    if (r->length == 0 )
        return;

    rexp = (ulong *) flint_malloc(N*r->length*sizeof(ulong));
    gexp = (ulong *) flint_malloc(N*1        *sizeof(ulong));
    mpoly_repack_monomials(rexp, bits, r->exps, r->bits, r->length, ctx->minfo);
    mpoly_repack_monomials(gexp, bits, g->exps, g->bits, 1,         ctx->minfo);

    /* mask with high bit set in each field of exponent vector */
    for (i = 0; i < FLINT_BITS/bits; i++)
        mask = (mask << bits) + (UWORD(1) << (bits - 1));

    for (i = 0; i < r->length; i++)
    {
        int divides;

        if (bits <= FLINT_BITS)
            divides = mpoly_monomial_divides_test(rexp + i*N, gexp + 0*N, N, mask);
        else
            divides = mpoly_monomial_divides_mp_test(rexp + i*N, gexp + 0*N, N, bits);

        if (divides)
        {
            flint_printf("fmpz_mpoly_remainder_strongtest FAILED i = %wd\n", i);
            flint_printf("rem ");fmpz_mpoly_print_pretty(r, NULL, ctx); printf("\n\n");
            flint_printf("den ");fmpz_mpoly_print_pretty(g, NULL, ctx); printf("\n\n");
            flint_abort();
        }
    }

    flint_free(rexp);
    flint_free(gexp);
}

#ifdef __cplusplus
}
#endif

#endif
