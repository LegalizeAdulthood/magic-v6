/*
 * irInternal.h --
 *
 * This file defines data structures and constants and declares
 * variables INTERNAL TO THE IROUTER
 * but shared by two or more source files.  
 * 
 * Structures etc. that are exported by the irouter are defined in
 * irouter.h.
 *
 * Structures etc. that are local to a given source 
 * file are declared at the top of that source file.
 *
 * Structures, etc.,  specific to a given function are usually defined at 
 * the head of that function.
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
 * rcsid $Header: irInternal.h,v 6.1 90/08/28 19:23:54 mayo Exp $
 */

#ifndef _MAGIC
int err0 = Need_to_include_magic_header;
#endif
#ifndef _DATABASE
int err1 = Need_to_include_database_header;
#endif
#ifndef _MZROUTER
int err2 = Need_to_include_mzrouter_header;
#endif
#ifndef _IROUTER
int err3 = Need_to_include_i_router_header;
#endif

/* ------------------------ Version String -------------------------- */
#define IROUTER_VERSION "0.6"

/* ------------------------ Debugging flags ------------------------- */
#include "irDebug.h"
extern ClientData irDebugID;

/* ------------------------ Routines Global to IRouter --------------- */
extern RouteType *irFindRouteType();
extern RouteLayer *irFindRouteLayer();
extern RouteContact *irFindRouteContact();

extern char *irRepeatChar();

/* ------------------------ Data Global to IRouter ------------------- */
extern MazeParameters *irMazeParms;

/* irRouteLayer, Contact, and Type pointers should be identical to
 * corresponding fields in irMazeParms.  They are referenced directly
 * instead of through parms struc for historical reasons 
 */
extern RouteLayer *irRouteLayers;
extern RouteContact *irRouteContacts;
extern RouteType *irRouteTypes;

extern int irRouteWid;		/* Reference window for subcell expansion
				   modes */

/* start types = methods of specifying route start */
#define ST_CURSOR	1
#define ST_LABEL	2
#define ST_POINT	3

/* dest types = methods of specifying route destination */
#define DT_BOX		1
#define DT_LABEL	2
#define DT_RECT		3
#define DT_SELECTION	4

