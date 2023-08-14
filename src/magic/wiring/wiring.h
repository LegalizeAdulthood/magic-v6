/*
 * wiring.h --
 *
 * Contains definitions for things that are exported by the
 * wiring module.
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
 * rcsid $Header: wiring.h,v 6.0 90/08/28 19:02:57 mayo Exp $
 */

#define _WIRING

#ifndef	_MAGIC
int err0 = Need_to_include_magic_header;
#endif	_MAGIC
#ifndef	_DATABASE
int err0 = Need_to_include_database_header;
#endif	_DATABASE

/* Table that defines the shape of contacts and the layers that they
 * connect.  This definition allows some layers to extend around the
 * contact, to support technologies where the contact pads are different
 * sizes for the different layers that the contact connects.
 */

typedef struct
{
    TileType con_type;		/* Type of material that forms core of
				 * contact.
				 */
    int con_size;		/* Minimum size of this contact (size of
				 * minimum con_type area).
				 */
    TileType con_layer1;	/* First of two layers that the contact
				 * really isn't a contact.
				 */
    int con_surround1;		/* How much additional material of type
				 * con_layer1 must be painted around the
				 * edge of the contact.
				 */
    TileType con_layer2;	/* Same information for second layer that
				 * the contact connects.
				 */
    int con_surround2;
} Contact;

extern Contact *WireContacts[];	/* Points to all the contacts that are
				 * defined.
				 */
extern int WireNumContacts;	/* Number of valid entries in WireContacts. */

/* Procedures for placing wires: */

extern void WirePickType();
extern void WireAddLeg();
extern void WireAddContact();

/* Legal values for the "direction" parameter to WireAddLeg: */

#define WIRE_CHOOSE 0
#define WIRE_HORIZONTAL 1
#define WIRE_VERTICAL 2

/* Procedures for reading the technology file: */

extern Void WireTechInit();
extern bool WireTechLine();
extern Void WireTechFinal();

/* Initialization: */

extern void WireInit();
