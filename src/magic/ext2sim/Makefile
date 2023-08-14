SHELL=/bin/sh
#
# rcsid "$Header: Makefile,v 6.0 90/08/29 13:13:52 mayo Exp $"
#

CADDIR  = /udir/mayo/cadhome
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	ext2sim.c
OBJS=	ext2sim.o
POBJS=	ext2sim.po
LIBS=	${LIB}/libextflat.a ${LIB}/libmagicutils.a
PLIBS=	${LIB}/libextflat_p.a ${LIB}/libmagicutils_p.a

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/extflat.ln ${LIB}/lint/utils.ln

ext2sim: ${OBJS} ${LIBS}
	rm -f ext2sim
	${CC} ${CFLAGS} ${OBJS} ${LIBS} -o ext2sim -lm

pext2sim: ${POBJS}
	rm -f pext2sim
	${CC} ${PCFLAGS} ${POBJS} ${PLIBS} -o pext2sim -lm

install: ${CADDIR}/bin/ext2sim

${CADDIR}/bin/ext2sim: ext2sim
	rm -f ${CADDIR}/bin/ext2sim
	cp ext2sim ${CADDIR}/bin/ext2sim

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

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
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:13:49 PDT 1990.
 
# include file dependencies for "ext2sim.c"
ext2sim.o:	ext2sim.c
ext2sim.o:	${H}/magic.h
ext2sim.o:	${H}/paths.h
ext2sim.o:	${H}/geometry.h
ext2sim.o:	${H}/hash.h
ext2sim.o:	${H}/utils.h
ext2sim.o:	${H}/pathvisit.h
ext2sim.o:	${H}/extflat.h
ext2sim.o:	${H}/runstats.h
 
# include file dependencies for "finds.c"
finds.o:	finds.c
finds.o:	${H}/magic.h
finds.o:	${H}/hash.h
 
