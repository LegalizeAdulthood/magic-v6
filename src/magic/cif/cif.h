/*
 * cif.h --
 *
 * This procedure defines things that are exported by the
 * cif module to the rest of the world.
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
 * rcsid "$Header: cif.h,v 6.0 90/08/28 18:05:27 mayo Exp $
 */

#define _CIF

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_DATABASE
int err1 = Need_to_include_database_header;
#endif	_DATABASE

/* Procedures that parse the cif sections of a technology file. */

extern Void CIFTechInit();
extern bool CIFTechLine();
extern Void CIFTechFinal();
extern Void CIFReadTechInit();
extern bool CIFReadTechLine();
extern Void CIFReadTechFinal();

/* Externally-visible procedures: */

extern void CIFSeeLayer();
extern void CIFSeeHierLayer();
extern void CIFPrintStats();

extern bool CIFWrite();
extern void CIFReadFile();

extern void CIFSetStyle();
extern void CIFSetReadStyle();

extern int CIFOutputScaleFactor();
