/*
 * touchingtypes.h --
 *     Header, types TouchingTypes() function. 
 *     (TouchingTypes() returns mask of types touching a given point.)
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
 * rcsid $Header: touchingtypes.h,v 6.0 90/08/28 19:01:26 mayo Exp $
 */

#define	_LIST

/* -------------------- Functions (exported by touching.c) ---------------- */
extern TileTypeBitMask TouchingTypes();
