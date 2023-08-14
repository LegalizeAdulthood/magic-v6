/*
 * styles.h --
 *
 * Definitions of styles used for system purposes.
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
 * rcsid:  $Header: styles.h,v 6.0 90/08/28 18:47:45 mayo Exp $
 */

#define _STYLES 1

/* This file just defines standard display styles expected by Magic.
 * The first 128 styles are for the mask information, but styles
 * after that are reserved for system purposes, such as drawing
 * and erasing tools, etc.
 *
 * STYLE_SOLIDHIGHLIGHTS:    used to draw solid highlight areas.
 * STYLE_MEDIUMHIGHLIGHTS:   used to draw highlights in a medium-weight stipple.
 * STYLE_PALEHIGHLIGHTS:     used to draw highlights in a pale stipple.
 * STYLE_HORIZHIGLIGHTS:     used to draw highlights with horizontal lines.
 * STYLE_VERTHIGLIGHTS:      used to draw highlights with vertical lines.
 * STYLE_OUTLINEHIGHLIGHTS:  used to draw highlights as solid box outlines.
 * STYLE_DRAWBOX:	     same as STYLE_OUTLINEHIGHLIGHTS (old and archaic).
 * STYLE_DOTTEDHIGLIGHTS:    used to draw highlights with dotted box outlines.
 * STYLE_ERASEBOX:	     used to erase the box from the screen.
 * STYLE_ERASEHIGHLIGHTS:    used to erase all highlights from an area.
 * STYLE_ERASEALL:	     used to erase all information from the screen.
 * STYLE_ERASEALLBUTTOOLS:   used to erase everything but the tools.
 * STYLE_LABEL:		     used to draw labels.
 * STYLE_BBOX:		     used to draw cell bounding boxes and names.
 * STYLE_GRID:		     used to draw grid on the screen (dotted lines).
 * STYLE_SOLIDGRID:	     an alternate grid, drawn with solid lines.
 * STYLE_ORIGIN:	     used to display a box at origin of edit cell.
 * STYLE_DRAWTILE:	     used to draw tiles for *watch command.
 * STYLE_BORDER:	     used to draw borders around windows.
 * STYLE_ELEVATOR	     used by the window package to draw the elevator
 *			     (slug) in the scroll bars.
 * STYLE_CAPTION:	     used to draw window captions.
 * STYLE_BACKGROUND:	     used to draw the areas outside of windows.
 * STYLE_CMEDIT		     used to display the color being edited by the
 *			     colormap editor.
 * STYLE_WHITE, etc.	     used to generate common colors, for use in
 *			     menus.  Most colors have several styles,
 *			     corresponding to different saturation levels.
 *			     These colors are numbered where 1 corresponds
 *			     to a very pale color and a large number is used
 *			     for a highly saturated (rich) color.  Unnumbered
 *			     colors correspond to the most saturated ones.
 *
 * If any of the style numbers below are changed, all of the display styles
 * files must be modified so that they agree.
 */

/* Styles for drawing and erasing highlights: */
#define STYLE_SOLIDHIGHLIGHTS	160
#define STYLE_MEDIUMHIGHLIGHTS	161
#define STYLE_PALEHIGHLIGHTS	162
#define STYLE_HORIZHIGHLIGHTS	163
#define STYLE_VERTHIGHLIGHTS	164
#define	STYLE_DRAWBOX		165
#define STYLE_OUTLINEHIGHLIGHTS	165
#define STYLE_DOTTEDHIGHLIGHTS	166
#define	STYLE_ERASEBOX		167
#define STYLE_ERASEHIGHLIGHTS	168
#define	STYLE_ERASEALL		169
#define	STYLE_ERASEALLBUTTOOLS	170

/* Other miscellaneous styles */
#define	STYLE_LABEL		171
#define	STYLE_BBOX		172
#define	STYLE_GRID		173
#define	STYLE_SOLIDGRID		174
#define STYLE_ORIGIN		175
#define	STYLE_DRAWTILE		176
#define	STYLE_BORDER		177
#define STYLE_ELEVATOR		178
#define	STYLE_CAPTION		179
#define	STYLE_BACKGROUND	180
#define STYLE_CMEDIT		181

/* Pure colors */
#define	STYLE_WHITE		129
#define STYLE_GRAY1		130
#define STYLE_GRAY2		131
#define STYLE_GRAY		131
#define STYLE_BLACK		132
#define STYLE_RED1		133
#define STYLE_PINK		133
#define STYLE_RED2		134
#define STYLE_RED3		135
#define	STYLE_RED		135
#define STYLE_GREEN1		136
#define STYLE_GREEN2		137
#define STYLE_GREEN3		138
#define	STYLE_GREEN		138
#define STYLE_BLUE1		139
#define STYLE_BLUE2		140
#define STYLE_BLUE3		141
#define	STYLE_BLUE		141
#define STYLE_PURPLE1		142
#define STYLE_PURPLE2		143
#define STYLE_PURPLE		143
#define STYLE_YELLOW1		144
#define STYLE_YELLOW2		145
#define STYLE_YELLOW		145
#define STYLE_ORANGE1		146
#define STYLE_ORANGE2		147
#define STYLE_ORANGE		147
#define STYLE_BROWN1		148
#define STYLE_BROWN2		149
#define STYLE_BROWN		149
#define STYLE_MAGENTA		150
#define STYLE_CYAN		151

/* Styles 200 through 219 are reserved for Mocha Draw. */
#define STYLE_MDBACKGROUND	200
#define STYLE_MD1		201
#define STYLE_MD2		202
#define STYLE_MD3		203
#define STYLE_MD4		204
#define STYLE_MD5		205
#define STYLE_MD6		206
#define STYLE_MD7		207
#define STYLE_MD8		208
#define STYLE_MD9		209
#define STYLE_MD10		210
#define STYLE_MD11		211
#define STYLE_MD12		212
#define STYLE_MD13		213
#define STYLE_MD14		214
#define STYLE_MD15		215
#define STYLE_MD16		216
#define STYLE_MD17		217
#define STYLE_MD18		218
#define STYLE_MD19		219


/*
 * Here are the cursors defined in the standard styles file.
 *
 */

/* misc patterns */
#define	STYLE_CURS_NORMAL	0
#define	STYLE_CURS_STAR		1

/* corners */
#define	STYLE_CURS_LLCORNER	2
#define	STYLE_CURS_LRCORNER	3
#define	STYLE_CURS_ULCORNER	4
#define	STYLE_CURS_URCORNER	5

/* boxes */
#define	STYLE_CURS_LLBOX	6
#define	STYLE_CURS_LRBOX	7
#define	STYLE_CURS_ULBOX	8
#define	STYLE_CURS_URBOX	9

/* entire windows */
#define	STYLE_CURS_LLWIND	10
#define	STYLE_CURS_LRWIND	11
#define	STYLE_CURS_ULWIND	12
#define	STYLE_CURS_URWIND	13

/* corners of windows */
#define	STYLE_CURS_LLWINDCORN	14
#define	STYLE_CURS_LRWINDCORN	15
#define	STYLE_CURS_ULWINDCORN	16
#define	STYLE_CURS_URWINDCORN	17

/* netlist editing icon */
#define STYLE_CURS_NET		18

/* wiring icon */
#define STYLE_CURS_ARROW	19

/* irouting icon */
#define STYLE_CURS_IROUTE	20

/* rsim icon */
#define STYLE_CURS_RSIM		21
