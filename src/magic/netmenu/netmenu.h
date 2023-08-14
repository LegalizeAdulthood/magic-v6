/*
 * netmenu.h --
 *
 * Defines the interface provided by the netmenu module.
 * This module implements a menu-based system for editing
 * labels and netlists.
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
 * rcsid = "$Header: netmenu.h,v 6.0 90/08/28 18:51:02 mayo Exp $";
 */

#define _NETMENU

/* data structures */

/* procedures */

extern void NMinit();
extern void NMUnsetCell();
extern Void NMNewNetlist();
extern Void NMWriteNetlist();
extern char *NMAddTerm();
extern Void NMDeleteTerm();
extern Void NMJoinNets();
extern Void NMDeleteNet();
extern int NMEnumNets();
extern int NMEnumTerms();
extern char *NMNthNet();
extern char *NMTermInList();
extern Void NMVerify();
extern bool NMHasList();
extern void NMFlushNetlist();
extern char *NMNetlistName();
extern void NMMeasureAll();
