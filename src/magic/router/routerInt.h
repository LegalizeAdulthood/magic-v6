/*
 * routerInt.h -
 *
 *	Internal definitions for Router module.
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
 * rcsid="$Header: routerInt.h,v 6.1 90/08/28 19:26:36 mayo Exp $" 
 */

/*
 *	Declarations local to the router module,
 *	but global to all source files within the router module.
 */

int	rtrTarget;			/* Via minimization, target type	*/
int	rtrReplace;			/* Via minimization, replacement type	*/
int	rtrDelta;			/* Change in layer width		*/
