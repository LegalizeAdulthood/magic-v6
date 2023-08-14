SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 11:42:51 mayo Exp $
#

MODULE=	garouter
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	gaChannel.c gaMain.c gaMaze.c gaSimple.c gaStem.c gaTest.c
OBJS=	gaChannel.o gaMain.o gaMaze.o gaSimple.o gaStem.o gaTest.o
HDRS=   garouter.h
POBJS=	gaChannel.po gaMain.po gaMaze.po gaSimple.po gaStem.po gaTest.po
ALLSRCS= ${SRCS} ${HDRS} Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= 
#LINTLIBS= ${LIB}/lint/database.ln \
	${LIB}/lint/gcr.ln ${LIB}/lint/grouter.ln \
	${LIB}/lint/signals.ln ${LIB}/lint/textio.ln \
	${LIB}/lint/tiles.ln ${LIB}/lint/utils.ln
.SUFFIXES: .po

${MODULE}.o: ${OBJS}
	rm -f ${MODULE}.o
	ld -r ${OBJS} -o ${MODULE}.o

${MODULE}.po:	${POBJS}
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

installhdrs: ${H}/${MODULE}.h ${H}/gaDebug.h

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

${H}/gaDebug.h: gaDebug.h
	rm -f ${H}/gaDebug.h 
	cp gaDebug.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:42:45 PDT 1990.
 
# include file dependencies for "gaChannel.c"
gaChannel.o:	gaChannel.c
gaChannel.o:	${H}/magic.h
gaChannel.o:	${H}/geometry.h
gaChannel.o:	${H}/hash.h
gaChannel.o:	${H}/doubleint.h
gaChannel.o:	${H}/heap.h
gaChannel.o:	${H}/tile.h
gaChannel.o:	${H}/database.h
gaChannel.o:	${H}/windows.h
gaChannel.o:	${H}/main.h
gaChannel.o:	${H}/dbwind.h
gaChannel.o:	${H}/signals.h
gaChannel.o:	${H}/netmenu.h
gaChannel.o:	${H}/gcr.h
gaChannel.o:	${H}/router.h
gaChannel.o:	${H}/grouter.h
gaChannel.o:	garouter.h
gaChannel.o:	${H}/netlist.h
gaChannel.o:	${H}/textio.h
gaChannel.o:	${H}/styles.h
gaChannel.o:	${H}/debug.h
 
# include file dependencies for "gaMain.c"
gaMain.o:	gaMain.c
gaMain.o:	${H}/magic.h
gaMain.o:	${H}/geometry.h
gaMain.o:	${H}/hash.h
gaMain.o:	${H}/doubleint.h
gaMain.o:	${H}/heap.h
gaMain.o:	${H}/tile.h
gaMain.o:	${H}/database.h
gaMain.o:	${H}/windows.h
gaMain.o:	${H}/main.h
gaMain.o:	${H}/dbwind.h
gaMain.o:	${H}/signals.h
gaMain.o:	${H}/netmenu.h
gaMain.o:	${H}/gcr.h
gaMain.o:	${H}/router.h
gaMain.o:	${H}/grouter.h
gaMain.o:	garouter.h
gaMain.o:	${H}/netlist.h
gaMain.o:	${H}/textio.h
gaMain.o:	${H}/debug.h
gaMain.o:	${H}/drc.h
 
# include file dependencies for "gaMaze.c"
gaMaze.o:	gaMaze.c
gaMaze.o:	${H}/magic.h
gaMaze.o:	${H}/geometry.h
gaMaze.o:	${H}/hash.h
gaMaze.o:	${H}/doubleint.h
gaMaze.o:	${H}/heap.h
gaMaze.o:	${H}/tile.h
gaMaze.o:	${H}/database.h
gaMaze.o:	${H}/windows.h
gaMaze.o:	${H}/main.h
gaMaze.o:	${H}/dbwind.h
gaMaze.o:	${H}/signals.h
gaMaze.o:	${H}/netmenu.h
gaMaze.o:	${H}/gcr.h
gaMaze.o:	${H}/router.h
gaMaze.o:	${H}/grouter.h
gaMaze.o:	garouter.h
gaMaze.o:	gaInternal.h
gaMaze.o:	${H}/netlist.h
gaMaze.o:	${H}/textio.h
gaMaze.o:	${H}/styles.h
gaMaze.o:	${H}/malloc.h
gaMaze.o:	${H}/drc.h
gaMaze.o:	${H}/debug.h
gaMaze.o:	${H}/list.h
gaMaze.o:	${H}/../mzrouter/mzrouter.h
 
# include file dependencies for "gaSimple.c"
gaSimple.o:	gaSimple.c
gaSimple.o:	${H}/magic.h
gaSimple.o:	${H}/geometry.h
gaSimple.o:	${H}/geofast.h
gaSimple.o:	${H}/hash.h
gaSimple.o:	${H}/doubleint.h
gaSimple.o:	${H}/heap.h
gaSimple.o:	${H}/tile.h
gaSimple.o:	${H}/database.h
gaSimple.o:	${H}/windows.h
gaSimple.o:	${H}/main.h
gaSimple.o:	${H}/dbwind.h
gaSimple.o:	${H}/signals.h
gaSimple.o:	${H}/netmenu.h
gaSimple.o:	${H}/gcr.h
gaSimple.o:	${H}/router.h
gaSimple.o:	${H}/grouter.h
gaSimple.o:	garouter.h
gaSimple.o:	${H}/netlist.h
gaSimple.o:	${H}/textio.h
gaSimple.o:	${H}/styles.h
gaSimple.o:	${H}/malloc.h
gaSimple.o:	${H}/drc.h
gaSimple.o:	${H}/debug.h
gaSimple.o:	${H}/extract.h
gaSimple.o:	${H}/extractInt.h
 
# include file dependencies for "gaStem.c"
gaStem.o:	gaStem.c
gaStem.o:	${H}/magic.h
gaStem.o:	${H}/geometry.h
gaStem.o:	${H}/hash.h
gaStem.o:	${H}/doubleint.h
gaStem.o:	${H}/heap.h
gaStem.o:	${H}/tile.h
gaStem.o:	${H}/database.h
gaStem.o:	${H}/windows.h
gaStem.o:	${H}/main.h
gaStem.o:	${H}/dbwind.h
gaStem.o:	${H}/signals.h
gaStem.o:	${H}/netmenu.h
gaStem.o:	${H}/gcr.h
gaStem.o:	${H}/router.h
gaStem.o:	${H}/grouter.h
gaStem.o:	garouter.h
gaStem.o:	gaInternal.h
gaStem.o:	${H}/netlist.h
gaStem.o:	${H}/textio.h
gaStem.o:	${H}/styles.h
gaStem.o:	${H}/malloc.h
gaStem.o:	${H}/drc.h
gaStem.o:	${H}/debug.h
 
# include file dependencies for "gaTest.c"
gaTest.o:	gaTest.c
gaTest.o:	${H}/magic.h
gaTest.o:	${H}/geometry.h
gaTest.o:	${H}/hash.h
gaTest.o:	${H}/doubleint.h
gaTest.o:	${H}/heap.h
gaTest.o:	${H}/tile.h
gaTest.o:	${H}/database.h
gaTest.o:	${H}/signals.h
gaTest.o:	${H}/textio.h
gaTest.o:	${H}/debug.h
gaTest.o:	${H}/gcr.h
gaTest.o:	${H}/router.h
gaTest.o:	${H}/grouter.h
gaTest.o:	${H}/graphics.h
gaTest.o:	garouter.h
gaTest.o:	${H}/windows.h
gaTest.o:	${H}/dbwind.h
gaTest.o:	${H}/txcommands.h
gaTest.o:	${H}/main.h
gaTest.o:	${H}/utils.h
gaTest.o:	${H}/commands.h
gaTest.o:	${H}/styles.h
 
