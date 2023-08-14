SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 13:13:27 mayo Exp $
#

MODULE=	wiring
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	wireOps.c wireTech.c wireUndo.c
OBJS=	wireOps.o wireTech.o wireUndo.o
HDRS=   wiring.h wireInt.h
POBJS=	wireOps.po wireTech.po wireUndo.po
ALLSRCS= ${SRCS} ${HDRS} Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln ${LIB}/lint/dbwind.ln ${LIB}/lint/drc.ln\
	${LIB}/lint/graphics.ln ${LIB}/lint/signals.ln \
	${LIB}/lint/tech.ln ${LIB}/lint/textio.ln ${LIB}/lint/tiles.ln \
	${LIB}/lint/utils.ln $(LIB)/lint/undo.ln
.SUFFIXES: .po

${MODULE}.o: ${OBJS}
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

profile: installhdrs ${LIB}/${MODULE}.po ${LIB}/lint/${MODULE}.ln

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln

installhdrs: ${H}/${MODULE}.h ${H}/wireInt.h

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

${H}/wireInt.h: wireInt.h
	rm -f ${H}/wireInt.h 
	cp wireInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:13:22 PDT 1990.
 
# include file dependencies for "wireOps.c"
wireOps.o:	wireOps.c
wireOps.o:	${H}/magic.h
wireOps.o:	${H}/geometry.h
wireOps.o:	${H}/tile.h
wireOps.o:	${H}/hash.h
wireOps.o:	${H}/database.h
wireOps.o:	${H}/select.h
wireOps.o:	${H}/textio.h
wireOps.o:	${H}/windows.h
wireOps.o:	${H}/dbwind.h
wireOps.o:	${H}/drc.h
wireOps.o:	${H}/main.h
wireOps.o:	wiring.h
wireOps.o:	wireInt.h
wireOps.o:	${H}/txcommands.h
wireOps.o:	${H}/styles.h
 
# include file dependencies for "wireTech.c"
wireTech.o:	wireTech.c
wireTech.o:	${H}/magic.h
wireTech.o:	${H}/geometry.h
wireTech.o:	${H}/tile.h
wireTech.o:	${H}/hash.h
wireTech.o:	${H}/database.h
wireTech.o:	${H}/tech.h
wireTech.o:	wiring.h
wireTech.o:	${H}/malloc.h
 
# include file dependencies for "wireUndo.c"
wireUndo.o:	wireUndo.c
wireUndo.o:	${H}/magic.h
wireUndo.o:	${H}/geometry.h
wireUndo.o:	${H}/tile.h
wireUndo.o:	${H}/hash.h
wireUndo.o:	${H}/database.h
wireUndo.o:	${H}/tech.h
wireUndo.o:	wiring.h
wireUndo.o:	wireInt.h
wireUndo.o:	${H}/textio.h
wireUndo.o:	${H}/undo.h
 
