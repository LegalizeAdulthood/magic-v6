/* grCmap.c -
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1985, 1990 Regents of the University of California. * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  The University of California        * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
 *
 * This file provides routines that manipulate the color map on behalf
 * of the magic design system.
 */

#ifndef lint
static char rcsid[]="$Header: grCMap.c,v 6.0 90/08/28 18:40:37 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "windows.h"
#include "graphics.h"
#include "graphicsInt.h"
#include "utils.h"
#include "textio.h"

static char colorMap[256*3];		/* Storage for the color map. */


/*-----------------------------------------------------------------------------
 * GrReadCmap:
 *
 *	This routine initializes the color map values from the data
 *	given in a file.
 *
 * Results:
 *	The return value is TRUE if the color map was successfully
 *	loaded, and FALSE otherwise.
 *
 * Side Effects:
 *	The color map is read from the file and loaded into the graphics
 *	display.  The name of the color map file is x.y.z.cmap1, where
 *	x is techStyle, y is dispType, and z is monType.
 *
 * Design:
 *	The format of the file is one or more lines of the form
 *	<red> <green> <blue> <max location>.  When the first line
 *	is read in, the given red, green, and blue values are used
 *	to fill locations 0 - <max location> in the color map.  Then
 *	the next line is used to fill from there to the next max location,
 *	which must be larger than the first, and so on.  The last
 *	<max location> is expected to be 255.
 *-----------------------------------------------------------------------------
 */

bool
GrReadCMap(techStyle, dispType, monType, path, libPath)
char *techStyle;		/* The type of dstyle file requested by
				 * the current technology.
				 */
char *dispType;			/* A class of color map files for one or
				 * more display types.  Usually this
				 * is defaulted to NULL, in which case the
				 * type required by the current driver is
				 * used.  If the current driver lists a
				 * NULL type, it means no color map is
				 * needed at all (it's a black-and-white
				 * display), so nothing is loaded.
				 */
char *monType;			/* The class of monitors being used.  Usually
				 * given as "std".
				 */
char *path;			/* a search path */
char *libPath;			/* a library search path */

{
    FILE *f;
    int max, red, green, blue, newmax;
    char *ptr;
    char fullName[256];

    if (dispType == NULL)
    {
	if (grCMapType == NULL) return TRUE;
	dispType = grCMapType;
    }
    (void) sprintf(fullName, "%.80s.%.80s.%.80s", techStyle,
	    dispType, monType);
    f = PaOpen(fullName, "r", ".cmap1", path, libPath, (char **) NULL);
    if (f == NULL)
    {
	TxError("Couldn't open color map file \"%s.cmap1\"\n", fullName);
	return FALSE;
    }
    ptr = colorMap;
    max = 0;
    while (TRUE)
    {
	if (fscanf(f, "%d %d %d %d", &red, &green, &blue, &newmax) != 4)
	{
	    TxError("Syntax error in color map file \"%s.cmap1\"\n", fullName);
	    return FALSE;
	}
	if ((newmax < max) || (newmax > 255))
	{
	    TxError("Colors in map are out of order or too large.\n");
	    (void) fclose(f);
	    return FALSE;
	}
	for (; max <= newmax; max++)
	{
	    *ptr++ = red&0377;
	    *ptr++ = green&0377;
	    *ptr++ = blue&0377;
	}
	if (max >= 255)
	{
	    GrSetCMap(colorMap);
	    (void) fclose(f);
	    return TRUE;
	}
    }
}


/*-----------------------------------------------------------------------------
 *
 * GrResetCMap --
 *
 *	Write out the contents of colorMap to the graphics display.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Updates the color map on the graphics display.
 *
 *-----------------------------------------------------------------------------
 */

void
GrResetCMap()
{
    GrSetCMap(colorMap);
}


/*-----------------------------------------------------------------------------
 * GrSaveCMap
 *
 *	CMSave will save the current contents of the color map in a file
 *	so that it can be read back in later with GrLoadCMap.
 *
 * Results:
 *	TRUE is returned if the color map was successfully saved.  Otherwise
 *	FALSE is returned.  The file that's actually modified is x.y.z.cmap1,
 *	where x is techStyle, y is dispType, and z is monType.
 *
 * Side Effects:
 *	The file is overwritten with the color map values in the form
 *	described above under GrLoadCMap.
 *-----------------------------------------------------------------------------
 */

bool
GrSaveCMap(techStyle, dispType, monType, path, libPath)
char *techStyle;		/* The type of dstyle file requested by
				 * the current technology.
				 */
char *dispType;			/* A class of color map files for one or
				 * more display types.  Usually this
				 * is defaulted to NULL, in which case the
				 * type required by the current driver is
				 * used.
				 */
char *monType;			/* The class of monitors being used.  Usually
				 * given as "std".
				 */
char *path;			/* a search path */
char *libPath;			/* a library search path */

{
    FILE *f;
    char *ptr, *ptr2;
    int red, green, blue, i;
    char fullName[256];

    if (dispType == NULL) dispType = grCMapType;
    (void) sprintf(fullName, "%.80s.%.80s.%.80s", techStyle,
	    dispType, monType);
    f = PaOpen(fullName, "w", ".cmap1", path, libPath, (char **) NULL);
    if (f == NULL)
    {
	TxError("Couldn't write color map file \"%s.cmap1\"\n", fullName);
	return FALSE;
    }
    ptr = colorMap;
    red = colorMap[0];
    green = colorMap[1];
    blue = colorMap[2];
    for (i= 0;  i<256;  i++)
    {
	ptr2 = ptr;
	if (red == *ptr2++)
	    if (green == *ptr2++)
		if (blue == *ptr2++)
		{
		    ptr = ptr2;
		    continue;
		}
	(void) fprintf(f, "%d %d %d %d\n", red&0377, green&0377, 
		blue&0377, i-1);
	red = *ptr++;
	green = *ptr++;
	blue = *ptr++;
    }
    (void) fprintf(f, "%d %d %d 255\n", red, green, blue);
    (void) fclose(f);
    return TRUE;
}


/*-----------------------------------------------------------------------------
 *	GrGetColor reads a color value out of the map.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The values of red, green, and blue are overwritten with the
 *	red, green, and blue intensities for the color indicated by
 *	layer.
 *-----------------------------------------------------------------------------
 */

Void
GrGetColor(color, red, green, blue)
int color;			/* Color to be read. */
int *red, *green, *blue;	/* Pointers to values of color elements. */

{
    char *ptr;
    ptr = &(colorMap[3*color]);
    *red = *ptr++&0377;
    *green = *ptr++&0377;
    *blue = *ptr++&0377;
}



/*-----------------------------------------------------------------------------
 *	GrPutColor modifies the color map values for a single layer spec.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The indicated color is modified to have the given red, green, and
 *	blue intensities.
 *-----------------------------------------------------------------------------
 */

Void
GrPutColor(color, red, green, blue)
int color;			/* Color to be changed. */
int red, green, blue;		/* New intensities for color. */

{
    char *ptr;
    ptr = &(colorMap[3*color]);
    *ptr++ = red&0377;
    *ptr++ = green&0377;
    *ptr = blue&0377;
    GrSetCMap(colorMap);
}


/*-----------------------------------------------------------------------------
 *	GrPutManyColors --
 *
 *	Stores a new set of intensities in all portions of the map
 *	which contain a given set of color bits.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The color map entries for all layer combinations containing at
 *	least the bits in layers are reset to have the values given by
 *	red, green, and blue.  For example, if layers is 1 then all odd
 *	map entries are changed.  If layers is 3, every fourth map entry
 *	is changed, and if layers is 0 then all entries are changed.  This
 *	routine is a little tricky because if an opaque layer is present,
 *	then the six mask color bits must match exactly, since the opaque
 *	layer obscures ones beneath it.
 *-----------------------------------------------------------------------------
 */

Void
GrPutManyColors(color, red, green, blue, opaqueBit)
int color;			/* A specification of colors to be modified. */
int red, green, blue;		/* New intensity values. */
int opaqueBit;			/* The opaque/transparent bit.  It is assumed
				 * that the opaque layer colors or 
				 * transparent layer bits lie to the right
				 * of the opaque/transparent bit.
				 */

{
    char *ptr;
    int i, mask;
    mask = color;
    /* if a transparent layer */
    if (color&(opaqueBit+opaqueBit-1)) mask |= opaqueBit;
    /* if a opaque layer */
    if (color&opaqueBit) mask |= (opaqueBit-1);
    ptr = colorMap;
    for (i=0;  i<256; i++)
	if ((i&mask) == color)
	{
	    *ptr++ = red&0377;
	    *ptr++ = green&0377;
	    *ptr++ = blue&0377;
	}
	else
	{
	    ptr++;  ptr++; ptr++;
	}
    GrSetCMap(colorMap);
}
