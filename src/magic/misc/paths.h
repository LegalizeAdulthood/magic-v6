/*
 * paths.h --
 *
 *     	Definitions of Unix filename paths used by Magic and related utility
 *	programs.
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
 * rcsid="$Header"
 */

#define _PATHS

/*
 * Paths used by 'ext2sim' and 'magicusage'.
 *
 */

#define	EXT_PATH	"~cad/lib/magic/%s ~cad/lib/magic/tutorial"
#define DOT_MAGIC_PATH	"~cad/lib/magic/sys ~ ."

/*
 * Paths used by 'magic'.
 *
 */
#define MAGIC_CMOS_PATH	"~cad/lib/magic/cmos ~cad/lib/magic/tutorial"
#define MAGIC_NMOS_PATH	"~cad/lib/magic/nmos ~cad/lib/magic/tutorial"
#define MAGIC_INIT_PATH \
	"~cad/lib/magic/cmos ~cad/lib/magic/nmos ~cad/lib/magic/tutorial"
#define MAGIC_SYS_PATH	". ~cad/lib/magic/sys"
#define MAGIC_SYS_DOT	"~cad/lib/magic/sys/.magic"
#define MAGIC_LIB_PATH	"~cad/lib/magic/%s"
#define HELPER_PATH	". ~cad/bin"	/* Used by graphics drivers */

/*
 * Path to default pager
 */
#ifdef SYSV
# ifdef hpux
#  define PAGERDIR "/usr/bin/more"
# else
#  define PAGERDIR "/usr/bin/pg"
# endif
#else
# define PAGERDIR "/usr/ucb/more"
#endif

/*
 * Other common paths.
 */
#define CAD_LIB_PATH	". ~cad/lib/"
