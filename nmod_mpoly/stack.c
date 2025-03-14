/*
    Copyright (C) 2019 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "nmod_poly.h"
#include "nmod_mpoly.h"

void nmod_poly_stack_init(nmod_poly_stack_t S, flint_bitcnt_t bits, const nmod_mpoly_ctx_t ctx)
{
    S->bits = bits;
    S->ctx = ctx;

    S->poly_alloc = 0;
    S->poly_array = NULL;
    S->poly_top = 0;

    S->mpolyun_alloc = 0;
    S->mpolyun_array = NULL;
    S->mpolyun_top = 0;
}

void nmod_poly_stack_clear(nmod_poly_stack_t S)
{
    slong i;

    FLINT_ASSERT(S->poly_top == 0);

    for (i = 0; i < S->poly_alloc; i++)
    {
        nmod_poly_clear(S->poly_array[i]);
        flint_free(S->poly_array[i]);
    }
    if (S->poly_array)
        flint_free(S->poly_array);
    S->poly_array = NULL;

    for (i = 0; i < S->mpolyun_alloc; i++)
    {
        nmod_mpolyun_clear(S->mpolyun_array[i], S->ctx);
        flint_free(S->mpolyun_array[i]);
    }
    if (S->mpolyun_array)
        flint_free(S->mpolyun_array);
    S->mpolyun_array = NULL;

    S->ctx = NULL;
}

void nmod_poly_stack_set_ctx(nmod_poly_stack_t S, const nmod_mpoly_ctx_t ctx)
{
    slong i;

    FLINT_ASSERT(S->ctx->minfo->nvars == ctx->minfo->nvars);

    S->ctx = ctx;

    for (i = 0; i < S->poly_alloc; i++)
    {
        nmod_poly_set_mod(S->poly_array[i], S->ctx->ffinfo->mod);
    }

    for (i = 0; i < S->mpolyun_alloc; i++)
    {
        nmod_mpolyun_set_mod(S->mpolyun_array[i], S->ctx->ffinfo->mod);
    }
}

/* insure that k slots are available after top and return pointer to top */
nmod_poly_struct ** nmod_poly_stack_fit_request_poly(nmod_poly_stack_t S, slong k)
{
    slong newalloc, i;

    FLINT_ASSERT(S->poly_alloc >= S->poly_top);

    if (S->poly_top + k > S->poly_alloc)
    {
        newalloc = FLINT_MAX(WORD(1), S->poly_top + k);

        if (S->poly_array)
        {
            S->poly_array = (nmod_poly_struct **) flint_realloc(S->poly_array,
                                           newalloc*sizeof(nmod_poly_struct*));
        }
        else
        {
            S->poly_array = (nmod_poly_struct **) flint_malloc(
                                           newalloc*sizeof(nmod_poly_struct*));
        }

        for (i = S->poly_alloc; i < newalloc; i++)
        {
            S->poly_array[i] = (nmod_poly_struct *) flint_malloc(
                                                     sizeof(nmod_poly_struct));
            nmod_poly_init_mod(S->poly_array[i], S->ctx->ffinfo->mod);
        }
        S->poly_alloc = newalloc;
    }

    return S->poly_array + S->poly_top;
}



nmod_mpolyun_struct ** nmod_poly_stack_fit_request_mpolyun(nmod_poly_stack_t S, slong k)
{
    slong newalloc, i;

    FLINT_ASSERT(S->mpolyun_alloc >= S->mpolyun_top);

    if (S->mpolyun_top + k > S->mpolyun_alloc)
    {
        newalloc = FLINT_MAX(WORD(1), S->mpolyun_top + k);

        if (S->mpolyun_array)
        {
            S->mpolyun_array = (nmod_mpolyun_struct **) flint_realloc(S->mpolyun_array,
                                           newalloc*sizeof(nmod_mpolyun_struct*));
        }
        else
        {
            S->mpolyun_array = (nmod_mpolyun_struct **) flint_malloc(
                                           newalloc*sizeof(nmod_mpolyun_struct*));
        }

        for (i = S->mpolyun_alloc; i < newalloc; i++)
        {
            S->mpolyun_array[i] = (nmod_mpolyun_struct *) flint_malloc(
                                                     sizeof(nmod_mpolyun_struct));
            nmod_mpolyun_init(S->mpolyun_array[i], S->bits, S->ctx);
        }
        S->mpolyun_alloc = newalloc;
    }

    return S->mpolyun_array + S->mpolyun_top;
}

