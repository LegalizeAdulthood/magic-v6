/*
 * irouter.h --
 *
 * This file defines the interface provided by the interactive router
 * module, which is the top-level module that controls interactive
 * routing.
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1987, 1990 Michael H. Arnold, Walter S. Scott, and  *
 *     * the Regents of the University of California.                      *
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
 * rcsid $Header: irouter.h,v 6.1 90/08/28 19:24:12 mayo Exp $
 */

#define _IROUTER

/*
 * Interface procedures.
 */

extern Void IRInit();
extern Void IRTest();
extern Void IRButtonProc();

/*
 * Technology file client procedures.
 * The sections should be processed in the order
 * listed below.
 */

    /* "irouter" section */
extern Void IRTechInit();
extern bool IRTechLine();

    /* "drc" section */
extern Void IRDRCInit();
extern bool IRDRCLine();

