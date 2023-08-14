#ifndef lint
static char rcsid[] = "$Header: printstuff.c,v 6.0 90/08/28 19:01:16 mayo Exp $";
#endif not lint

#include "magic.h"
#include "geometry.h"

PrintTrans(t)
Transform *t;
{
    printf("Translate: (%d, %d)\n", t->t_c, t->t_f);
    printf("%d\t%d\n", t->t_a, t->t_d);
    printf("%d\t%d\n", t->t_b, t->t_e);
}

PrintRect(r)
Rect *r;
{
    printf("(%d,%d) :: (%d,%d)\n", r->r_xbot, r->r_ybot, r->r_xtop, r->r_ytop);
}
