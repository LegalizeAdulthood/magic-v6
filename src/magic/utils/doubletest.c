/* doubletest.c -
 *
 *	Test the double int package.
 */

#ifndef lint
static char rcsid[] = "$Header: doubletest.c,v 6.0 90/08/28 19:00:31 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "doubleint.h"

/* ----------------------------------------------------------------------------
 *
 * main --
 *
 *	Test the double int package.
 *
 * Results:
 *	Just prints things.
 *
 * Side Effects:
 *	printing.
 *
 * ----------------------------------------------------------------------------
 */

main()
{
    DoubleInt d1, d2, d3, d4, d5, d6, d7;
    char str[1000];

    DOUBLE_CREATE(d1, 1234567890);
    DoubleString(d1, str);
    printf("Test 1:  1234567890 == %s  (DOUBLE_CREATE, DoubleString)\n", str);

    DOUBLE_CREATE(d1, 1234567890);
    d2 = DoubleMultI(d1, 100);
    DoubleString(d2, str);
    printf("Test 2:  123456789000 == %s  (DoubleMultI)\n", str);

    DOUBLE_CREATE(d1, 1000111000);
    DOUBLE_CREATE(d2, 2000111000);
    DOUBLE_ADD(d3, d1, d2);
    DoubleString(d3, str);
    printf("Test 3:  3000222000 == %s  (DOUBLE_ADD)\n", str);

    DOUBLE_CREATE(d1, 0xFFFFFFFF);
    DOUBLE_ADDI(d3, d1, 23);
    DoubleString(d3, str);
    printf("Test 4:  4294967318 == %s  (DOUBLE_ADDI)\n", str);

    DOUBLE_CREATE(d1, 2000333000);
    DOUBLE_CREATE(d2, 1000111000);
    DOUBLE_SUB(d3, d1, d2);
    DoubleString(d3, str);
    printf("Test 5:  1000222000 == %s  (DOUBLE_SUB)\n", str);

    DOUBLE_CREATE(d1, 0xF0002222);
    DOUBLE_ADDI(d2, d1, 0x20001111);
    DOUBLE_SUBI(d3, d2, 415748);
    DoubleString(d3, str);
    printf("Test 6:  4563000111 == %s  (DOUBLE_SUBI)\n", str);

    exit(0);
}
