/*
 * plotVers.c --
 *
 * This file contains the procedures that generate plots on
 * Versatec-style black-and-white printers.
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
 */

#ifndef lint
static char rcsid[]="$Header: plotVers.c,v 6.0 90/08/28 18:52:05 mayo Exp $";
#endif  not lint

#include <stdio.h>
#ifndef	SYSV
#include <vfont.h>
#endif	SYSV
#include "magic.h"
#include "geometry.h"
#include "geofast.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "malloc.h"
#include "plotInt.h"
#include "windows.h"
#include "commands.h"
#include "textio.h"
#include "utils.h"
#include "tech.h"
#include "signals.h"
#include "router.h"

/* Library imports: */

extern char *mktemp();
extern int rasFileByteCount; 
  
/* Records of the following type are used to describe how to generate
 * output for the mask layers.  Each style describes a particular
 * stipple pattern.
 */


typedef struct versatecstyle
{
    TileTypeBitMask vs_layers;		/* Layers to plot in this style. */
    Stipple vs_stipple;			/* Stipple pattern to use. */
    int vs_flags;			/* Flag bits, see below. */
    struct versatecstyle *vs_next;	/* Pointer to next style in list. */

    		/* If the color flag is false, all stipples will be black */
    VersatecColor vs_color;		/* Stipple color */
} VersatecStyle;

/* Flag values for VersatecStyles:
 *
 * VS_CROSS -	if this bit is set, then generate an outline with an
 *		X through the middle, like for contacts, instead of
 *		stippling.  The stipple pattern is ignored in this
 *		case.
 * VS_BORDER -	if this bit is set, generate an outline with no X
 *		through the middle and no stipple.  The stipple
 *		pattern is ignored in this case.
 */

#define VS_CROSS 1
#define VS_BORDER 2

static VersatecStyle *plotVersStyles;
static VersatecStyle *plotColorVersStyles;

char *plotVersatecColorNames[] = {
    "Black",
    "Cyan",
    "Magenta",
    "Yellow"
};

/*
 * ----------------------------------------------------------------------------
 * The parameters below control various aspects of the plotting
 * process.  The initial values are defaults for the versatec
 * printer.  However, many of them can be modified by users
 * with the "plot option" command.
 * ----------------------------------------------------------------------------
 */

int PlotVersWidth = 7040;	/* Number of dots across Versatec page.
				 * Should be a multiple of 32.
				 */
int PlotVersDotsPerInch = 200;	/* Dots per inch. */
int PlotVersSwathHeight = 64;	/* Width of swath to generate at one time,
				 * in dots.
				 */

/* Name of printer to use for output: */

static char *defaultPrinter = "versatec";
char *PlotVersPrinter;

/* Should we produce Versatec plots in color or B&W?  "true" means color,
 * "false" means B&W
 */

bool PlotVersColor = 0;

/* Command to use to actually print rasterized file.  Contains two %s'es,
 * which are supplied with the printer name and the name of the raster file.
 */

static char *defaultCommand = "/usr/ucb/lpr -P%s -c -s -r -JMagic %s";
char *PlotVersCommand;

/* Directory in which to create temporary file to hold raster: */

static char *defaultDirectory = "/usr/tmp";
char *PlotTempDirectory;

/* Name of fonts to use: */

static char *defaultIdFont = "vfont.I.12";
char *PlotVersIdFont;
static char *defaultNameFont = "vfont.B.12";
char *PlotVersNameFont;
static char *defaultLabelFont = "vfont.R.8";
char *PlotVersLabelFont;

/*
 * ----------------------------------------------------------------------------
 * The variables below are shared between the top-level Versatec
 * plotting procedure and the lower-level search functions.  They
 * contain specific information about how to generate the current
 * plot.  There area three coordinate systems of interest here:
 *
 * 1. Magic root coordinates.
 * 2. Raster coordinates:  based on printer pixels, one unit per pixel,
 *    where (0,0) corresponds to the lower-leftmost dot that will be
 *    printed.
 * 3. Swath coordinates:  the raster will be much too large to keep in
 *    memory all at once, so it's generated in a series of horizontal
 *    swaths.  The stippling routines all work in swath-relative
 *    coordinates, which are the same as raster coordinates except for
 *    a y-displacement (swathY below).
 * ----------------------------------------------------------------------------
 */

static Raster *raster = NULL;	/* Raster used to hold current swath. */
Point plotLL;		/* Point in root Magic coords that corresponds
				 * to (0,0) in raster coordinates.
				 */
int swathY;		/* The y-coordinate in raster coordinates that
				 * corresponds to (0,0) in swath coords.  It's
				 * always >= 0.
				 */
int scale;		/* How many (2**scaleShift)-ths of a pixel
				 * correspond to one Magic unit.
				 */
int scaleShift;		/* The idea is that one Magic unit is equal
				 * to scale/(2**scaleShift) pixels.
				 */
static Rect rootClip;		/* Total root area of the plot.  Used for
				 * clipping.
				 */
static Rect swathClip;		/* Rectangle used for clipping to the area of
				 * the current swath.  This is in swath
				 * coordinates.
				 */
static VersatecStyle *curStyle;	/* Current style being processed. */
static TileTypeBitMask curMask; /* Mask of layers currently being stippled.
				 * This is the AND of the mask from curStyle
				 * and the layers that the user wants plotted.
				 */
static int crossSize;		/* Length of each arm of the crosses used
				 * to draw labels, in pixel units.
				 */
static Font *labelFont;		/* Font to use when rendering labels. */
static Font *cellNameFont;	/* Font to use when rendering cell names. */
static Font *cellIdFont;	/* Font to use when rendering cell ids. */

/*
 * ----------------------------------------------------------------------------
 *	PlotVersTechInit --
 *
 * 	Called once at beginning of technology file read-in to initialize
 *	data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears out the list of things to plot.
 * ----------------------------------------------------------------------------
 */

Void
PlotVersTechInit()
{
    VersatecStyle *style;

    for (style = plotVersStyles; style != NULL; style = style->vs_next)
    {
	freeMagic((char *) style);
    }
    plotVersStyles = NULL;

    StrDup(&PlotVersPrinter, defaultPrinter);
    StrDup(&PlotVersCommand, defaultCommand);
    StrDup(&PlotTempDirectory, defaultDirectory);
    StrDup(&PlotVersIdFont, defaultIdFont);
    StrDup(&PlotVersNameFont, defaultNameFont);
    StrDup(&PlotVersLabelFont, defaultLabelFont);
}

/*
 * ----------------------------------------------------------------------------
 *	PlotColorVersTechInit --
 *
 * 	Called once at beginning of technology file read-in to initialize
 *	data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears out the list of things to plot.
 * ----------------------------------------------------------------------------
 */

Void
PlotColorVersTechInit()
{
    VersatecStyle *style;

    for (style = plotColorVersStyles; style != NULL; style = style->vs_next)
    {
	freeMagic((char *) style);
    }
    plotColorVersStyles = NULL;

    StrDup(&PlotVersPrinter, defaultPrinter);
    StrDup(&PlotVersCommand, defaultCommand);
    StrDup(&PlotTempDirectory, defaultDirectory);
    StrDup(&PlotVersIdFont, defaultIdFont);
    StrDup(&PlotVersNameFont, defaultNameFont);
    StrDup(&PlotVersLabelFont, defaultLabelFont);
}

/*
 * ----------------------------------------------------------------------------
 *	PlotVersTechLine --
 *
 * 	This procedure is invoked by the technology module once for
 *	each line in the "versatec" subsection of the "plot" section
 *	of the technology file.
 *
 * Results:
 *	Always returns TRUE (otherwise the technology module would
 *	abort Magic with a fatal error).
 *
 * Side effects:
 *	Builds up the table of Versatec styles.
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
bool
PlotVersTechLine(sectionName, argc, argv)
    char *sectionName;		/* Name of this section (unused). */
    int argc;			/* Number of arguments on line. */
    char *argv[];		/* Pointers to fields of line. */
{
    VersatecStyle *new;
    int i;

    new = (VersatecStyle *) mallocMagic(sizeof(VersatecStyle));

    DBTechNoisyNameMask(argv[0], &new->vs_layers);

    /* I'm not sure why this transformation is done, and it doesn't work.
     * --Bob
     */
#ifdef	NOT_DEF
    /* Replace non-primary contact images with primary images. */
    for (i = TT_TECHDEPBASE; i < DBNumTypes; i++)
    {
	if TTMaskHasType(&new->vs_layers, i)
	    TTMaskSetMask(&new->vs_layers, &DBLayerTypeMaskTbl[i]);
    }
    TTMaskAndMask(&new->vs_layers, &DBUserLayerBits);
#endif
    if (argc == 2)
    {
        if (strcmp(argv[1], "X") == 0)
	    new->vs_flags = VS_CROSS;
	else if (strcmp(argv[1], "B") == 0)
	    new->vs_flags = VS_BORDER;
	else
	{
	    TechError("Second field must be \"X\" or \"B\"\n");
	    freeMagic((char *) new);
	    return TRUE;
	}
    }
    else
    {
	int i, value;

	if (argc != 17)
	{
	    TechError("\"versatec\" lines must have either 2 or 17 fields.\n");
	    freeMagic((char *)new);
	    return TRUE;
	}
	new->vs_color = 0;
	new->vs_flags = 0;
	for (i = 0; i < 16; i++)
	{
	    (void) sscanf(argv[i+1], "%x", &value);
	    new->vs_stipple[i] = (value<<16) | (value & 0xffff);
#ifndef BIG_ENDIAN
	    new->vs_stipple[i] = PlotSwapBytes(new->vs_stipple[i]);
#endif  BIG_ENDIAN
	}
    }

    new->vs_next = plotVersStyles;
    plotVersStyles = new;

    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *	PlotColorVersTechLine --
 *
 * 	This procedure is invoked by the technology module once for
 *	each line in the "colorversatec" subsection of the "plot" section
 *	of the technology file.
 *
 * Results:
 *	Always returns TRUE (otherwise the technology module would
 *	abort Magic with a fatal error).
 *
 * Side effects:
 *	Builds up the table of ColorVersatec styles.
 * ----------------------------------------------------------------------------
 */

	/* ARGSUSED */
bool
PlotColorVersTechLine(sectionName, argc, argv)
    char *sectionName;		/* Name of this section (unused). */
    int argc;			/* Number of arguments on line. */
    char *argv[];		/* Pointers to fields of line. */
{
    VersatecStyle *new;
    static struct { char *l_str; int l_color; } colors[] = {
	"black",	BLACK,
	"cyan",		CYAN,
	"magenta",	MAGENTA,
	"yellow",	YELLOW,
	0
    };
    int i;

    new = (VersatecStyle *) mallocMagic(sizeof(VersatecStyle));

    DBTechNoisyNameMask(argv[0], &new->vs_layers);

    /* Replace non-primary contact images with primary images. */
    /****  I'm not sure why this is done, and it doesn't work.  --Bob
    for (i = TT_TECHDEPBASE; i < DBNumTypes; i++)
    {
	if TTMaskHasType(&new->vs_layers, i)
	    TTMaskSetMask(&new->vs_layers, &DBLayerTypeMaskTbl[i]);
    }
    TTMaskAndMask(&new->vs_layers, &DBUserLayerBits);
    ****/
    if (argc == 2)
    {
	new->vs_color = BLACK;
        if (strcmp(argv[1], "X") == 0)
	    new->vs_flags = VS_CROSS;
	else if (strcmp(argv[1], "B") == 0)
	    new->vs_flags = VS_BORDER;
	else
	{
	    TechError("Second field must be \"X\" or \"B\"\n");
	    freeMagic((char *) new);
	    return TRUE;
	}
    }
    else
    {
	int i, value;

	if (argc != 18)
	{
	    TechError(
		"\"colorversatec\" lines must have either 2 or 18 fields.\n");
	    freeMagic((char *)new);
	    return TRUE;
	}
	i = LookupStruct(argv[1], (LookupTable *) colors, sizeof colors[0]);
	if (i < 0)
	{
	    TechError("First field must be BLACK, CYAN, MAGENTA or YELLOW.\n");
	    freeMagic((char *)new);
	    return TRUE;
	}
	new->vs_color = colors[i].l_color;
	new->vs_flags = 0;
	for (i = 0; i < 16; i++)
	{
	    (void) sscanf(argv[i+2], "%x", &value);
	    new->vs_stipple[i] = (value<<16) | (value & 0xffff);
#ifndef BIG_ENDIAN
	    new->vs_stipple[i] = PlotSwapBytes(new->vs_stipple[i]);
#endif  BIG_ENDIAN
	}
    }

    new->vs_next = plotColorVersStyles;
    plotColorVersStyles = new;

    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * plotTransToSwath --
 *
 * 	Transforms a rectangle from Magic root coordinates to the
 *	coordinates of the current swath.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Dst is modified to hold a rectangle that has been transformed
 *	to swath coordinates.
 *
 * ----------------------------------------------------------------------------
 */

void
plotTransToSwath(src, dst)
    register Rect *src;		/* Rectangle in Magic root coords. */
    register Rect *dst;		/* Rectangle to be filled in with swath
				 * corresponding to src.
				 */
{
    dst->r_xbot = ((src->r_xbot - plotLL.p_x)*scale) >> scaleShift;
    dst->r_xtop = ((src->r_xtop - plotLL.p_x)*scale) >> scaleShift;
    dst->r_ybot = (((src->r_ybot - plotLL.p_y)*scale) >> scaleShift) - swathY;
    dst->r_ytop = (((src->r_ytop - plotLL.p_y)*scale) >> scaleShift) - swathY;
}

/*
 * ----------------------------------------------------------------------------
 *
 * plotVersLine  --
 *
 * 	This procedure plots a line of a given thickness.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current raster is modified.
 *
 * ----------------------------------------------------------------------------
 */

void
plotVersLine(area, widen)
    Rect *area;			/* The "corner" points of this rectangle
				 * give the endpoints of the line, in
				 * Magic root coordinates.
				 */
    int widen;			/* Amount by which to widen line.  0 means
				 * line is drawn one pixel wide, 1 means 3
				 * pixels wide, etc.
				 */
{
    Rect swathArea;

    plotTransToSwath(area, &swathArea);

    /* Handle Manhattan lines using rectangle-drawing, since it's faster. */

    if ((swathArea.r_xbot == swathArea.r_xtop) ||
	    (swathArea.r_ybot == swathArea.r_ytop))
    {
	GEO_EXPAND(&swathArea, widen, &swathArea);
	GEOCLIP(&swathArea, &swathClip);
	if ((swathArea.r_xbot <= swathArea.r_xtop) &&
		(swathArea.r_ybot <= swathArea.r_ytop))
	    PlotFillRaster(raster, &swathArea, PlotBlackStipple);
    }
    else
    PlotRastFatLine(raster, &swathArea.r_ll, &swathArea.r_ur, widen);
}

/*
 * ----------------------------------------------------------------------------
 *
 * plotVersRect --
 *
 * 	This procedure takes a rectangular area, given in Magic root
 *	coordinates, translates it to swath coordinates, and draws
 *	it as an outline of a given thickness.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies raster.
 *
 * ----------------------------------------------------------------------------
 */

void
plotVersRect(area, widen)
    register Rect *area;	/* Rectangular area to draw, in root
				 * coordinates.
				 */
    int widen;			/* If zero, rectangular outline is drawn
				 * one pixel wide.  If non-zero, the outline
				 * is widened by this many units on each
				 * side.
				 */
{
    Rect side;

    /* First, the bottom side. */

    if (area->r_xbot != area->r_xtop)
    {
	side = *area;
	side.r_ytop = side.r_ybot;
	plotVersLine(&side, widen);

	/* Now the top side, if it doesn't coincide with the bottom. */

	if (area->r_ybot != area->r_ytop)
	{
	    side = *area;
	    side.r_ybot = side.r_ytop;
	    plotVersLine(&side, widen);
	}
    }

    /* Now do the left side. */

    if (area->r_ybot != area->r_ytop)
    {
	side = *area;
	side.r_xtop = side.r_xbot;
	plotVersLine(&side, widen);

	/* Now the right side, if it doesn't coincide with the left. */

	if (area->r_xbot != area->r_xtop)
	{
	    side = *area;
	    side.r_xbot = side.r_xtop;
	    plotVersLine(&side, widen);
	}
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * plotVersTile --
 *
 *	This procedure is called for paint tiles.  It renders each tile
 *	in the current style, in the current swath.
 *
 * Results:
 *	Always returns 0 to keep the search alive.
 *
 * Side effects:
 *	Modifies the raster area.
 *
 * ----------------------------------------------------------------------------
 */

int
plotVersTile(tile, cxp)
    register Tile *tile;	/* Tile that's of type to be output. */
    TreeContext *cxp;		/* Describes search in progress. */
{
    Rect tileArea, rootArea, swathArea, edge;
    register Tile *neighbor;
    register Transform *trans = &cxp->tc_scx->scx_trans;

    /* Transform tile coords to root coords and then to swath coords. */
    
    TITORECT(tile, &tileArea);
    GEOTRANSRECT(trans, &tileArea, &rootArea);
    plotTransToSwath(&rootArea, &swathArea);

    /* Handle bordered and X'ed things specially. */
    
    if (curStyle->vs_flags & (VS_CROSS|VS_BORDER))
    {
	plotVersRect(&rootArea, 0);
	if ((curStyle->vs_flags & VS_CROSS)
	    && ((swathArea.r_xtop - swathArea.r_xbot) > 6)
	    && ((swathArea.r_ytop - swathArea.r_ybot) > 6))
	{
	    Rect r2;
	    plotVersLine(&rootArea, 0);
	    r2.r_xtop = rootArea.r_xbot;
	    r2.r_ybot = rootArea.r_ybot;
	    r2.r_xbot = rootArea.r_xtop;
	    r2.r_ytop = rootArea.r_ytop;
	    plotVersLine(&r2, 0);
	}
	return 0;
    }

    /* Clip and then stipple. */

    GEOCLIP(&swathArea, &swathClip);
    if (swathArea.r_xbot > swathArea.r_xtop) return 0;
    if (swathArea.r_ybot > swathArea.r_ytop) return 0;
    PlotFillRaster(raster, &swathArea, curStyle->vs_stipple);

    /* Now output lines for any edges between material of the type
     * currently being drawn and material of other types.  This is
     * done by searching along the tile's borders for neighbors that
     * have the wrong types.  First, search the tile's bottom border
     * (unless it is at infinity).
     */
    
    if (tileArea.r_ybot > TiPlaneRect.r_ybot)
    {
	edge.r_ybot = edge.r_ytop = tileArea.r_ybot;
	for (neighbor = tile->ti_lb; LEFT(neighbor) < tileArea.r_xtop;
		neighbor = neighbor->ti_tr)
	{
	    if (TTMaskHasType(&curMask, TiGetType(neighbor))) continue;
	    edge.r_xbot = LEFT(neighbor);
	    edge.r_xtop = RIGHT(neighbor);
	    if (edge.r_xbot < tileArea.r_xbot) edge.r_xbot = tileArea.r_xbot;
	    if (edge.r_xtop > tileArea.r_xtop) edge.r_xtop = tileArea.r_xtop;
	    GEOTRANSRECT(trans, &edge, &rootArea);
	    plotVersLine(&rootArea, 0);
	}
    }

    /* Now go along the tile's left border, doing the same thing.   Ignore
     * edges that are at infinity.
     */

    if (tileArea.r_xbot > TiPlaneRect.r_xbot)
    {
	edge.r_xbot = edge.r_xtop = tileArea.r_xbot;
	for (neighbor = tile->ti_bl; BOTTOM(neighbor) < tileArea.r_ytop;
		neighbor = neighbor->ti_rt)
	{
	    if (TTMaskHasType(&curMask, TiGetType(neighbor))) continue;
	    edge.r_ybot = BOTTOM(neighbor);
	    edge.r_ytop = TOP(neighbor);
	    if (edge.r_ybot < tileArea.r_ybot) edge.r_ybot = tileArea.r_ybot;
	    if (edge.r_ytop < tileArea.r_ytop) edge.r_ytop = tileArea.r_ytop;
	    GEOTRANSRECT(trans, &edge, &rootArea);
	    plotVersLine(&rootArea, 0);
	}
    }

    /* Same thing for the tile's top border. */

    if (tileArea.r_ytop < TiPlaneRect.r_ytop)
    {
	edge.r_ybot = edge.r_ytop = tileArea.r_ytop;
	for (neighbor = tile->ti_rt; RIGHT(neighbor) > tileArea.r_xbot;
		neighbor = neighbor->ti_bl)
	{
	    if (TTMaskHasType(&curMask, TiGetType(neighbor))) continue;
	    edge.r_xbot = LEFT(neighbor);
	    edge.r_xtop = RIGHT(neighbor);
	    if (edge.r_xbot < tileArea.r_xbot) edge.r_xbot = tileArea.r_xbot;
	    if (edge.r_xtop > tileArea.r_xtop) edge.r_xtop = tileArea.r_xtop;
	    GEOTRANSRECT(trans, &edge, &rootArea);
	    plotVersLine(&rootArea, 0);
	}
    }

    /* Finally, the right border. */

    if (tileArea.r_xtop < TiPlaneRect.r_xtop)
    {
	edge.r_xbot = edge.r_xtop = tileArea.r_xtop;
	for (neighbor = tile->ti_tr; TOP(neighbor) > tileArea.r_ybot;
		neighbor = neighbor->ti_lb)
	{
	    if (TTMaskHasType(&curMask, TiGetType(neighbor))) continue;
	    edge.r_ybot = BOTTOM(neighbor);
	    edge.r_ytop = TOP(neighbor);
	    if (edge.r_ybot < tileArea.r_ybot) edge.r_ybot = tileArea.r_ybot;
	    if (edge.r_ytop < tileArea.r_ytop) edge.r_ytop = tileArea.r_ytop;
	    GEOTRANSRECT(trans, &edge, &rootArea);
	    plotVersLine(&rootArea, 0);
	}
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * plotVersLabel --
 *
 * 	This procedure is invoked for labels.  It generates bits to
 *	display the label in the current raster swath.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	Modifies the raster.
 *
 * ----------------------------------------------------------------------------
 */

int
plotVersLabel(scx, label)
    SearchContext *scx;		/* Describes state of search when label
				 * was found.
				 */
    Label *label;		/* Label that was found. */
{
    Rect rootArea, swathArea, labelSize;
    Point point;
    int pos;

    /* Translate the label's area and relative position to root
     * coordinates and then to swath coordinates.  Figure out
     * the point relative to which the label is to be positioned.
     */

    GeoTransRect(&scx->scx_trans, &label->lab_rect, &rootArea);
    plotTransToSwath(&rootArea, &swathArea);
    pos = GeoTransPos(&scx->scx_trans, label->lab_pos);
    PlotTextSize(labelFont, label->lab_text, &labelSize);

    switch (pos)
    {
	case GEO_NORTH:
	case GEO_NORTHEAST:
	case GEO_NORTHWEST:
	    point.p_y = swathArea.r_ytop + crossSize + 2 - labelSize.r_ybot;
	    break;

	case GEO_CENTER:
	case GEO_WEST:
	case GEO_EAST:
	    point.p_y = (swathArea.r_ytop + swathArea.r_ybot)/2;
	    point.p_y -= (labelSize.r_ytop + labelSize.r_ybot)/2;
	    break;
	
	case GEO_SOUTH:
	case GEO_SOUTHEAST:
	case GEO_SOUTHWEST:
	    point.p_y = swathArea.r_ybot - crossSize - 2 - labelSize.r_ytop;
	    break;
    }
    switch (pos)
    {
	case GEO_WEST:
	case GEO_NORTHWEST:
	case GEO_SOUTHWEST:
	    point.p_x = swathArea.r_xbot - crossSize - 2 - labelSize.r_xtop;
	    break;
	
	case GEO_CENTER:
	case GEO_NORTH:
	case GEO_SOUTH:
	    point.p_x = (swathArea.r_xtop + swathArea.r_xbot)/2;
	    point.p_x -= (labelSize.r_xtop + labelSize.r_xbot)/2;
	    break;
	
	case GEO_EAST:
	case GEO_NORTHEAST:
	case GEO_SOUTHEAST:
	    point.p_x = swathArea.r_xtop + crossSize + 2 - labelSize.r_xbot;
	    break;
    }

    /* Output lines marking the label's area.  Different things are
     * done depending on whether the label is a point, a line, or an
     * area.
     */
    
    if ((rootArea.r_xbot == rootArea.r_xtop) &&
	    (rootArea.r_ybot == rootArea.r_ytop))
    {
	Rect tmp;

	/* Point label.  Output a cross. */

	tmp = swathArea;
	tmp.r_ytop += crossSize;
	tmp.r_ybot -= crossSize;
	GEO_EXPAND(&tmp, 1, &tmp);
	GEOCLIP(&tmp, &swathClip);
	if ((tmp.r_xbot <= tmp.r_xtop) &&
		(tmp.r_ybot <= tmp.r_ytop))
	    PlotFillRaster(raster, &tmp, PlotBlackStipple);
	tmp = swathArea;
	tmp.r_xtop += crossSize;
	tmp.r_xbot -= crossSize;
	GEO_EXPAND(&tmp, 1, &tmp);
	GEOCLIP(&tmp, &swathClip);
	if ((tmp.r_xbot <= tmp.r_xtop) &&
		(tmp.r_ybot <= tmp.r_ytop))
	    PlotFillRaster(raster, &tmp, PlotBlackStipple);
    }
    else
    {
	/* Line or rectangle.  Draw outline. */

	plotVersRect(&rootArea, 1);
    }

    /* Output the text for the label.  Before outputting the label,
     * erase the area where the label will appear in order to make
     * the label more visible.
     */

    labelSize.r_xbot += point.p_x - 1;
    labelSize.r_xtop += point.p_x + 1;
    labelSize.r_ybot += point.p_y - 1;
    labelSize.r_ytop += point.p_y + 1;
    GEOCLIP(&labelSize, &swathClip);
    PlotClearRaster(raster, &labelSize);
    PlotRasterText(raster, &swathClip, labelFont, label->lab_text, &point);

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * plotVersCell --
 *
 * 	This procedure is invoked for unexpanded cells.
 *
 * Results:
 *	Always returns 0 to keep the search from aborting.
 *
 * Side effects:
 *	The raster is modified to depict the cell's boundary,
 *	name, and instance id.
 *
 * ----------------------------------------------------------------------------
 */

int
plotVersCell(scx)
    SearchContext *scx;		/* Describes cell whose bbox is to
				 * be plotted.
				 */	
{
    char idName[100];
    Rect rootArea, swathArea, textSize;
    Point point;
    CellDef *def;

    /* Convert the cell's bounding box to root coordinates and then
     * draw as a thick outline.
     */

    def = scx->scx_use->cu_def;
    GeoTransRect(&scx->scx_trans, &def->cd_bbox, &rootArea);
    plotVersRect(&rootArea, 2);

    if (!PlotShowCellNames)
	return (0);

    /* Output the cell's name and id text. */
    plotTransToSwath(&rootArea, &swathArea);
    PlotTextSize(cellNameFont, def->cd_name, &textSize);
    point.p_x = (swathArea.r_xtop + swathArea.r_xbot)/2;
    point.p_x -= (textSize.r_xtop + textSize.r_xbot)/2;
    point.p_y = (2*swathArea.r_ytop + swathArea.r_ybot)/3;
    point.p_y -= (textSize.r_ytop + textSize.r_ybot)/2;
    PlotRasterText(raster, &swathClip, cellNameFont, def->cd_name, &point);

    (void) DBPrintUseId(scx, idName, 100);
    PlotTextSize(cellIdFont, idName, &textSize);
    point.p_x = (swathArea.r_xtop + swathArea.r_xbot)/2;
    point.p_x -= (textSize.r_xtop + textSize.r_xbot)/2;
    point.p_y = (swathArea.r_ytop + 2*swathArea.r_ybot)/3;
    point.p_y -= (textSize.r_ytop + textSize.r_ybot)/2;
    PlotRasterText(raster, &swathClip, cellIdFont, idName, &point);

    return 0;
}

/*
 * ----------------------------------------------------------------------------
 *
 * PlotVersatec --
 *
 * 	This procedure generates a raster file suitable for driving
 *	printers like the Versatec black-and-white family, and runs
 *	a spooling program to print the file.  If PlotVersColor is true,
 *      it will generate a versatec color plot file in straight color raster
 *      format.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lots of disk space is chewed up by the file.
 *
 * ----------------------------------------------------------------------------
 */

void
PlotVersatec(scx, layers, xMask, width)
    SearchContext *scx;			/* The use and area and transformation
					 * in this describe what to plot.
					 */
    TileTypeBitMask *layers;		/* Tells what layers to plot.  Only
					 * paint layers in this mask, and also
					 * expanded according to xMask, are
					 * plotted.  If L_LABELS is set, then
					 * labels on the layers are also
					 * plotted, if expanded according to
					 * xMask.  If L_CELL is set, then
					 * subcells that are unexpanded
					 * according to xMask are plotted as
					 * bounding boxes.
					 */
    int xMask;				/* An expansion mask, used to indicate
					 * the window whose expansion status
					 * will be used to determine
					 * visibility.  Zero means treat
					 * everything as expanded.
					 */
    float width;			/* How many inches across the plot
					 * should be.  If greater than the
					 * printer width or <= 0, then use
					 * the printer width.
					 */
{
    static char *yesNo[] = {"no", "yes", NULL};
    int dotsAcross, dotsDown, swathsDown, scaleDown;
    char fileName[200], command[300], answer[32];
    float length, mBytes;
    Transform tinv;
    int action;
    FILE *file;
    VersatecColor color;
    bool haveColorMessage;

    haveColorMessage = FALSE;
    GeoTransRect(&scx->scx_trans, &scx->scx_area, &rootClip);
    GEO_EXPAND(&rootClip, 1, &rootClip);
    dotsAcross = width * PlotVersDotsPerInch;
    if (dotsAcross <= 0 || dotsAcross > PlotVersWidth)
	dotsAcross = PlotVersWidth;

    /*
     * Compute the number of pixels per magic unit.
     * This number will be the fraction: scale / (1 << scaleShift).
     * In order to be reasonably sure that we have enough precision
     * in the numerator of the fraction, require that scale have at
     * least three bits (i.e., be greater than or equal to 4).
     */
    for (scale = 0, scaleShift = 4; ; scaleShift++)
    {
	scaleDown = 1 << scaleShift;
	scale = (scaleDown * dotsAcross) / (rootClip.r_xtop - rootClip.r_xbot);
	if (scaleShift >= 8 * sizeof (int))
	{
	    TxError("The area selected is too many lambda wide to plot.\n");
	    TxError("(There are numerical problems with rasterizing it).\n");
	    TxError("Try selecting a smaller area, or else asking for ");
	    TxError("a wider plot.\n");
	    return;
	}
	if (scale >= 4)
	    break;
    }

    /*
     * Compute scaling information, and tell the user how big the
     * plot will be.
     */
    dotsDown = ((rootClip.r_ytop - rootClip.r_ybot)*scale) >> scaleShift;
    swathsDown = (dotsDown + PlotVersSwathHeight - 1)/PlotVersSwathHeight;
    dotsDown = swathsDown * PlotVersSwathHeight;
    mBytes = ((PlotVersWidth/8)*dotsDown)/1000000.0;
    length = dotsDown;
    length /= PlotVersDotsPerInch;
    TxPrintf("Plot will be %.1f inches long, and will take", length);
    TxPrintf(" %.2f Megabytes in \"%s\".\n", 
	     PlotVersColor ? 4.0*mBytes : mBytes, PlotTempDirectory);
    do
    {
	TxPrintf("Do you still want the plot? [yes] ");
	if (TxGetLine(answer, sizeof answer) == NULL || answer[0] == '\0')
	{
	    action = 1;
	    break;
	}
    } while ((action = Lookup(answer, yesNo)) < 0);
    if (action == 0) return;

    /* The plot has been "approved".  Now obtain a swath raster if
     * we don't already have one.  If the swath size has changed,
     * recycle the raster for a new one.
     */
    
    if (raster == NULL)
	raster = PlotNewRaster(PlotVersSwathHeight, PlotVersWidth);
    else
    {
	if ((raster->ras_width != PlotVersWidth)
		|| (raster->ras_height != PlotVersSwathHeight))
	{
	    PlotFreeRaster(raster);
	    raster = PlotNewRaster(PlotVersSwathHeight, PlotVersWidth);
	}
    }

    /* Load font information for the plot, if it isn't already
     * loaded.
     */
    
    labelFont = PlotLoadFont(PlotVersLabelFont);
    cellNameFont = PlotLoadFont(PlotVersNameFont);
    cellIdFont = PlotLoadFont(PlotVersIdFont);
    if ((labelFont == NULL) || (cellNameFont == NULL) || (cellIdFont == NULL))
	return;

    /* Compute the name of the file to use for output, and open it. */

    sprintf(fileName, "%s/magicPlotXXXXXX", PlotTempDirectory);
    mktemp(fileName);
    file = PaOpen(fileName, "w", (char *) NULL, (char *) NULL, (char *) NULL,
	    (char **) NULL);
    if (file == NULL)
    {
	TxError("Couldn't open file \"%s\" to write plot.\n", fileName);
	return;
    }
    /* Set up the rest of the transformation variables.
     * Arrange for the plot to be centered on the page.
     */

    plotLL.p_y = rootClip.r_ybot;
    plotLL.p_x = (rootClip.r_xtop+rootClip.r_xbot)/2 - (PlotVersWidth*8)/scale;
    if (plotLL.p_x > rootClip.r_xbot)
	plotLL.p_x = rootClip.r_xbot;

    /* Compute a distance equal to 1/8th the size of a typical wire
     * (max of thicknesses of routing layers).  This is used to offset
     * text from labels and to compute cross size for point labels.
     */

    if (RtrMetalWidth > RtrPolyWidth)
	crossSize = (RtrMetalWidth*scale)/(scaleDown*8);
    else crossSize = (RtrPolyWidth*scale)/(scaleDown*8);
    if (crossSize < 2) crossSize = 2;

    /* Step down the page one swath at a time, rasterizing everything
     * that overlaps the current swath, then outputting the swath.
     */
    
    GeoInvertTrans(&scx->scx_trans, &tinv);
    for (color = BLACK; color <= YELLOW; color++)
    {
	int swathsDownThisColor;

	swathsDownThisColor = swathsDown;
	rasFileByteCount = 0;

	/* issue preamble for this color's raster. if we are doing colors */
	if (PlotVersColor) 
	{
	    if (PlotDumpColorPreamble(color, file, dotsDown, PlotVersWidth) != 0)
	      goto error;
	    if (SigInterruptPending)
	      goto error;
	}
	if (PlotVersColor)
	{
	    TxPrintf("\nDumping %s Raster:", plotVersatecColorNames[color]);
	    TxFlush();
	}
	for (swathsDownThisColor -= 1; 
	     swathsDownThisColor >= 0; 
	     swathsDownThisColor -= 1)
	{
	    SearchContext scx2;
	    Rect root, labelArea;
	    int labelHeight;

	    swathY = swathsDownThisColor * PlotVersSwathHeight;
	    PlotClearRaster(raster, (Rect *) NULL);

	    /* Compute the area of the swath that overlaps the portion of
	     * the layout we're plotting.
	     */
	
	    plotTransToSwath(&rootClip, &swathClip);
	    if (swathClip.r_xbot < 0) swathClip.r_xbot = 0;
	    if (swathClip.r_ybot < 0) swathClip.r_ybot = 0;
	    if (swathClip.r_xtop >= PlotVersWidth)
	      swathClip.r_xtop = PlotVersWidth - 1;
	    if (swathClip.r_ytop >= PlotVersSwathHeight)
	      swathClip.r_ytop = PlotVersSwathHeight - 1;

	    /* Compute the area of layout that overlaps this swath.  This is
	     * done twice, once for mask material and once for labels.  The
	     * separate computation for labels is because labels stick out
	     * from their positioning points.  We may have to search a larger
	     * area than just the swath in order to find all the labels that
	     * must be drawn in this swath.  Only the y-direction needs to
	     * be expanded this way, since we're only swathing in y.  Even
	     * non-label stuff has to be expanded slightly, because lines
	     * are drawn more than 1 pixel thick.
	     */
	
	    scx2 = *scx;
	    root.r_xbot = (scaleDown*swathClip.r_xbot)/scale + plotLL.p_x;
	    root.r_xtop = (scaleDown*swathClip.r_xtop)/scale + plotLL.p_x;
	    root.r_ybot = (scaleDown*(swathY-4))/scale + plotLL.p_y;
	    root.r_ytop = (scaleDown*(swathY+swathClip.r_ytop+4))/scale+plotLL.p_y;
	    GEO_EXPAND(&root, 1, &root);
	    GeoTransRect(&tinv, &root, &scx2.scx_area);

	    labelArea.r_xbot = root.r_xbot;
	    labelArea.r_xtop = root.r_xtop;
	    labelHeight =
	      (labelFont->fo_bbox.r_ytop - labelFont->fo_bbox.r_ybot) + 2;
	    labelArea.r_ybot =
	      (scaleDown*(swathY-crossSize-labelHeight))/scale
		+ plotLL.p_y;
	    labelArea.r_ytop =
	      (scaleDown*(swathY+swathClip.r_ytop+crossSize+labelHeight))/scale
		+ plotLL.p_y;
	    GEO_EXPAND(&labelArea, 1, &labelArea);

	    /* For each Versatec style, output stippled areas for all
	     * the tiles requested by the style. 
	     */

	    curStyle = PlotVersColor?plotColorVersStyles:plotVersStyles;
	    if (PlotVersColor && (plotColorVersStyles == NULL))
	    {
		if (!haveColorMessage) 
		{
		    TxError("\nWarning:  No color versatec styles are defined in the technology file!\nPlot will be monochrome.\n");
		}
		haveColorMessage = TRUE;
		curStyle = plotVersStyles;
	    }
	    if (curStyle == NULL)
	    {
		TxError("Warning:  No monochrome versatec styles are defined in the technology file!\nPlotting aborted.\n");
		return;
	    }

	    for ( ;
		 curStyle != NULL;
		 curStyle = curStyle->vs_next)
	    {
		/* if we are plotting in B&W, then visit all the tiles in this
		 * swath, otherwise only visit them if they should be
		 * plotted in the current style's color.
		 */

		if ((PlotVersColor == FALSE) || (curStyle->vs_color == color))
		{
		    TTMaskAndMask3(&curMask, layers, &curStyle->vs_layers);
		    (void) DBTreeSrTiles(&scx2, &curMask, xMask, plotVersTile,
				     (ClientData) NULL);
		}
	    }

	    /* Output labels, if they are wanted. */

	    if (TTMaskHasType(layers, L_LABEL) && color == BLACK)
	    {
		curMask = *layers;
		TTMaskSetType(&curMask, TT_SPACE);
		GeoTransRect(&tinv, &labelArea, &scx2.scx_area);
		(void) DBTreeSrLabels(&scx2, &curMask, xMask, (TerminalPath *) NULL,
				      plotVersLabel, (ClientData) NULL);
	    }

	    /* Output subcell bounding boxes, if they are wanted. */

	    if (TTMaskHasType(layers, L_CELL) && color == BLACK)
	    {
		(void) DBTreeSrCells(&scx2, xMask, plotVersCell, (ClientData) NULL);
	    }

	    TxPrintf("#");
	    TxFlush();

	    if (PlotDumpRaster(raster, file) != 0)
	      goto error;
	    if (SigInterruptPending)
	      goto error;
	}
	/* stop after first color (black) if this is not a color plot */
	if (PlotVersColor == FALSE) break;
	TxPrintf ("\nWrote %d bytes of data.\n", rasFileByteCount);
    }

    /* Close the file and issue the command to plot it. */

    TxPrintf("\n");
    fclose(file);
    sprintf(command, PlotVersCommand, PlotVersPrinter, fileName);
    if (system(command) != 0)
    {
	TxError("Couldn't execute spooler command to print \"%s\"\n",
		fileName);
    }
    return;

    error:
    TxError("\nVersatec plot aborted.\n");
    fclose(file);
    unlink(fileName);
}

