
/*
 * grGlyphs.h --
 *
 * 	Data structures to hold glyphs.
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
 *
 * rcsid "$Header: glyphs.h,v 6.0 90/08/28 18:40:20 mayo Exp $"
 */

#define _GLYPHS

#ifndef _GEOMETRY
	err = Need_to_include_geometry_header
#endif

/* data structures */

typedef struct GR_GLY2 {
    Point gr_origin;	/* The location of the origin of the glyph. */
    int gr_xsize;	/* The width of the glyph. */
    int gr_ysize;	/* The height of the glyph. */
    ClientData gr_cache;/* The device driver may cache pixels and stuff here
			 * for more efficient display.  If another module
			 * changes the other fields of this record, they
			 * should call 'gr_free()' on this field (if it is
			 * non-null) and then set this field to NULL.
			 */
    Void (*gr_free)();	/* Proc to free above field, takes that field as its
			 * sole argument.
			 */
    int gr_pixels[1];	/* Will actually be as large an array as needed. */
} GrGlyph;

typedef struct GR_GLY3
{
    int gr_num;	   	   /* The number of glyphs in this record. */
    GrGlyph *gr_glyph[1];  /* Will be big enough to hold as many glyphs as 
			    * we have.
			    */
} GrGlyphs;

/* procedures */

extern void GrFreeGlyphs();

/* constants */
