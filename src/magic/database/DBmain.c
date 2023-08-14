/*
 * DBMain.c --
 *
 * Converted version of 'main.c' from the Magic layout editor.
 * For programs that include the standalone version of the
 * database module.  Defines several externals to resolve
 * references from the database module to those procedures
 * or variables.  Users wishing to use the real version of
 * these procedures instead of the dummies will have to
 * provide their own version of the code in this file.
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
 *		      Lawrence Livermore National Laboratory
 */

#ifndef lint
static char rcsid[] = "$Header: DBmain.c,v 6.0 90/08/28 18:10:01 mayo Exp $";
#endif  not lint

#include <stdio.h>
#include <sgtty.h>	
#include <sys/types.h>
#include <sys/times.h>
#include <signal.h>
#include "magic.h"
#include "geometry.h"
#include "utils.h"
#include "tech.h"
#include "hash.h"
#include "tile.h"
#include "database.h"
#include "undo.h"
#include "signals.h"

/* Search path */
char *Path = ".";

/* Used to find cells */
char CellLibPath[1024] = "~cad/lib/magic/nmos:~cad/lib/magic/tutorial";

/* Used to find color maps, styles, technologies, etc. */
char SysLibPath[1024] = "~cad/lib/magic/sys";

/*
 * See the file main.h for a description of the information kept
 * pertaining to the edit cell.
 */
CellUse	*EditCellUse = NULL;

/* Imports */
extern char *ctime();

/* Forward declarations */
sigRetVal dbMainOnTerm();
void dbMainDummyClient();

/*
 * ----------------------------------------------------------------------------
 *
 * DBMainInit --
 *
 * Initialize the database module, using technology techName.
 * The primary purpose of this procedure is to read only those
 * sections of the technology file pertinent to the database.
 * Clients wishing additionally to use procedures other than
 * those in the database should write their own version of
 * this procedure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reads in the technology file for techName and initializes
 *	all of the local variables of the database module necessary
 *	prior to calling any other procedures in the module.
 *
 * ----------------------------------------------------------------------------
 */

DBMainInit(techName)
    char *techName;
{
    static struct sigvec vec = {0, 0, 0};
    SectionID sec_tech, sec_planes, sec_types;
    SectionID sec_connect, sec_contact, sec_compose;
    int (*nullProc)() = 0;

    /* Set the technology */
    TechDefault = StrDup((char **) NULL, techName);

    /* initialize modules and the text display */
    DBCellInit();
    DBVerbose = FALSE;

    /* Handle termination (mainly for case where we run out of memory) */
#ifdef SYSV
    signal(SIGTERM,dbMainOnTerm);
#else
    vec.sv_handler = dbMainOnTerm;
    sigvec(SIGTERM, &vec, 0);
#endif SYSV

    /* initialize technology */
    TechInit();
    TechAddClient("tech", DBTechInit, DBTechSetTech, nullProc,
		    (SectionID) 0, &sec_tech);
    TechAddClient("planes",	DBTechInitPlane, DBTechAddPlane, nullProc,
		    (SectionID) 0, &sec_planes);
    TechAddClient("types", DBTechInitType, DBTechAddType, DBTechFinalType,
		    sec_planes, &sec_types);
    dbMainDummyClient("styles");
    TechAddClient("contact", DBTechInitContact,
		    DBTechAddContact, DBTechFinalContact,
		    sec_types|sec_planes, &sec_contact);
    TechAddClient("compose", DBTechInitCompose,
		    DBTechAddCompose, DBTechFinalCompose,
		    sec_types|sec_planes|sec_contact, &sec_compose);
    TechAddClient("connect", DBTechInitConnect,
		    DBTechAddConnect, DBTechFinalConnect,
		    sec_types|sec_planes|sec_contact, &sec_connect);

    dbMainDummyClient("cifoutput");
    dbMainDummyClient("cifinput");
    dbMainDummyClient("newrouter");
    dbMainDummyClient("irouter");
    dbMainDummyClient("drc");
    dbMainDummyClient("extract");
    dbMainDummyClient("wiring");
    dbMainDummyClient("router");
    dbMainDummyClient("plowing");
    dbMainDummyClient("plot");
    if (!TechLoad(TechDefault))
    {
	TxError("Cannot load default (%s) technology\n", TechDefault);
	MainExit(0);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * dbMainDummyClient --
 *
 * Used to process sections of a .tech file that aren't needed for
 * batch-mode use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds a dummy client to for processing section "name" -- this
 *	only serves to prevent an error message when section name is
 *	encountered while reading the .tech file.
 *
 * ----------------------------------------------------------------------------
 */

bool 
trueProc() { return TRUE; }

Void 
voidProc() { }

void
dbMainDummyClient(name)
    char *name;
{
    static int dummySec;
    TechAddClient(name, voidProc, trueProc, voidProc, (SectionID) 0, &dummySec);
};

/*
 * ----------------------------------------------------------------------------
 *
 * dbMainOnTerm --
 *
 * Catch the terminate (SIGTERM) signal.
 * Force all modified cells to be written to disk (in new files, of course).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes cells out to disk (by calling DBPanicSave()).
 *	Exits.
 *
 * ----------------------------------------------------------------------------
 */

sigRetVal
dbMainOnTerm()
{
    DBPanicSave();
    exit (1);
    /*NOTREACHED*/
}

/*
 * ----------------------------------------------------------------------------
 *
 * DUMMY PROCEDURES:
 *
 * These are stubs needed to satisfy references made by the database
 * module to procedures external to it.
 *
 * ----------------------------------------------------------------------------
 */

/* DBWIND */
int DBWFeedbackCount = 0;
TileTypeBitMask DBWStyleToTypesTbl[256];
void DBWAreaChanged() {}
void DBWLabelChanged() {}

/* SIGNALS */
bool SigInterruptPending = FALSE;
void SigEnableInterrupts() {}
void SigDisableInterrupts() {}

/* UNDO */
int UndoDisableCount = 1;
void UndoDisable() {}
void UndoEnable() {}
void UndoFlush() {}
UndoType UndoAddClient() { return 1; }
UndoEvent *UndoNewEvent() { return ((UndoEvent *) NULL); }

/* WINDOWS */
void WindAreaChanged() {}
void WindUpdate() {}

/* DRC */
void DRCCheckThis() {}
