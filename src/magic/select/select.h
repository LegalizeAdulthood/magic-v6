/*
 * select.h --
 *
 * Contains definitions for things that are exported by the
 * selection module.
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
 * rcsid="$Header: select.h,v 6.0 90/08/28 18:56:54 mayo Exp $"
 */

#define _SELINT

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_DATABASE
int err0 = Need_to_include_database_header;
#endif	_DATABASE

/* Procedures to modify the selection. */

extern void SelectArea();
extern void SelectChunk();
extern void SelectNet();
extern void SelectRegion();
extern void SelectInit();
extern void SelectClear();
extern void SelectCell();
extern void SelRemoveArea();
extern int  SelRemoveSel2();
extern int  SelectRemoveCellUse();

/* Procedures to enumerate what's in the selection. */

extern int SelEnumPaint();
extern int SelEnumCells();
extern int SelEnumLabels();

/* Procedures to operate on the selection. */

extern void SelectDelete();
extern void SelectCopy();
extern void SelectTransform();
extern void SelectExpand();
extern void SelectStretch();
extern void SelectArray();
extern void SelectDump();

/* The following is the root cell that contains the current selection. */

extern CellDef *SelectRootDef;

/* The dummy cell that actually holds the selection: */

extern CellDef *SelectDef;
extern CellUse *SelectUse;
