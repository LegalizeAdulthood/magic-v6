/*
 * plow.h --
 *
 * Exported definitions for the plow module.
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
 * rcsid $Header: plow.h,v 6.0 90/08/28 18:53:35 mayo Exp $
 */

/* Technology file clients */
extern Void PlowDRCInit(), PlowDRCFinal();
extern Void PlowTechInit(), PlowTechFinal();
extern bool PlowDRCLine(), PlowTechLine();

/* Called by CmdPlow() */
extern bool Plow();

/* Debugging command procedure */
extern Void PlowTest();

/* Exported tile type masks */
extern TileTypeBitMask PlowFixedTypes;		/* Non-stretchable types */
extern TileTypeBitMask PlowContactTypes;	/* Contact types */
extern TileTypeBitMask PlowCoveredTypes; 	/* Types that cannot be
						 * uncovered by plowing.
						 */
extern TileTypeBitMask PlowDragTypes;		/* Types that drag along
						 * trailing min-width
						 * material when they move.
						 */

/* Jog horizon ("plow horizon" command) */
extern int PlowJogHorizon;

/* TRUE if we should eliminate jogs after each plow operation */
extern bool PlowDoStraighten;
