SHELL=/bin/sh
#
# sccsid @(#)Makefile	4.5 MAGIC (Berkeley) 11/10/85
#

MODULE=	plot
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	plotGremln.c plotMain.c plotRutils.c plotVers.c plotPixels.c
OBJS=	plotGremln.o plotMain.o plotRutils.o plotVers.o plotPixels.o
HDRS=   plot.h plotInt.h
POBJS=	plotGremln.po plotMain.po plotRutils.po plotVers.po plotPixels.po
ALLSRCS= ${SRCS} ${HDRS} Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/commands.ln ${LIB}/lint/database.ln \
	${LIB}/lint/signals.ln \
	${LIB}/lint/tech.ln ${LIB}/lint/textio.ln ${LIB}/lint/tiles.ln \
	${LIB}/lint/utils.ln $(LIB)/lint/undo.ln
.SUFFIXES: .po

${MODULE}.o: ${OBJS}
	rm -f ${MODULE}.o
	ld -r ${OBJS} -o ${MODULE}.o

${MODULE}.po: ${POBJS}
	rm -f ${MODULE}.po
	ld -r ${POBJS} -o ${MODULE}.po

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

installhdrs: ${H}/${MODULE}.h ${H}/plotInt.h

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

${H}/plotInt.h: plotInt.h
	rm -f ${H}/plotInt.h 
	cp plotInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:10:16 PDT 1990.
 
# include file dependencies for "plotGremln.c"
plotGremln.o:	plotGremln.c
plotGremln.o:	${H}/magic.h
plotGremln.o:	${H}/geometry.h
plotGremln.o:	${H}/tile.h
plotGremln.o:	${H}/hash.h
plotGremln.o:	${H}/database.h
plotGremln.o:	${H}/tech.h
plotGremln.o:	${H}/malloc.h
plotGremln.o:	${H}/utils.h
plotGremln.o:	${H}/windows.h
plotGremln.o:	${H}/commands.h
plotGremln.o:	${H}/router.h
plotGremln.o:	${H}/textio.h
 
# include file dependencies for "plotMain.c"
plotMain.o:	plotMain.c
plotMain.o:	${H}/magic.h
plotMain.o:	${H}/geometry.h
plotMain.o:	${H}/tile.h
plotMain.o:	${H}/hash.h
plotMain.o:	${H}/database.h
plotMain.o:	${H}/tech.h
plotMain.o:	${H}/malloc.h
plotMain.o:	plotInt.h
plotMain.o:	${H}/textio.h
plotMain.o:	${H}/utils.h
 
# include file dependencies for "plotPixels.c"
plotPixels.o:	plotPixels.c
plotPixels.o:	${H}/magic.h
plotPixels.o:	${H}/geometry.h
plotPixels.o:	${H}/geofast.h
plotPixels.o:	${H}/tile.h
plotPixels.o:	${H}/hash.h
plotPixels.o:	${H}/database.h
plotPixels.o:	${H}/dbwtech.h
plotPixels.o:	${H}/malloc.h
plotPixels.o:	plotInt.h
plotPixels.o:	${H}/windows.h
plotPixels.o:	${H}/commands.h
plotPixels.o:	${H}/textio.h
plotPixels.o:	${H}/utils.h
plotPixels.o:	${H}/tech.h
plotPixels.o:	${H}/signals.h
plotPixels.o:	${H}/router.h
plotPixels.o:	${H}/styles.h
plotPixels.o:	${H}/graphicsInt.h	
 
# include file dependencies for "plotRutils.c"
plotRutils.o:	plotRutils.c
plotRutils.o:	${H}/magic.h
plotRutils.o:	${H}/geometry.h
plotRutils.o:	${H}/tile.h
plotRutils.o:	${H}/hash.h
plotRutils.o:	${H}/database.h
plotRutils.o:	${H}/malloc.h
plotRutils.o:	plotInt.h
plotRutils.o:	${H}/textio.h
plotRutils.o:	${H}/utils.h
 
# include file dependencies for "plotVers.c"
plotVers.o:	plotVers.c
plotVers.o:	${H}/magic.h
plotVers.o:	${H}/geometry.h
plotVers.o:	${H}/geofast.h
plotVers.o:	${H}/tile.h
plotVers.o:	${H}/hash.h
plotVers.o:	${H}/database.h
plotVers.o:	${H}/malloc.h
plotVers.o:	plotInt.h
plotVers.o:	${H}/windows.h
plotVers.o:	${H}/commands.h
plotVers.o:	${H}/textio.h
plotVers.o:	${H}/utils.h
plotVers.o:	${H}/tech.h
plotVers.o:	${H}/signals.h
plotVers.o:	${H}/router.h
 
# include file dependencies for "vdmpc.c"
vdmpc.o:	vdmpc.c
 
