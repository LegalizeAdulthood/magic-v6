/*
 * wireInt.h --
 *
 * Contains definitions for things that are used by more than
 * one file in the wiring module, but aren't exported.
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
 * rcsid $Header: wireInt.h,v 6.0 90/08/28 19:02:51 mayo Exp $
 */

#define _WIREINT

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_DATABASE
int err0 = Need_to_include_database_header;
#endif	_DATABASE

/* Undo-able wiring parameters: */

extern TileType WireType;
extern int WireWidth;
extern int WireLastDir;

/* Undo procedure: */

extern void WireRememberForUndo();
extern void WireUndoInit();
