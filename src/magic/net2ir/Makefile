SHELL=/bin/sh
# rcsid = "$Header: Makefile,v 6.0 90/08/29 13:14:54 mayo Exp $"
#
# Makefile for net2ir
#

CADDIR  =`../:findcad`
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	net2ir.c
OBJS=	net2ir.o
LIBS=	${LIB}/libmagicutils.a

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/utils.ln

net2ir: ${OBJS} ${LIBS}
	rm -f net2ir
	${CC} ${CFLAGS} ${OBJS} ${LIBS} -o net2ir

install: net2ir
	rm -f ${CADDIR}/bin/net2ir
	cp net2ir ${CADDIR}/bin/net2ir

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:14:51 PDT 1990.
 
# include file dependencies for "net2ir.c"
net2ir.o:	net2ir.c
net2ir.o:	${H}/magic.h
net2ir.o:	${H}/geometry.h
net2ir.o:	${H}/hash.h
net2ir.o:	${H}/malloc.h
net2ir.o:	${H}/utils.h
 
