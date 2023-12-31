/*
 * runstats.h --
 *
 * Flags to RunStats()
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
 * rcsid "$Header: runstats.h,v 6.0 90/08/28 19:01:18 mayo Exp $"
 */

#define	_RUNSTATS

#define	RS_TCUM		01	/* Cumulative user and system time */
#define	RS_TINCR	02	/* User and system time since last call */
#define	RS_MEM		04	/* Size of heap area */

extern char *RunStats();
extern char *RunStatsRealTime();
