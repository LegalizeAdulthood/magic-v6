/*
 * magic.h --
 *
 * Global definitions for all MAGIC modules
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
 * Needs to include <stdio.h> and <sys/types.h>
 *
 * rcsid="$Header"
 */

#define	_MAGIC

/* --------------------- Universal pointer type ----------------------- */

typedef char *	ClientData;

/* --------------------------- Booleans ------------------------------- */

#define	bool	int
#define	TRUE	1
#define	FALSE	0

/* --------------------------- Infinities ------------------------------ */

/* maximum representable positive integer */
/* (special case vaxes to avoid compiler bug in ultrix) */
#ifdef vax
#define MAXINT 0x7fffffff
#else
#define MAXINT (((unsigned long) ~0) >> 1)
#endif

/* ----------------------- Simple functions --------------------------- */

#define MAX(a,b)	(((a) < (b)) ? (b) : (a))
#define MIN(a,b)	(((a) > (b)) ? (b) : (a))
#define	ABS(x)		(((x) >= 0)  ? (x) : -(x))
#define	ABSDIFF(x,y)	(((x) < (y)) ? (y) - (x) : (x) - (y))
#define ODD(i)		(i&1)
#define EVEN(i)		(!(i&1))
/* Round anything (e.g. a double) to nearest integer */
#define ROUND(x) ((int)((x)+.5))

/* ------------ Function headers of globally used functions ----------- */

extern char *strcpy(), *strncpy(), *index(), *rindex();
extern char *strcat(), *strncat();


/* -------------------------- Search paths ---------------------------- */

extern char CellLibPath[];	/* Used as last resort in finding cells. */
extern char SysLibPath[];	/* Used as last resort in finding system
				 * files like color maps, styles, etc.
				 */

/* --------------------- Debugging and assertions --------------------- */

#ifdef	PARANOID
#define	ASSERT(p, where) \
    ((!(p)) \
	? (sprintf(AbortMessage, "%s botched: %s\n",  \
	    where, "p"), \
	fputs(AbortMessage, stderr), \
	niceabort(), \
	TRUE) \
	: FALSE)
#else
#define	ASSERT(p, where)	(FALSE)
#endif	PARANOID

/* This version of Magic uses the #define flags defined by the compilers on
 * different machines, not the old-style flags defined by the Magic
 * maintainer.  If we goof, however, the compiler should choke on the following
 * code, yielding a (somewhat) intelligible error message.
 */
#ifdef 	SUN2
    err1 = Dont_use_SUN2_flag_as_the_compiler_defines_the_sun_flag;
#endif	SUN2
#ifdef 	SUN3
    err2 = Dont_use_SUN3_flag_as_the_compiler_defines_the_sun_flag;
#endif	SUN3
#ifdef 	VAX
    err3 = Dont_use_VAX_flag_as_the_compiler_defines_the_vax_flag;
#endif	VAX


/* ------------------------ Malloc/free ------------------------------- */

/*
 * Magic has its own versions of malloc() and free(), called mallocMagic()
 * and freeMagic().  Magic procedures should ONLY use these procedures.
 * Just for the sake of robustness, though, we define malloc and free
 * here to error strings.
 */
#define	malloc	You_should_use_the_Magic_procedure_mallocMagic instead
#define	free	You_should_use_the_Magic_procedure_freeMagic instead
#define calloc	You_should_use_the_Magic_procedure_callocMagic instead

/* ---------- Flag for global variables (for readability) ------------- */
/*	      Also a Void type, which is like void except that we	*/
/*	      can declare pointers to functions that return it.	        */

#define	global	/* Nothing */
#define Void	int

/* ------------ Globally-used strings. -------------------------------- */

extern char *MagicVersion;
extern char AbortMessage[];

/* ---------------- Start of Machine Configuration Section ----------------- */

/* The great thing about standards is that there are so many to choose from! */
#ifdef	m68k
#define	mc68000		
#endif

/* Both Sun3 and SPARC machines turn on LITTLE_ENDIAN!!!  Buy a DECstation, you
 * bozos.
 */
#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif
#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

    /* ------- Configuration:  Selection of Byte Ordering ------- */

/*	Big Endian:
 *		MSB....................LSB
 *		byte0  byte1  byte2  byte3
 *
 *	Little Endian:
 *		MSB....................LSB
 *		byte3  byte2  byte1  byte0
 *
 *	In big-endian, a pointer to a word points to the byte that
 *	contains the most-significant-bit of the word.  In little-endian, 
 *	it points to the byte containing the least-significant-bit.
 *
 */

#ifdef	vax
#define	LITTLE_ENDIAN	/* The good 'ol VAX. */
#endif

#ifdef	MIPSEL
#define	LITTLE_ENDIAN	/* MIPS processors in little-endian mode. */
#endif

#ifdef	wrltitan
#define	LITTLE_ENDIAN 	/* A DEC-WRL titan research machine (only 20 exist). */
			/* NOT intended for the Ardent titan machine. */
#endif

#ifdef	MIPSEB
#define	BIG_ENDIAN	/* MIPS processors in big-endian mode. */
#endif

#ifdef	mc68000
#define	BIG_ENDIAN	/* All 68xxx machines, such as Sun2's and Sun3's. */
#endif

#ifdef	macII
#define	BIG_ENDIAN	/* Apple MacII (also a 68000, but being safe here.) */
#endif

#ifdef	sparc
#define	BIG_ENDIAN	/* All SPARC-based machines. */
#endif

#ifdef	ibm032
#define	BIG_ENDIAN 	/* IBM PC-RT and related machines. */
#endif

#ifdef	hp9000s300
#define	BIG_ENDIAN 	/* HP 9000 machine.  */
#endif

#ifdef	hp9000s800
#define	BIG_ENDIAN 	/* HP 9000 machine.  */
#endif

#ifdef	hp9000s820
#define	BIG_ENDIAN 	/* HP 9000 machine.  */
#endif

/* Well, how'd we do? */

#if	!defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN)
    error1 = You_need_to_define_LITTLE_ENDIAN_or_BIG_ENDIAN_for_your_machine.
#endif
#if	defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
    error1 = You_should_not_define_both_LITTLE_ENDIAN_and_BIG_ENDIAN.
#endif

    /* ------- Configuration:  Handle Missing Routines/Definitions ------- */

/* Many machines don't have the moncontrol() procedure.  (see main.c) */
#ifdef	wrltitan
# define NEED_MONCNTL
#endif
#ifdef	SYSV
# define NEED_MONCNTL
#endif
#ifdef	sun
# define NEED_MONCNTL
#endif  sun

/* System V is missing some BSDisms. */
#ifdef SYSV
# ifndef index
#  define index(x,y)		strchr((x),(int)(y))
# endif
# ifndef bcopy
#  define bcopy(a, b, c)	memcpy(b, a, c)
# endif
# ifndef bzero
#  define bzero(a, b)		memset(a, 0, b)
# endif
# ifndef bcmp
#  define bcmp(a, b, c)		memcmp(b, a, c)
# endif
# ifndef rindex
#  ifdef hpux
#    define rindex()  strrchr()
#  else
#    define rindex(x,y)  strrchr((x),(int)(y))
#  endif
# endif
#endif

/* Some machines need vfprintf().  (A temporary MIPS bug?) (see txOutput.c) */
#if 	(defined(MIPSEB) && defined(SYSTYPE_BSD43)) || ibm032
# define	NEED_VFPRINTF
#endif

/* Some machines don't have the varargs.h package.  If your machine is one
 * of these, try turning this flag on.
 */
#ifdef	funny_machine_no_varargs
# define	NO_VARARGS
#endif

/* Some machines expect signal handlers to return an "int".  But most machines
 * expect them to return a "void".  If your machine expects an "int", put in
 * an "ifdef" below.
 */

#if 	(defined(MIPSEB) && defined(SYSTYPE_BSD43)) || ibm032
# define	SIG_RETURNS_INT
#endif

/* We have this really fancy abort procedure in misc/niceabort.c.  However,
 * these days only vax's appear to have all the things neccessary to make it
 * work (i.e. /usr/ucb/gcore).
 */

#ifdef	vax
# define	FANCY_ABORT
#endif

/* 
 * Sprintf is a "char *" under BSD, and an "int" under System V. 
 */

#ifdef	SYSV
    extern int sprintf();
#else
    extern char* sprintf();
#endif

/* ------------------ End of Machine Configuration Section ----------------- */

