/*============================================================================

    This file is part of FLINT.

    FLINT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLINT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLINT; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

===============================================================================*/
/****************************************************************************

   Copyright (C) 2009 William Hart, Andy Novocin

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpir.h>
#include "flint.h"
#include "ulong_extras.h"
#include "fmpz.h"

int main(void)
{
   int result;
   printf("get_d_2exp....");
   fflush(stdout);

   fmpz_randinit();

   double output;
   long exp;

   for (ulong i = 0; i < 10000UL; i++) 
   {
      fmpz_t a;
      
      fmpz_init(a);

      fmpz_randtest(a, 200);

      output = fmpz_get_d_2exp(&exp, a);

      result = (fmpz_bits(a) == exp);
      
      if (!result)
      {
         printf("FAIL\n");
         printf("a = "); fmpz_print(a); printf("\n");
         printf("output = %f\n", output);
         printf("exp = %ld, bits = %lu\n", exp, fmpz_bits(a));
         abort();
      }

      fmpz_clear(a);
   }

   fmpz_randclear();

   _fmpz_cleanup();
   printf("PASS\n");
   return 0;
}
