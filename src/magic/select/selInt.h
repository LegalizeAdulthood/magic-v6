/*
 * selInt.h --
 *
 * Contains definitions that are private to the implementation of
 * the select module.  No other module should need anything in here.
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
 * rcsid="$Header: selInt.h,v 6.0 90/08/28 18:56:47 mayo Exp $"
 */

#define _SELINT

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_DATABASE
int err0 = Need_to_include_database_header;
#endif	_DATABASE

/* Procedures, variables, and records that are shared between
 * files:
 */

extern Void SelRedisplay();
extern void SelSetDisplay();
extern void SelUndoInit();
extern void SelRememberForUndo();
extern void SelectAndCopy2();

extern CellUse *Select2Use;
extern CellDef *Select2Def;

extern CellUse *selectLastUse;
