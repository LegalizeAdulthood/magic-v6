SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 11:41:46 mayo Exp $
#

MODULE=	debug
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	debugFlags.c hist.c
HDRS=   ${MODULE}.h
OBJS=	debugFlags.o hist.o
POBJS=	debugFlags.po hist.po
ALLSRCS= ${SRCS} ${HDRS} Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/utils.ln
.SUFFIXES: .po

all:	${MODULE}.o

${MODULE}.o:	${OBJS}
	rm -f ${MODULE}.o
	ld -r ${OBJS} -o ${MODULE}.o
	
.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

profile: installhdrs ${LIB}/${MODULE}.po

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln

installhdrs: ${H}/${MODULE}.h


${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -r -x ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/${MODULE}.po: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:41:42 PDT 1990.
 
# include file dependencies for "debugFlags.c"
debugFlags.o:	debugFlags.c
debugFlags.o:	${H}/magic.h
debugFlags.o:	debug.h
debugFlags.o:	${H}/textio.h
debugFlags.o:	${H}/malloc.h
debugFlags.o:	${H}/utils.h
 
# include file dependencies for "hist.c"
hist.o:	hist.c
hist.o:	${H}/magic.h
hist.o:	${H}/geometry.h
hist.o:	${H}/tile.h
hist.o:	${H}/utils.h
hist.o:	${H}/textio.h
hist.o:	debug.h
hist.o:	${H}/malloc.h
 
