/*
    Copyright (C) 2019 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "fmpz_mpoly.h"

void gcd_check(
    fmpz_mpoly_t g,
    fmpz_mpoly_t a,
    fmpz_mpoly_t b,
    fmpz_mpoly_ctx_t ctx,
    slong thread_limit,
    slong i,
    slong j,
    const char * name)
{
    int res;
    fmpz_mpoly_t ca, cb, cg;

    fmpz_mpoly_init(ca, ctx);
    fmpz_mpoly_init(cb, ctx);
    fmpz_mpoly_init(cg, ctx);

    res = fmpz_mpoly_gcd_berlekamp_massey_threaded(g, a, b, ctx, thread_limit);
    fmpz_mpoly_assert_canonical(g, ctx);

    if (!res)
    {
        flint_printf("Check gcd can be computed\n"
                                         "i = %wd, j = %wd, %s\n", i, j, name);
        flint_abort();
    }

    if (fmpz_mpoly_is_zero(g, ctx))
    {
        if (!fmpz_mpoly_is_zero(a, ctx) || !fmpz_mpoly_is_zero(b, ctx))
        {
            printf("FAIL\n");
            flint_printf("Check zero gcd only results from zero inputs\n"
                                         "i = %wd, j = %wd, %s\n", i, j, name);
            flint_abort();
        }
        goto cleanup;
    }

    if (fmpz_sgn(g->coeffs + 0) <= 0)
    {
        printf("FAIL\n");
        flint_printf("Check gcd has positive lc\n"
                                         "i = %wd, j = %wd, %s\n", i, j, name);
        flint_abort();
    }

    res = 1;
    res = res && fmpz_mpoly_divides(ca, a, g, ctx);
    res = res && fmpz_mpoly_divides(cb, b, g, ctx);
    if (!res)
    {
        printf("FAIL\n");
        flint_printf("Check divisibility\n"
                                         "i = %wd, j = %wd, %s\n", i, j, name);
        flint_abort();
    }

    res = fmpz_mpoly_gcd_berlekamp_massey_threaded(cg, ca, cb, ctx, thread_limit);
    fmpz_mpoly_assert_canonical(cg, ctx);

    if (!res)
    {
        printf("FAIL\n");
        flint_printf("Check gcd of cofactors can be computed\n"
                                         "i = %wd, j = %wd, %s\n", i, j, name);
        flint_abort();
    }

    if (!fmpz_mpoly_is_one(cg, ctx))
    {
        printf("FAIL\n");
        flint_printf("Check gcd of cofactors is one\n"
                                         "i = %wd, j = %wd, %s\n", i, j, name);
        flint_abort();
    }

cleanup:

    fmpz_mpoly_clear(ca, ctx);
    fmpz_mpoly_clear(cb, ctx);
    fmpz_mpoly_clear(cg, ctx);
}


int
main(void)
{
    slong i, j, tmul = 10;
    slong max_threads = 5;
    FLINT_TEST_INIT(state);
#ifdef _WIN32
    tmul = 1;
#endif

    flint_printf("gcd_berlekamp_massey_threaded....");
    fflush(stdout);

    for (i = 0; i < 2 + 2*tmul; i++)
    {
        fmpz_mpoly_ctx_t ctx;
        fmpz_mpoly_t g, a, b;
        const char* vars[] = {"y", "t", "x", "z"};

        fmpz_mpoly_ctx_init(ctx, 4, ORD_DEGLEX);
        fmpz_mpoly_init(g, ctx);
        fmpz_mpoly_init(a, ctx);
        fmpz_mpoly_init(b, ctx);

        /*
            The main variables are y and t, which have reasonable degrees.
        */
        fmpz_mpoly_set_str_pretty(g, "39 - t*x + 39*x^100 - t*x^101 + 39*x^3*y - t*x^4*y - 7*x^2*y^3*z^11 - 7*x^102*y^3*z^11 - 7*x^5*y^4*z^11 + 78*t^15*x^78*y^3*z^13 - 2*t^16*x^79*y^3*z^13 + x^1000*y^3*z^20 + x^1100*y^3*z^20 + x^1003*y^4*z^20 - 14*t^15*x^80*y^6*z^24 + 2*t^15*x^1078*y^6*z^33", vars, ctx);
        fmpz_mpoly_set_str_pretty(a, "39 - t*x - 7*x^2*y^3*z^11 + x^1000*y^3*z^20", vars, ctx);
        fmpz_mpoly_set_str_pretty(b, "1 + x^100 + x^3*y + 2*t^15*x^78*y^3*z^13", vars, ctx);
        fmpz_mpoly_mul(a, a, g, ctx);
        fmpz_mpoly_mul(b, b, g, ctx);
        gcd_check(g, a, b, ctx, 100, -1, 0, "example");

        /*
            The lesser variables are x and z and the gcd degree bounds will be
                deg_x(gcd) < 4
                deg_z(gcd) < 5
            Therefore, the initial ksub will be
                x = v^5
                z = v
            and the cofactors are not r.p. under this ksub.
        */
        fmpz_mpoly_set_str_pretty(g, "y + t^2 + x^3 + z^4", vars, ctx);
        fmpz_mpoly_set_str_pretty(a, "y*t + 1 + (x - z^5)*(y + t)", vars, ctx);
        fmpz_mpoly_set_str_pretty(b, "y*t + 1 + (x - z^5)*(y - t + x)", vars, ctx);
        fmpz_mpoly_mul(a, a, g, ctx);
        fmpz_mpoly_mul(b, b, g, ctx);
        gcd_check(g, a, b, ctx, 100, -1, 1, "trigger unlucky ksub");

        /*
            The coefficients of g are the first three chosen smooth primes.
            The first one will trigger a restart when updating the zippler.
            The other two will trigger a restart when (unsuccessfully) solving
            for the coeffs. In total there should be three restarts.
        */
        fmpz_mpoly_set_str_pretty(g, "y + 33857*t^2 + 35153*x^3 + 40433*z^4", vars, ctx);
        fmpz_mpoly_set_str_pretty(a, "y^4 + t^3 + x^2 + 1", vars, ctx);
        fmpz_mpoly_set_str_pretty(b, "y^3 + t^4 + x^2 + z", vars, ctx);
        fmpz_mpoly_mul(a, a, g, ctx);
        fmpz_mpoly_mul(b, b, g, ctx);
        gcd_check(g, a, b, ctx, 100, -1, 2, "trigger zipple no match");

        /*
            The initial ksub will be
                x = v^4
                z = v
            This kills lc_yt(a) over Z[x,z] in first example, and over Z/pZ[x,z]
            for the first chosen smooth prime in the second example.
        */
        fmpz_mpoly_set_str_pretty(g, "y + t + x^3 + z^3", vars, ctx);
        fmpz_mpoly_set_str_pretty(a, "(x - z^4)*y + 1", vars, ctx);
        fmpz_mpoly_set_str_pretty(b, "(x + z)*y + t", vars, ctx);
        fmpz_mpoly_mul(a, a, g, ctx);
        fmpz_mpoly_mul(b, b, g, ctx);
        gcd_check(g, a, b, ctx, 100, -1, 3, "trigger ksub lc kill");

        fmpz_mpoly_set_str_pretty(g, "y + t + x^3 + z^3", vars, ctx);
        fmpz_mpoly_set_str_pretty(a, "(x - z^4 + 33857*(x*z))*y + 1", vars, ctx);
        fmpz_mpoly_set_str_pretty(b, "(x + z)*y + t", vars, ctx);
        fmpz_mpoly_mul(a, a, g, ctx);
        fmpz_mpoly_mul(b, b, g, ctx);
        gcd_check(g, a, b, ctx, 100, -1, 4, "trigger ksub lc kill mod p");

        /*
            gcd(lc_yt(a), lc_yt(b)) will have terms vanishing modulo the first
            two chosen smooth primes. There will be two restarts.
        */
        fmpz_mpoly_set_str_pretty(g, "(1 + x^10)*t*y + t + x + z", vars, ctx);
        fmpz_mpoly_set_str_pretty(a, "(1 + x + t)*(x - 33857*x^2 + 35153*z^4)*y*t + z + x*y", vars, ctx);
        fmpz_mpoly_set_str_pretty(b, "(2*x - t^2)*(x - 33857*x^2 + 35153*z^4)*y^2*t + t*z + y", vars, ctx);
        fmpz_mpoly_mul(a, a, g, ctx);
        fmpz_mpoly_mul(b, b, g, ctx);
        gcd_check(g, a, b, ctx, 100, -1, 5, "trigger gcd lc terms vanish mod p");

        /*
            The ksub will elevate the degrees past FLINT_BITS. Content removal
            can be done because the leading coeffs wrt yt are monomial.
        */
        if (FLINT_BITS == 64)
        {
            fmpz_mpoly_set_str_pretty(g, "t*y + t + x^9999999999 + z^9999999999", vars, ctx);
            fmpz_mpoly_set_str_pretty(a, "t + y + x^9999999999", vars, ctx);
            fmpz_mpoly_set_str_pretty(b, "t^2 + t*z^9999999999 + y + 1", vars, ctx);
        }
        else
        {
            fmpz_mpoly_set_str_pretty(g, "t*y + t + x^999999 + z^9999999", vars, ctx);
            fmpz_mpoly_set_str_pretty(a, "t + y + x^999999", vars, ctx);
            fmpz_mpoly_set_str_pretty(b, "t^2 + t*z^999999 + y + 1", vars, ctx);
        }
        fmpz_mpoly_mul(a, a, g, ctx);
        fmpz_mpoly_mul(b, b, g, ctx);
        gcd_check(g, a, b, ctx, 100, -1, 6, "trigger big p");

        flint_set_num_threads(n_randint(state, max_threads) + 1);

        fmpz_mpoly_clear(a, ctx);
        fmpz_mpoly_clear(b, ctx);
        fmpz_mpoly_clear(g, ctx);
        fmpz_mpoly_ctx_clear(ctx);
    }

    for (i = 0; i < tmul * flint_test_multiplier(); i++)
    {
        fmpz_mpoly_ctx_t ctx;
        fmpz_mpoly_t a, b, g, t;
        flint_bitcnt_t coeff_bits;
        slong len, len1, len2;
        slong degbound;

        fmpz_mpoly_ctx_init_rand(ctx, state, 20);

        fmpz_mpoly_init(g, ctx);
        fmpz_mpoly_init(a, ctx);
        fmpz_mpoly_init(b, ctx);
        fmpz_mpoly_init(t, ctx);

        len = n_randint(state, 40) + 1;
        len1 = n_randint(state, 50);
        len2 = n_randint(state, 50);

        degbound = 120/(2*ctx->minfo->nvars - 1);

        coeff_bits = n_randint(state, 100);

        for (j = 0; j < 4; j++)
        {
            do {
                fmpz_mpoly_randtest_bound(t, state, len, coeff_bits + 1, degbound, ctx);
            } while (t->length == 0);
            fmpz_mpoly_randtest_bound(a, state, len1, coeff_bits, degbound, ctx);
            fmpz_mpoly_randtest_bound(b, state, len2, coeff_bits, degbound, ctx);
            fmpz_mpoly_mul(a, a, t, ctx);
            fmpz_mpoly_mul(b, b, t, ctx);
            fmpz_mpoly_randtest_bits(g, state, len, coeff_bits, FLINT_BITS, ctx);
            flint_set_num_threads(n_randint(state, max_threads) + 1);
            gcd_check(g, a, b, ctx, n_randint(state, max_threads + 3),
                                                        i, j, "random sparse");
        }

        fmpz_mpoly_clear(g, ctx);
        fmpz_mpoly_clear(a, ctx);
        fmpz_mpoly_clear(b, ctx);
        fmpz_mpoly_clear(t, ctx);
        fmpz_mpoly_ctx_clear(ctx);
    }

    printf("PASS\n");
    FLINT_TEST_CLEANUP(state);

    return 0;
}
