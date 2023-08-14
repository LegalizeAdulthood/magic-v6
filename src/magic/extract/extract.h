/*
 * extract.h --
 *
 * Defines the exported interface to the circuit extractor.
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
 * rcsid "$Header: extract.h,v 6.0 90/08/28 18:15:41 mayo Exp $"
 */

#define _EXTRACT

/* Extractor warnings */
#define	EXTWARN_DUP	0x01	/* Warn if two nodes have the same name */
#define	EXTWARN_LABELS	0x02	/* Warn if connecting to unlabelled subcell
				 * node.
				 */
#define	EXTWARN_FETS	0x04	/* Warn about badly constructed fets */

#define	EXTWARN_ALL	(EXTWARN_DUP|EXTWARN_LABELS|EXTWARN_FETS)

extern int ExtDoWarn;		/* Bitmask of above */

/* Extractor options */
#define	EXT_DOADJUST		0x01	/* Extract hierarchical adjustments */
#define	EXT_DOCAPACITANCE	0x02	/* Extract capacitance */
#define	EXT_DOCOUPLING		0x04	/* Extract coupling capacitance */
#define	EXT_DORESISTANCE	0x08	/* Extract resistance */
#define	EXT_DOLENGTH		0x10	/* Extract pathlengths */
#define	EXT_DOALL		0x1f	/* ALL OF THE ABOVE */

extern int ExtOptions;		/* Bitmask of above */

extern bool ExtTechLine();
extern Void ExtTechFinal();
