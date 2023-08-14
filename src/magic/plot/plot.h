/*
 * plot.h --
 *
 * Contains definitions for things that are exported by the
 * plot module.
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
 * rcsid $Header: plot.h,v 6.0 90/08/28 18:51:45 mayo Exp $
 */

#define _PLOT

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC

/* Technology-file reading procedures: */

extern Void PlotTechInit();
extern bool PlotTechLine();
extern bool PlotTechFinal();

/* Top-level plot procedures: */

extern void PlotGremlin();
extern void PlotVersatec();
extern void PlotSetParam();
extern void PlotPrintParams();
