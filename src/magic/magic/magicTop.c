/* magicTop.c -
 *
 *	The top level of the Magic VLSI Layout system.
 *
 *	This top level is purposely very short and is located directly in
 *	the directory used to remake Magic.  This is so that other
 *	programs may use all of Magic as a library but still provide their
 *	own 'main' procedure.  
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
 */

#ifndef lint
static char rcsid[]="$Header: magicTop.c,v 6.0 90/08/28 19:07:46 mayo Exp $";
#endif  not lint

/* dummy definitions */
YCRVert() {};
YCRHoriz() {};
Yacr() {};

/*---------------------------------------------------------------------------
 * main:
 *
 *	Top level of Magic.  Do NOT add code to this routine, as all code
 *	should go into `main.c' in the `main' directory.
 *
 * Results:	
 *	None.
 *
 * Side Effects:
 *	Invokes Magic to edit a VLSI design.
 *
 *----------------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char *argv[];
{
    /* One-line procedure -- do not add code here. */
    magicMain(argc, argv);
}

/* String containing the version number of magic.  Don't change the string
 * here, nor its format.  It is updated by the Makefile in this directory. 
 */
char *MagicVersion = "Magic - Version 6.3 - Last updated Thu Sep 13 13:21:25 PDT 1990  \n";
