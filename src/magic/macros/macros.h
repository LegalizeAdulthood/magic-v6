/*
 * macros.h --
 *
 * Routines to define and retrieve macros
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
 * rcsid "$Header: macros.h,v 6.0 90/08/28 18:47:04 mayo Exp $"
 */

#define _MACROS
#ifndef	_MAGIC
	err = Need_to_include_magic_h
#endif

/* procedures */
extern void MacroDefine();
extern void MacroDelete();
extern char *MacroRetrieve();	/* returns a malloc'ed string */

