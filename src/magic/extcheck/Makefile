SHELL=/bin/sh
#
# rcsid "$Header: Makefile,v 6.0 90/08/29 13:15:07 mayo Exp $"
#

CADDIR  =`../:findcad`
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	extcheck.c
OBJS=	extcheck.o
POBJS=	extcheck.po
LIBS=	${LIB}/libextflat.a ${LIB}/libmagicutils.a
PLIBS=	${LIB}/libextflat_p.a ${LIB}/libmagicutils_p.a

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/extflat.ln ${LIB}/lint/utils.ln

extcheck: ${OBJS} ${LIBS}
	rm -f extcheck
	${CC} ${CFLAGS} ${OBJS} ${LIBS} -o extcheck -lm

pextcheck: ${POBJS}
	rm -f pextcheck
	${CC} ${PCFLAGS} ${POBJS} ${PLIBS} -o pextcheck -lm

install: extcheck
	rm -f ${CADDIR}/bin/extcheck
	cp extcheck ${CADDIR}/bin/extcheck

lint:	${SRCS} ${LINTLIBS}
	rm -f lint
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} > lint

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

.SUFFIXES: .po

.c.po:
	-mv $*.o $*.so
	${CC} ${PCFLAGS} -c $*.c
	rm -f $*.po
	mv $*.o $*.po
	-mv $*.so $*.o

 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:15:05 PDT 1990.
 
# include file dependencies for "extcheck.c"
extcheck.o:	extcheck.c
extcheck.o:	${H}/magic.h
extcheck.o:	${H}/paths.h
extcheck.o:	${H}/geometry.h
extcheck.o:	${H}/hash.h
extcheck.o:	${H}/utils.h
extcheck.o:	${H}/pathvisit.h
extcheck.o:	${H}/extflat.h
extcheck.o:	${H}/runstats.h
 
