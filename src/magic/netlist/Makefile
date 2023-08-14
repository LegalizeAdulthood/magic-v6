SHELL=/bin/sh
#
# rcsid "$Header: Makefile,v 6.0 90/08/29 13:09:39 mayo Exp $"
#

MODULE=	netlist
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin


SRCS=	${MODULE}.c
OBJS=	${MODULE}.o
POBJS=	${MODULE}.po
ALLSRCS= ${SRCS} ${MODULE}.h Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=${IFLAGS} -u
LINTLIBS=

.SUFFIXES: .po

${MODULE}.o: ${H}/magic.h ${MODULE}.h ${MODULE}.c

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

profile: installhdrs ${LIB}/${MODULE}.po ${LIB}/lint/${MODULE}.ln

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln

installhdrs: ${H}/${MODULE}.h

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -x -r ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/${MODULE}.po: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:09:36 PDT 1990.
 
# include file dependencies for "netlist.c"
netlist.o:	netlist.c
netlist.o:	${H}/magic.h
netlist.o:	${H}/geometry.h
netlist.o:	${H}/hash.h
netlist.o:	${H}/doubleint.h
netlist.o:	${H}/heap.h
netlist.o:	${H}/tile.h
netlist.o:	${H}/database.h
netlist.o:	netlist.h
netlist.o:	${H}/signals.h
netlist.o:	${H}/textio.h
netlist.o:	${H}/malloc.h
netlist.o:	${H}/styles.h
 
