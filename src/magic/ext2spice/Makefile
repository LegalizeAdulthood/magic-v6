SHELL=/bin/sh
#
# rcsid "$Header: Makefile,v 6.0 90/08/29 13:14:05 mayo Exp $"
#

CADDIR  = /udir/mayo/cadhome
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	ext2spice.c
OBJS=	ext2spice.o
POBJS=	ext2spice.po
LIBS=	${LIB}/libextflat.a ${LIB}/libmagicutils.a
PLIBS=	${LIB}/libextflat_p.a ${LIB}/libmagicutils_p.a

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/extflat.ln ${LIB}/lint/utils.ln

ext2spice: ${OBJS} ${LIBS}
	rm -f ext2spice
	${CC} ${CFLAGS} ${OBJS} ${LIBS} -o ext2spice -lm

pext2spice: ${POBJS}
	rm -f pext2spice
	${CC} ${PCFLAGS} ${POBJS} ${PLIBS} -o pext2spice -lm

install: ${CADDIR}/bin/ext2spice

${CADDIR}/bin/ext2spice: ext2spice
	rm -f ${CADDIR}/bin/ext2spice
	cp ext2spice ${CADDIR}/bin/ext2spice

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
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:14:02 PDT 1990.
 
# include file dependencies for "ext2spice.c"
ext2spice.o:	ext2spice.c
ext2spice.o:	${H}/magic.h
ext2spice.o:	${H}/paths.h
ext2spice.o:	${H}/geometry.h
ext2spice.o:	${H}/malloc.h
ext2spice.o:	${H}/hash.h
ext2spice.o:	${H}/utils.h
ext2spice.o:	${H}/pathvisit.h
ext2spice.o:	${H}/extflat.h
ext2spice.o:	${H}/runstats.h
 
