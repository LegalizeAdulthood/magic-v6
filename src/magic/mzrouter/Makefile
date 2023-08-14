SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 13:09:27 mayo Exp $
#

MODULE=	mzrouter
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

HDRS=   mzrouter.h mzInternal.h mzDebug.h 

SRCS=	mzBlock.c  mzDebug.c  mzEstimate.c  \
	mzXtndDown.c  mzXtndLeft.c  mzXtndRght.c  mzXtndUp.c \
	mzHint.c  mzInit.c  mzNumLine.c  mzMain.c  mzSearch.c  mzStart.c \
	mzSubrs.c  mzTech.c  mzTestCmd.c  mzWalk.c

OBJS=	mzBlock.o  mzDebug.o  mzEstimate.o  \
	mzXtndDown.o  mzXtndLeft.o  mzXtndRght.o  mzXtndUp.o \
	mzHint.o  mzInit.o  mzNumLine.o  mzMain.o  mzSearch.o  mzStart.o \
	mzSubrs.o  mzTech.o  mzTestCmd.o  mzWalk.o

POBJS=	mzBlock.po mzDebug.po mzEstimate.po  \
	mzXtndDown.po mzXtndLeft.po mzXtndRght.po mzXtndUp.po \
	mzHint.po mzInit.po mzNumLine.po mzMain.po mzSearch.po mzStart.po \
	mzSubrs.po mzTech.po mzTestCmd.po mzWalk.po

ALLSRCS= ${SRCS} ${HDRS} Makefile 

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= \
	${LIB}/lint/database.ln ${LIB}/lint/dbwind.ln ${LIB}/lint/debug.ln \
	${LIB}/lint/main.ln ${LIB}/lint/misc.ln \
	${LIB}/lint/signals.ln ${LIB}/lint/tech.ln ${LIB}/lint/tiles.ln\
	${LIB}/lint/undo.ln ${LIB}/lint/utils.ln \
	${LIB}/lint/windows.ln ${LIB}/lint/debug.ln ${LIB}/lint/undo.ln
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

#	install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln
install: installhdrs ${LIB}/${MODULE}.o

installhdrs: ${H}/${MODULE}.h ${H}/mzInternal.h ${H}/mzDebug.h

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

${H}/mzInternal.h: mzInternal.h
	rm -f ${H}/mzInternal.h 
	cp mzInternal.h ${H}

${H}/mzDebug.h: mzDebug.h
	rm -f ${H}/mzDebug.h 
	cp mzDebug.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:09:19 PDT 1990.
 
# include file dependencies for "mzBlock.c"
mzBlock.o:	mzBlock.c
mzBlock.o:	${H}/magic.h
mzBlock.o:	${H}/geometry.h
mzBlock.o:	${H}/geofast.h
mzBlock.o:	${H}/tile.h
mzBlock.o:	${H}/hash.h
mzBlock.o:	${H}/database.h
mzBlock.o:	${H}/signals.h
mzBlock.o:	${H}/textio.h
mzBlock.o:	${H}/windows.h
mzBlock.o:	${H}/dbwind.h
mzBlock.o:	${H}/malloc.h
mzBlock.o:	${H}/list.h
mzBlock.o:	${H}/debug.h
mzBlock.o:	${H}/textio.h
mzBlock.o:	${H}/doubleint.h
mzBlock.o:	${H}/heap.h
mzBlock.o:	mzrouter.h
mzBlock.o:	mzInternal.h
 
# include file dependencies for "mzDebug.c"
mzDebug.o:	mzDebug.c
mzDebug.o:	${H}/magic.h
mzDebug.o:	${H}/signals.h
mzDebug.o:	${H}/textio.h
mzDebug.o:	${H}/hash.h
mzDebug.o:	${H}/geometry.h
mzDebug.o:	${H}/tile.h
mzDebug.o:	${H}/database.h
mzDebug.o:	${H}/malloc.h
mzDebug.o:	${H}/list.h
mzDebug.o:	${H}/doubleint.h
mzDebug.o:	${H}/heap.h
mzDebug.o:	mzrouter.h
mzDebug.o:	mzInternal.h
 
# include file dependencies for "mzEstimate.c"
mzEstimate.o:	mzEstimate.c
mzEstimate.o:	${H}/magic.h
mzEstimate.o:	${H}/geometry.h
mzEstimate.o:	${H}/geofast.h
mzEstimate.o:	${H}/tile.h
mzEstimate.o:	${H}/hash.h
mzEstimate.o:	${H}/database.h
mzEstimate.o:	${H}/signals.h
mzEstimate.o:	${H}/textio.h
mzEstimate.o:	${H}/windows.h
mzEstimate.o:	${H}/dbwind.h
mzEstimate.o:	${H}/malloc.h
mzEstimate.o:	${H}/list.h
mzEstimate.o:	${H}/debug.h
mzEstimate.o:	${H}/textio.h
mzEstimate.o:	${H}/doubleint.h
mzEstimate.o:	${H}/heap.h
mzEstimate.o:	mzrouter.h
mzEstimate.o:	mzInternal.h
 
# include file dependencies for "mzHint.c"
mzHint.o:	mzHint.c
mzHint.o:	${H}/magic.h
mzHint.o:	${H}/geometry.h
mzHint.o:	${H}/geofast.h
mzHint.o:	${H}/tile.h
mzHint.o:	${H}/hash.h
mzHint.o:	${H}/database.h
mzHint.o:	${H}/signals.h
mzHint.o:	${H}/textio.h
mzHint.o:	${H}/windows.h
mzHint.o:	${H}/dbwind.h
mzHint.o:	${H}/malloc.h
mzHint.o:	${H}/list.h
mzHint.o:	${H}/debug.h
mzHint.o:	${H}/textio.h
mzHint.o:	${H}/doubleint.h
mzHint.o:	${H}/heap.h
mzHint.o:	mzrouter.h
mzHint.o:	mzInternal.h
 
# include file dependencies for "mzInit.c"
mzInit.o:	mzInit.c
mzInit.o:	${H}/magic.h
mzInit.o:	${H}/geometry.h
mzInit.o:	${H}/tile.h
mzInit.o:	${H}/hash.h
mzInit.o:	${H}/database.h
mzInit.o:	${H}/drc.h
mzInit.o:	${H}/select.h
mzInit.o:	${H}/signals.h
mzInit.o:	${H}/textio.h
mzInit.o:	${H}/windows.h
mzInit.o:	${H}/dbwind.h
mzInit.o:	${H}/styles.h
mzInit.o:	${H}/debug.h
mzInit.o:	${H}/undo.h
mzInit.o:	${H}/txcommands.h
mzInit.o:	${H}/malloc.h
mzInit.o:	${H}/list.h
mzInit.o:	${H}/doubleint.h
mzInit.o:	${H}/heap.h
mzInit.o:	mzrouter.h
mzInit.o:	mzInternal.h
 
# include file dependencies for "mzMain.c"
mzMain.o:	mzMain.c
mzMain.o:	${H}/magic.h
mzMain.o:	${H}/geometry.h
mzMain.o:	${H}/tile.h
mzMain.o:	${H}/hash.h
mzMain.o:	${H}/database.h
mzMain.o:	${H}/drc.h
mzMain.o:	${H}/select.h
mzMain.o:	${H}/signals.h
mzMain.o:	${H}/textio.h
mzMain.o:	${H}/windows.h
mzMain.o:	${H}/dbwind.h
mzMain.o:	${H}/styles.h
mzMain.o:	${H}/debug.h
mzMain.o:	${H}/undo.h
mzMain.o:	${H}/txcommands.h
mzMain.o:	${H}/malloc.h
mzMain.o:	${H}/main.h
mzMain.o:	${H}/geofast.h
mzMain.o:	${H}/../utils/list.h
mzMain.o:	${H}/doubleint.h
mzMain.o:	${H}/heap.h
mzMain.o:	${H}/touchingtypes.h
mzMain.o:	mzrouter.h
mzMain.o:	mzInternal.h
 
# include file dependencies for "mzNumLine.c"
mzNumLine.o:	mzNumLine.c
mzNumLine.o:	${H}/magic.h
mzNumLine.o:	${H}/geometry.h
mzNumLine.o:	${H}/geofast.h
mzNumLine.o:	${H}/tile.h
mzNumLine.o:	${H}/hash.h
mzNumLine.o:	${H}/database.h
mzNumLine.o:	${H}/signals.h
mzNumLine.o:	${H}/textio.h
mzNumLine.o:	${H}/windows.h
mzNumLine.o:	${H}/dbwind.h
mzNumLine.o:	${H}/malloc.h
mzNumLine.o:	${H}/list.h
mzNumLine.o:	${H}/debug.h
mzNumLine.o:	${H}/textio.h
mzNumLine.o:	${H}/doubleint.h
mzNumLine.o:	${H}/heap.h
mzNumLine.o:	mzrouter.h
mzNumLine.o:	mzInternal.h
 
# include file dependencies for "mzSearch.c"
mzSearch.o:	mzSearch.c
mzSearch.o:	${H}/magic.h
mzSearch.o:	${H}/geometry.h
mzSearch.o:	${H}/geofast.h
mzSearch.o:	${H}/doubleint.h
mzSearch.o:	${H}/hash.h
mzSearch.o:	${H}/heap.h
mzSearch.o:	${H}/../tiles/tile.h
mzSearch.o:	${H}/database.h
mzSearch.o:	${H}/signals.h
mzSearch.o:	${H}/textio.h
mzSearch.o:	${H}/malloc.h
mzSearch.o:	${H}/list.h
mzSearch.o:	${H}/debug.h
mzSearch.o:	${H}/styles.h
mzSearch.o:	${H}/windows.h
mzSearch.o:	${H}/dbwind.h
mzSearch.o:	mzrouter.h
mzSearch.o:	mzInternal.h
 
# include file dependencies for "mzStart.c"
mzStart.o:	mzStart.c
mzStart.o:	${H}/magic.h
mzStart.o:	${H}/geometry.h
mzStart.o:	${H}/geofast.h
mzStart.o:	${H}/../tiles/tile.h
mzStart.o:	${H}/doubleint.h
mzStart.o:	${H}/hash.h
mzStart.o:	${H}/heap.h
mzStart.o:	${H}/database.h
mzStart.o:	${H}/signals.h
mzStart.o:	${H}/textio.h
mzStart.o:	${H}/list.h
mzStart.o:	${H}/debug.h
mzStart.o:	mzrouter.h
mzStart.o:	mzInternal.h
 
# include file dependencies for "mzSubrs.c"
mzSubrs.o:	mzSubrs.c
mzSubrs.o:	${H}/magic.h
mzSubrs.o:	${H}/geometry.h
mzSubrs.o:	${H}/tile.h
mzSubrs.o:	${H}/hash.h
mzSubrs.o:	${H}/database.h
mzSubrs.o:	${H}/drc.h
mzSubrs.o:	${H}/select.h
mzSubrs.o:	${H}/signals.h
mzSubrs.o:	${H}/textio.h
mzSubrs.o:	${H}/windows.h
mzSubrs.o:	${H}/dbwind.h
mzSubrs.o:	${H}/styles.h
mzSubrs.o:	${H}/debug.h
mzSubrs.o:	${H}/undo.h
mzSubrs.o:	${H}/txcommands.h
mzSubrs.o:	${H}/malloc.h
mzSubrs.o:	${H}/main.h
mzSubrs.o:	${H}/geofast.h
mzSubrs.o:	${H}/list.h
mzSubrs.o:	${H}/doubleint.h
mzSubrs.o:	${H}/touchingtypes.h
mzSubrs.o:	${H}/heap.h
mzSubrs.o:	mzrouter.h
mzSubrs.o:	mzInternal.h
 
# include file dependencies for "mzTech.c"
mzTech.o:	mzTech.c
mzTech.o:	${H}/magic.h
mzTech.o:	${H}/geometry.h
mzTech.o:	${H}/hash.h
mzTech.o:	${H}/doubleint.h
mzTech.o:	${H}/heap.h
mzTech.o:	${H}/tile.h
mzTech.o:	${H}/database.h
mzTech.o:	${H}/signals.h
mzTech.o:	${H}/textio.h
mzTech.o:	${H}/wiring.h
mzTech.o:	${H}/utils.h
mzTech.o:	${H}/tech.h
mzTech.o:	${H}/malloc.h
mzTech.o:	${H}/list.h
mzTech.o:	${H}/windows.h
mzTech.o:	${H}/main.h
mzTech.o:	mzrouter.h
mzTech.o:	mzInternal.h
 
# include file dependencies for "mzTestCmd.c"
mzTestCmd.o:	mzTestCmd.c
mzTestCmd.o:	${H}/magic.h
mzTestCmd.o:	${H}/geometry.h
mzTestCmd.o:	${H}/hash.h
mzTestCmd.o:	${H}/tile.h
mzTestCmd.o:	${H}/database.h
mzTestCmd.o:	${H}/signals.h
mzTestCmd.o:	${H}/textio.h
mzTestCmd.o:	${H}/irouter.h
mzTestCmd.o:	${H}/graphics.h
mzTestCmd.o:	${H}/windows.h
mzTestCmd.o:	${H}/dbwind.h
mzTestCmd.o:	${H}/dbwtech.h
mzTestCmd.o:	${H}/txcommands.h
mzTestCmd.o:	${H}/main.h
mzTestCmd.o:	${H}/utils.h
mzTestCmd.o:	${H}/commands.h
mzTestCmd.o:	${H}/styles.h
mzTestCmd.o:	${H}/malloc.h
mzTestCmd.o:	${H}/list.h
mzTestCmd.o:	${H}/doubleint.h
mzTestCmd.o:	${H}/heap.h
mzTestCmd.o:	mzrouter.h
mzTestCmd.o:	mzInternal.h
 
# include file dependencies for "mzWalk.c"
mzWalk.o:	mzWalk.c
mzWalk.o:	${H}/magic.h
mzWalk.o:	${H}/geometry.h
mzWalk.o:	${H}/geofast.h
mzWalk.o:	${H}/../tiles/tile.h
mzWalk.o:	${H}/doubleint.h
mzWalk.o:	${H}/hash.h
mzWalk.o:	${H}/heap.h
mzWalk.o:	${H}/database.h
mzWalk.o:	${H}/signals.h
mzWalk.o:	${H}/textio.h
mzWalk.o:	${H}/list.h
mzWalk.o:	${H}/debug.h
mzWalk.o:	mzrouter.h
mzWalk.o:	mzInternal.h
 
# include file dependencies for "mzXtndDown.c"
mzXtndDown.o:	mzXtndDown.c
mzXtndDown.o:	${H}/magic.h
mzXtndDown.o:	${H}/geometry.h
mzXtndDown.o:	${H}/geofast.h
mzXtndDown.o:	${H}/../tiles/tile.h
mzXtndDown.o:	${H}/doubleint.h
mzXtndDown.o:	${H}/hash.h
mzXtndDown.o:	${H}/heap.h
mzXtndDown.o:	${H}/database.h
mzXtndDown.o:	${H}/signals.h
mzXtndDown.o:	${H}/textio.h
mzXtndDown.o:	${H}/list.h
mzXtndDown.o:	${H}/debug.h
mzXtndDown.o:	mzrouter.h
mzXtndDown.o:	mzInternal.h
 
# include file dependencies for "mzXtndLeft.c"
mzXtndLeft.o:	mzXtndLeft.c
mzXtndLeft.o:	${H}/magic.h
mzXtndLeft.o:	${H}/geometry.h
mzXtndLeft.o:	${H}/geofast.h
mzXtndLeft.o:	${H}/../tiles/tile.h
mzXtndLeft.o:	${H}/doubleint.h
mzXtndLeft.o:	${H}/hash.h
mzXtndLeft.o:	${H}/heap.h
mzXtndLeft.o:	${H}/database.h
mzXtndLeft.o:	${H}/signals.h
mzXtndLeft.o:	${H}/textio.h
mzXtndLeft.o:	${H}/list.h
mzXtndLeft.o:	${H}/debug.h
mzXtndLeft.o:	mzrouter.h
mzXtndLeft.o:	mzInternal.h
 
# include file dependencies for "mzXtndRght.c"
mzXtndRght.o:	mzXtndRght.c
mzXtndRght.o:	${H}/magic.h
mzXtndRght.o:	${H}/geometry.h
mzXtndRght.o:	${H}/geofast.h
mzXtndRght.o:	${H}/../tiles/tile.h
mzXtndRght.o:	${H}/doubleint.h
mzXtndRght.o:	${H}/hash.h
mzXtndRght.o:	${H}/heap.h
mzXtndRght.o:	${H}/database.h
mzXtndRght.o:	${H}/signals.h
mzXtndRght.o:	${H}/textio.h
mzXtndRght.o:	${H}/list.h
mzXtndRght.o:	${H}/debug.h
mzXtndRght.o:	mzrouter.h
mzXtndRght.o:	mzInternal.h
 
# include file dependencies for "mzXtndUp.c"
mzXtndUp.o:	mzXtndUp.c
mzXtndUp.o:	${H}/magic.h
mzXtndUp.o:	${H}/geometry.h
mzXtndUp.o:	${H}/geofast.h
mzXtndUp.o:	${H}/../tiles/tile.h
mzXtndUp.o:	${H}/doubleint.h
mzXtndUp.o:	${H}/hash.h
mzXtndUp.o:	${H}/heap.h
mzXtndUp.o:	${H}/database.h
mzXtndUp.o:	${H}/signals.h
mzXtndUp.o:	${H}/textio.h
mzXtndUp.o:	${H}/list.h
mzXtndUp.o:	${H}/debug.h
mzXtndUp.o:	mzrouter.h
mzXtndUp.o:	mzInternal.h
 
