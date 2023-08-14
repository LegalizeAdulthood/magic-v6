SHELL=/bin/sh
#
# sccsid @(#)Makefile	4.1 MAGIC (Berkeley) 7/4/85
#

MODULE=grouter
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
LL=	${MAGICDIR}/lib/lint
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=${IFLAGS} -u
LINTLIBS= ${LL}/utils.ln ${LL}/tiles.ln ${LL}/database.ln ${LL}/gcr.ln \
	  ${LL}/netmenu.ln ${LL}/textio.ln

BIN=	${MAGICDIR}/bin

HDRS=   grouter.h grouteDebug.h
SRCS=	grouteChan.c grouteCrss.c grouteDens.c grouteMain.c \
	grouteMult.c grouteMaze.c groutePath.c groutePen.c grouteTest.c
OBJS=	grouteChan.o grouteCrss.o grouteDens.o grouteMain.o \
	grouteMult.o grouteMaze.o groutePath.o groutePen.o grouteTest.o
POBJS=	grouteChan.po grouteCrss.po grouteDens.po grouteMain.po \
	grouteMult.po grouteMaze.po groutePath.po groutePen.po grouteTest.po

ALLSRCS= ${SRCS} ${HDRS} Makefile

grouter.o:	${OBJS}
	rm -f grouter.o
	ld -r ${OBJS} -o grouter.o

grouter.po:	${POBJS}
	rm -f grouter.po
	ld -r ${POBJS} -o grouter.po

.SUFFIXES: .po

.c.po :
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

installhdrs: ${H}/${MODULE}.h ${H}/grouteDebug.h


${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -r -x ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/grouter.po: grouter.po
	rm -f ${LIB}/grouter.po
	cp grouter.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${H}/grouteDebug.h: grouteDebug.h
	rm -f ${H}/grouteDebug.h 
	cp grouteDebug.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}
 

# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:08:12 PDT 1990.
 
# include file dependencies for "grouteChan.c"
grouteChan.o:	grouteChan.c
grouteChan.o:	${H}/magic.h
grouteChan.o:	${H}/geometry.h
grouteChan.o:	${H}/styles.h
grouteChan.o:	${H}/hash.h
grouteChan.o:	${H}/doubleint.h
grouteChan.o:	${H}/heap.h
grouteChan.o:	${H}/utils.h
grouteChan.o:	${H}/tile.h
grouteChan.o:	${H}/database.h
grouteChan.o:	${H}/debug.h
grouteChan.o:	${H}/gcr.h
grouteChan.o:	${H}/windows.h
grouteChan.o:	${H}/main.h
grouteChan.o:	${H}/dbwind.h
grouteChan.o:	${H}/signals.h
grouteChan.o:	${H}/router.h
grouteChan.o:	grouter.h
grouteChan.o:	${H}/netlist.h
grouteChan.o:	${H}/styles.h
grouteChan.o:	${H}/textio.h
grouteChan.o:	${H}/malloc.h
 
# include file dependencies for "grouteCrss.c"
grouteCrss.o:	grouteCrss.c
grouteCrss.o:	${H}/magic.h
grouteCrss.o:	${H}/geometry.h
grouteCrss.o:	${H}/geofast.h
grouteCrss.o:	${H}/hash.h
grouteCrss.o:	${H}/doubleint.h
grouteCrss.o:	${H}/heap.h
grouteCrss.o:	${H}/malloc.h
grouteCrss.o:	${H}/debug.h
grouteCrss.o:	${H}/tile.h
grouteCrss.o:	${H}/database.h
grouteCrss.o:	${H}/gcr.h
grouteCrss.o:	${H}/windows.h
grouteCrss.o:	${H}/main.h
grouteCrss.o:	${H}/dbwind.h
grouteCrss.o:	${H}/signals.h
grouteCrss.o:	${H}/router.h
grouteCrss.o:	grouter.h
grouteCrss.o:	${H}/netlist.h
grouteCrss.o:	${H}/textio.h
grouteCrss.o:	${H}/styles.h
 
# include file dependencies for "grouteDens.c"
grouteDens.o:	grouteDens.c
grouteDens.o:	${H}/magic.h
grouteDens.o:	${H}/geometry.h
grouteDens.o:	${H}/hash.h
grouteDens.o:	${H}/doubleint.h
grouteDens.o:	${H}/heap.h
grouteDens.o:	${H}/tile.h
grouteDens.o:	${H}/database.h
grouteDens.o:	${H}/netlist.h
grouteDens.o:	${H}/gcr.h
grouteDens.o:	${H}/router.h
grouteDens.o:	grouter.h
grouteDens.o:	${H}/signals.h
grouteDens.o:	${H}/textio.h
grouteDens.o:	${H}/malloc.h
grouteDens.o:	${H}/styles.h
grouteDens.o:	${H}/debug.h
 
# include file dependencies for "grouteMain.c"
grouteMain.o:	grouteMain.c
grouteMain.o:	${H}/magic.h
grouteMain.o:	${H}/geometry.h
grouteMain.o:	${H}/styles.h
grouteMain.o:	${H}/hash.h
grouteMain.o:	${H}/doubleint.h
grouteMain.o:	${H}/heap.h
grouteMain.o:	${H}/utils.h
grouteMain.o:	${H}/tile.h
grouteMain.o:	${H}/database.h
grouteMain.o:	${H}/debug.h
grouteMain.o:	${H}/gcr.h
grouteMain.o:	${H}/windows.h
grouteMain.o:	${H}/main.h
grouteMain.o:	${H}/dbwind.h
grouteMain.o:	${H}/signals.h
grouteMain.o:	${H}/router.h
grouteMain.o:	grouter.h
grouteMain.o:	${H}/netlist.h
grouteMain.o:	${H}/styles.h
grouteMain.o:	${H}/textio.h
grouteMain.o:	${H}/malloc.h
 
# include file dependencies for "grouteMaze.c"
grouteMaze.o:	grouteMaze.c
grouteMaze.o:	${H}/magic.h
grouteMaze.o:	${H}/geometry.h
grouteMaze.o:	${H}/geofast.h
grouteMaze.o:	${H}/hash.h
grouteMaze.o:	${H}/doubleint.h
grouteMaze.o:	${H}/heap.h
grouteMaze.o:	${H}/malloc.h
grouteMaze.o:	${H}/debug.h
grouteMaze.o:	${H}/tile.h
grouteMaze.o:	${H}/database.h
grouteMaze.o:	${H}/gcr.h
grouteMaze.o:	${H}/windows.h
grouteMaze.o:	${H}/main.h
grouteMaze.o:	${H}/dbwind.h
grouteMaze.o:	${H}/signals.h
grouteMaze.o:	${H}/router.h
grouteMaze.o:	grouter.h
grouteMaze.o:	${H}/netlist.h
grouteMaze.o:	${H}/textio.h
grouteMaze.o:	${H}/styles.h
 
# include file dependencies for "grouteMult.c"
grouteMult.o:	grouteMult.c
grouteMult.o:	${H}/magic.h
grouteMult.o:	${H}/geometry.h
grouteMult.o:	${H}/hash.h
grouteMult.o:	${H}/doubleint.h
grouteMult.o:	${H}/heap.h
grouteMult.o:	${H}/tile.h
grouteMult.o:	${H}/database.h
grouteMult.o:	${H}/router.h
grouteMult.o:	${H}/gcr.h
grouteMult.o:	grouter.h
grouteMult.o:	${H}/netlist.h
grouteMult.o:	${H}/signals.h
grouteMult.o:	${H}/textio.h
grouteMult.o:	${H}/malloc.h
grouteMult.o:	${H}/styles.h
grouteMult.o:	${H}/windows.h
grouteMult.o:	${H}/dbwind.h
 
# include file dependencies for "grouteName.c"
grouteName.o:	grouteName.c
grouteName.o:	${H}/magic.h
grouteName.o:	${H}/geometry.h
grouteName.o:	${H}/debug.h
grouteName.o:	${H}/hash.h
grouteName.o:	${H}/tile.h
grouteName.o:	${H}/database.h
grouteName.o:	${H}/gcr.h
grouteName.o:	${H}/router.h
grouteName.o:	${H}/heap.h
grouteName.o:	grouter.h
grouteName.o:	${H}/netlist.h
grouteName.o:	${H}/textio.h
grouteName.o:	${H}/malloc.h
grouteName.o:	${H}/utils.h
 
# include file dependencies for "grouteNet.c"
grouteNet.o:	grouteNet.c
grouteNet.o:	${H}/magic.h
grouteNet.o:	${H}/geometry.h
grouteNet.o:	${H}/geofast.h
grouteNet.o:	${H}/hash.h
grouteNet.o:	${H}/heap.h
grouteNet.o:	${H}/malloc.h
grouteNet.o:	${H}/debug.h
grouteNet.o:	${H}/tile.h
grouteNet.o:	${H}/database.h
grouteNet.o:	${H}/gcr.h
grouteNet.o:	${H}/windows.h
grouteNet.o:	${H}/main.h
grouteNet.o:	${H}/dbwind.h
grouteNet.o:	${H}/signals.h
grouteNet.o:	${H}/router.h
grouteNet.o:	grouter.h
grouteNet.o:	${H}/netlist.h
grouteNet.o:	${H}/textio.h
grouteNet.o:	${H}/styles.h
 
# include file dependencies for "groutePath.c"
groutePath.o:	groutePath.c
groutePath.o:	${H}/magic.h
groutePath.o:	${H}/geometry.h
groutePath.o:	${H}/hash.h
groutePath.o:	${H}/doubleint.h
groutePath.o:	${H}/heap.h
groutePath.o:	${H}/tile.h
groutePath.o:	${H}/debug.h
groutePath.o:	${H}/database.h
groutePath.o:	${H}/gcr.h
groutePath.o:	${H}/windows.h
groutePath.o:	${H}/main.h
groutePath.o:	${H}/dbwind.h
groutePath.o:	${H}/signals.h
groutePath.o:	${H}/router.h
groutePath.o:	grouter.h
groutePath.o:	${H}/textio.h
groutePath.o:	${H}/malloc.h
 
# include file dependencies for "groutePen.c"
groutePen.o:	groutePen.c
groutePen.o:	${H}/magic.h
groutePen.o:	${H}/geometry.h
groutePen.o:	${H}/hash.h
groutePen.o:	${H}/doubleint.h
groutePen.o:	${H}/heap.h
groutePen.o:	${H}/tile.h
groutePen.o:	${H}/database.h
groutePen.o:	${H}/netlist.h
groutePen.o:	${H}/gcr.h
groutePen.o:	${H}/router.h
groutePen.o:	grouter.h
groutePen.o:	${H}/signals.h
groutePen.o:	${H}/textio.h
groutePen.o:	${H}/malloc.h
groutePen.o:	${H}/styles.h
groutePen.o:	${H}/list.h
 
# include file dependencies for "groutePin.c"
groutePin.o:	groutePin.c
groutePin.o:	${H}/magic.h
groutePin.o:	${H}/geometry.h
groutePin.o:	${H}/styles.h
groutePin.o:	${H}/hash.h
groutePin.o:	${H}/heap.h
groutePin.o:	${H}/debug.h
groutePin.o:	${H}/tile.h
groutePin.o:	${H}/database.h
groutePin.o:	${H}/gcr.h
groutePin.o:	${H}/windows.h
groutePin.o:	${H}/main.h
groutePin.o:	${H}/dbwind.h
groutePin.o:	${H}/signals.h
groutePin.o:	${H}/router.h
groutePin.o:	grouter.h
groutePin.o:	${H}/textio.h
 
# include file dependencies for "grouteTest.c"
grouteTest.o:	grouteTest.c
grouteTest.o:	${H}/magic.h
grouteTest.o:	${H}/geometry.h
grouteTest.o:	${H}/hash.h
grouteTest.o:	${H}/doubleint.h
grouteTest.o:	${H}/heap.h
grouteTest.o:	${H}/tile.h
grouteTest.o:	${H}/database.h
grouteTest.o:	${H}/signals.h
grouteTest.o:	${H}/textio.h
grouteTest.o:	${H}/debug.h
grouteTest.o:	${H}/gcr.h
grouteTest.o:	${H}/router.h
grouteTest.o:	grouter.h
grouteTest.o:	${H}/graphics.h
grouteTest.o:	${H}/windows.h
grouteTest.o:	${H}/dbwind.h
grouteTest.o:	${H}/txcommands.h
grouteTest.o:	${H}/main.h
grouteTest.o:	${H}/utils.h
grouteTest.o:	${H}/commands.h
grouteTest.o:	${H}/malloc.h
grouteTest.o:	${H}/styles.h
 
# include file dependencies for "grouteTile.c"
grouteTile.o:	grouteTile.c
grouteTile.o:	${H}/magic.h
grouteTile.o:	${H}/geometry.h
grouteTile.o:	${H}/hash.h
grouteTile.o:	${H}/heap.h
grouteTile.o:	${H}/tile.h
grouteTile.o:	${H}/database.h
grouteTile.o:	${H}/debug.h
grouteTile.o:	${H}/gcr.h
grouteTile.o:	${H}/windows.h
grouteTile.o:	${H}/graphics.h
grouteTile.o:	${H}/main.h
grouteTile.o:	${H}/dbwind.h
grouteTile.o:	${H}/signals.h
grouteTile.o:	${H}/router.h
grouteTile.o:	grouter.h
grouteTile.o:	${H}/styles.h
grouteTile.o:	${H}/textio.h
 
