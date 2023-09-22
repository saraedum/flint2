/*
    Copyright (C) 2023 Jean Kieffer

    This file is part of Arb.

    Arb is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "acb_theta.h"

int main(void)
{
    slong iter;
    flint_rand_t state;

    flint_printf("jet_all....");
    fflush(stdout);

    flint_randinit(state);

    /* Test: compute theta values at two points in a common ball */
    for (iter = 0; iter < 10 * flint_test_multiplier(); iter++)
    {
        slong g = 1 + n_randint(state, 3);
        slong n = 1 << (2 * g);
        slong ord = n_randint(state, 3);
        slong bits = 2;
        slong nb = acb_theta_jet_nb(ord, g + 1);
        acb_mat_t tau1, tau2, tau3;
        acb_ptr z1, z2, z3;
        arb_ptr err;
        acb_ptr d1, d2, test;
        acb_t x;
        slong lprec = ACB_THETA_LOW_PREC + n_randint(state, 50);
        slong hprec = lprec + n_randint(state, 50);
        slong j, k;

        acb_mat_init(tau1, g, g);
        acb_mat_init(tau2, g, g);
        acb_mat_init(tau3, g, g);
        z1 = _acb_vec_init(g);
        z2 = _acb_vec_init(g);
        z3 = _acb_vec_init(g);
        err = _arb_vec_init(n * nb);
        d1 = _acb_vec_init(n * nb);
        d2 = _acb_vec_init(n * nb);
        test = _acb_vec_init(n * nb);
        acb_init(x);

        acb_siegel_randtest_reduced(tau1, state, hprec, bits);
        for (j = 0; j < g; j++)
        {
            acb_urandom(&z1[j], state, hprec);
        }

        for (j = 0; j < g; j++)
        {
            for (k = j; k < g; k++)
            {
                acb_set(acb_mat_entry(tau1, k, j), acb_mat_entry(tau1, j, k));
                acb_urandom(x, state, hprec);
                acb_mul_2exp_si(x, x, -lprec);
                acb_add(acb_mat_entry(tau2, j, k), acb_mat_entry(tau1, j, k), x, hprec);
                acb_set(acb_mat_entry(tau2, k, j), acb_mat_entry(tau2, j, k));
                acb_union(acb_mat_entry(tau3, j, k), acb_mat_entry(tau1, j, k),
                    acb_mat_entry(tau2, j, k), hprec);
                acb_set(acb_mat_entry(tau3, k, j), acb_mat_entry(tau3, j, k));
            }
            acb_urandom(x, state, hprec);
            acb_mul_2exp_si(x, x, -lprec);
            acb_add(&z2[j], &z1[j], x, hprec);
            acb_union(&z3[j], &z1[j], &z2[j], hprec);
        }

        if (!acb_mat_contains(tau3, tau2) || !acb_mat_contains(tau3, tau1)
            || !_acb_vec_contains(z3, z1, g) || !_acb_vec_contains(z3, z1, g))
        {
            flint_printf("FAIL (input)\n");
            flint_printf("lprec = %wd, hprec = %wd\n", lprec, hprec);
            acb_mat_printd(tau1, 5);
            acb_mat_printd(tau2, 5);
            acb_mat_printd(tau3, 5);
            _acb_vec_printd(z1, g, 5);
            _acb_vec_printd(z2, g, 5);
            _acb_vec_printd(z3, g, 5);
            flint_abort();
        }

        acb_theta_jet_naive_all(d1, z1, tau1, ord, hprec);
        acb_theta_jet_naive_all(d2, z2, tau2, ord, hprec);
        acb_theta_jet_error_bounds(err, z3, tau3, ord, ACB_THETA_LOW_PREC/2);
         /* Errors are wrt midpoint, so multiply by 2 */
        _arb_vec_scalar_mul_2exp_si(err, err, n * nb, 1);

        _acb_vec_set(test, d1, n * nb);
        for (k = 0; k < n * nb; k++)
        {
            acb_add_error_arb(&test[k], &err[k]);
        }

        if (!_acb_vec_overlaps(test, d2, n * nb))
        {
            flint_printf("FAIL (bounds)\n");
            flint_printf("values:\n");
            _acb_vec_printd(d1, n * nb, 5);
            _acb_vec_printd(d2, n * nb, 5);
            flint_printf("bounds:\n");
            _arb_vec_printd(err, n * nb, 5);
            flint_printf("difference:\n");
            _acb_vec_sub(d1, d1, d2, n * nb, hprec);
            _acb_vec_printd(d1, n * nb, 5);
            flint_abort();
        }

        acb_mat_clear(tau1);
        acb_mat_clear(tau2);
        acb_mat_clear(tau3);
        _acb_vec_clear(z1, g);
        _acb_vec_clear(z2, g);
        _acb_vec_clear(z3, g);
        _arb_vec_clear(err, n * nb);
        _acb_vec_clear(d1, n * nb);
        _acb_vec_clear(d2, n * nb);
        _acb_vec_clear(test, n * nb);
        acb_clear(x);
    }

    flint_randclear(state);
    flint_cleanup();
    flint_printf("PASS\n");
    return 0;
}
