/*
 * mzDebug.h --
 *
 * Declarations of debugging flag "handles" for maze router.
 *
 * This include file is referenced inside mzInternal.h, hence hiding this
 * file from make.  This is so that debuging flags can be added without
 * forcing recompilation of everything.
 *
 * Debug flags are defined and registered with the debug module in mzInit.c
 * NOTE: the values of the externs defined below are NOT THE FLAG VALUES,
 * rather they are handles for the flags - flags are tested with calls to
 * DebugIsSet().
 *
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1988, 1990 Michael H. Arnold and the Regents of the *
 *     * University of California.                                         *
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
 * rcsid $Header: mzDebug.h,v 6.1 90/08/28 19:35:01 mayo Exp $
 *
 */

extern int mzDebStep;	/* single step - pauses after each path extension,
			   and prints statistics */
extern int mzDebMaze;	/* traces maze router - details on selection of
			   best path and extension from it. */

