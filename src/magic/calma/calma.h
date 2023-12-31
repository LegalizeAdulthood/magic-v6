/*
 * calma.h --
 *
 * This file defines things that are exported by the
 * calma module to the rest of the world.
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
 * rcsid $Header: calma.h,v 6.0 90/08/28 18:03:51 mayo Exp $
 */

#define _CALMA

/* Externally visible variables */
extern bool CalmaDoLabels;
extern bool CalmaDoLower;
extern bool CalmaFlattenArrays;

/* Externally-visible procedures: */
extern bool CalmaWrite();
extern void CalmaReadFile();
