/*
    Copyright (C) 2018 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "nmod_mpoly.h"

/*
    Try to set G to the gcd of A and B using Brown's alogrithm M.
    This function switches to a big primes version if needed.
*/
int nmod_mpoly_gcd_brown(
    nmod_mpoly_t G,
    const nmod_mpoly_t A,
    const nmod_mpoly_t B,
    const nmod_mpoly_ctx_t ctx)
{
    int success;
    slong * perm;
    ulong * shift, * stride;
    slong i;
    flint_bitcnt_t new_bits;
    nmod_mpoly_ctx_t uctx;
    nmod_mpolyun_t An, Bn, Gn, Abarn, Bbarn;
    nmod_poly_stack_t Sp;

    if (nmod_mpoly_is_zero(A, ctx))
    {
        if (nmod_mpoly_is_zero(B, ctx))
        {
            nmod_mpoly_zero(G, ctx);
        }
        else
        {
            nmod_mpoly_make_monic(G, B, ctx);
        }
        return 1;
    }

    if (nmod_mpoly_is_zero(B, ctx))
    {
        nmod_mpoly_make_monic(G, A, ctx);
        return 1;
    }

    if (A->bits > FLINT_BITS || B->bits > FLINT_BITS)
    {
        return 0;
    }

    perm = (slong *) flint_malloc(ctx->minfo->nvars*sizeof(slong));
    shift = (ulong *) flint_malloc(ctx->minfo->nvars*sizeof(ulong));
    stride = (ulong *) flint_malloc(ctx->minfo->nvars*sizeof(ulong));
    for (i = 0; i < ctx->minfo->nvars; i++)
    {
        perm[i] = i;
        shift[i] = 0;
        stride[i] = 1;
    }

    if (ctx->minfo->nvars == 1)
    {
        nmod_poly_t a, b, g;
        nmod_poly_init(a, ctx->ffinfo->mod.n);
        nmod_poly_init(b, ctx->ffinfo->mod.n);
        nmod_poly_init(g, ctx->ffinfo->mod.n);
        _nmod_mpoly_to_nmod_poly_deflate(a, A, 0, shift, stride, ctx);
        _nmod_mpoly_to_nmod_poly_deflate(b, B, 0, shift, stride, ctx);
        nmod_poly_gcd(g, a, b);
        _nmod_mpoly_from_nmod_poly_inflate(G, A->bits, g, 0, shift, stride, ctx);
        nmod_poly_clear(a);
        nmod_poly_clear(b);
        nmod_poly_clear(g);
        success = 1;
        goto cleanup1;
    }

    new_bits = FLINT_MAX(A->bits, B->bits);

    nmod_mpoly_ctx_init(uctx, ctx->minfo->nvars - 1, ORD_LEX, ctx->ffinfo->mod.n);
    nmod_poly_stack_init(Sp, new_bits, uctx);
    nmod_mpolyun_init(An, new_bits, uctx);
    nmod_mpolyun_init(Bn, new_bits, uctx);
    nmod_mpolyun_init(Gn, new_bits, uctx);
    nmod_mpolyun_init(Abarn, new_bits, uctx);
    nmod_mpolyun_init(Bbarn, new_bits, uctx);

    nmod_mpoly_to_mpolyun_perm_deflate(An, uctx, A, ctx, perm, shift, stride, NULL, 0);
    nmod_mpoly_to_mpolyun_perm_deflate(Bn, uctx, B, ctx, perm, shift, stride, NULL, 0);
    success = nmod_mpolyun_gcd_brown_smprime(Gn, Abarn, Bbarn, An, Bn,
                                             uctx->minfo->nvars - 1, uctx, Sp);
    if (!success)
    {
        nmod_mpoly_to_mpolyun_perm_deflate(An, uctx, A, ctx, perm, shift, stride, NULL, 0);
        nmod_mpoly_to_mpolyun_perm_deflate(Bn, uctx, B, ctx, perm, shift, stride, NULL, 0);

        success = nmod_mpolyun_gcd_brown_lgprime(Gn, Abarn, Bbarn, An, Bn,
                                                 uctx->minfo->nvars - 1, uctx);
    }

    if (success)
    {
        nmod_mpoly_from_mpolyun_perm_inflate(G, new_bits, ctx,
                                                Gn, uctx, perm, shift, stride);
        nmod_mpoly_make_monic(G, G, ctx);
    }

    nmod_mpolyun_clear(An, uctx);
    nmod_mpolyun_clear(Bn, uctx);
    nmod_mpolyun_clear(Gn, uctx);
    nmod_mpolyun_clear(Abarn, uctx);
    nmod_mpolyun_clear(Bbarn, uctx);
    nmod_poly_stack_clear(Sp);
    nmod_mpoly_ctx_clear(uctx);

cleanup1:

    flint_free(perm);
    flint_free(shift);
    flint_free(stride);

    return success;
}
