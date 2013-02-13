/*=============================================================================

    This file is part of ARB.

    ARB is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ARB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ARB; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

=============================================================================*/
/******************************************************************************

    Copyright (C) 2012 Fredrik Johansson

******************************************************************************/

#include "fmprb.h"

/* With parameter n, the error is bounded by 3/(3+sqrt(8))^n */
#define ERROR_A 1.5849625007211561815 /* log2(3) */
#define ERROR_B 2.5431066063272239453 /* log2(3+sqrt(8)) */

void borwein_error(fmpr_t err, long n);

void
fmprb_zeta_ui_vec_borwein(fmprb_struct * z, ulong start, long num,
    ulong step, long prec)
{
    long j, k, s, n, wp;
    fmpz_t c, d, t, u;
    fmpz * zeta;
    fmpr_t err;

    if (num < 1)
        return;

    wp = prec + FLINT_BIT_COUNT(prec);
    n = wp / 2.5431066063272239453 + 1;

    fmpz_init(c);
    fmpz_init(d);
    fmpz_init(t);
    fmpz_init(u);
    zeta = _fmpz_vec_init(num);

    fmpz_set_ui(c, 1);
    fmpz_mul_2exp(c, c, 2 * n - 1);
    fmpz_set(d, c);

    for (k = n; k > 0; k--)
    {
        /* divide by first k^s */
        fmpz_ui_pow_ui(u, k, start);
        fmpz_tdiv_q(t, d, u);
        if (k % 2 == 0)
            fmpz_neg(t, t);
        fmpz_add(zeta, zeta, t);

        /* remaining k^s */
        fmpz_ui_pow_ui(u, k, step);
        for (j = 1; j < num; j++)
        {
            fmpz_tdiv_q(t, t, u);
            fmpz_add(zeta + j, zeta + j, t);
        }

        /* hypergeometric recurrence */
        fmpz_mul2_uiui(c, c, k, 2 * k - 1);
        fmpz_divexact2_uiui(c, c, 2 * (n - k + 1), n + k - 1);
        fmpz_add(d, d, c);
    }

    fmpr_init(err);
    borwein_error(err, n);

    for (k = 0; k < num; k++)
    {
        fmprb_struct * x = z + k;
        s = start + step * k;

        fmprb_set_fmpz(x, zeta + k);
        /* the error in each term in the main loop is < 2 */
        fmpr_set_ui(fmprb_radref(x), 2 * n);
        fmprb_div_fmpz(x, x, d, wp);

        /* mathematical error for eta(s), bounded by 3/(3+sqrt(8))^n */
        fmprb_add_error_fmpr(x, err);

        /* convert from eta(s) to zeta(s) */
        fmprb_div_2expm1_ui(x, x, s - 1, wp);
        fmprb_mul_2exp_si(x, x, s - 1);
    }

    fmpr_clear(err);

    fmpz_clear(c);
    fmpz_clear(d);
    fmpz_clear(t);
    fmpz_clear(u);
    _fmpz_vec_clear(zeta, num);
}
