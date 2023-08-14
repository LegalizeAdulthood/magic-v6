#include <stdio.h>
#include <X/Xlib.h>


#ifndef lint
static char rcsid[]="$Header: prcolors.c,v 6.0 90/08/28 18:41:51 mayo Exp $";
#endif  not lint

main()
{
    Color color;
    int i;

    XOpenDisplay(NULL);
    for (i = 0; i < 256; i++)
    {
	color.pixel = i;
	if (XQueryColor(&color))
	    printf("#%x\t%5d%5d%5d\n", i,
		color.red / 256, color.green / 256, color.blue / 256);
    }
}
