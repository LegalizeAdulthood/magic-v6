SHELL=/bin/sh
#
# rcsid="$Header: Makefile,v 6.0 90/08/29 13:11:21 mayo Exp $"
#

MODULE=	router
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

HDRS=   router.h rtrDcmpose.h routerInt.h 

SRCS=	rtrMain.c rtrDcmpose.c ftrFdback.c rtrHazards.c rtrChannel.c \
	rtrPaint.c rtrPin.c rtrSide.c rtrStem.c rtrTech.c rtrVia.c rtrTravers.c

OBJS=	rtrMain.o rtrDcmpose.o ftrFdback.o rtrHazards.o rtrChannel.o \
	rtrPaint.o rtrPin.o rtrSide.o rtrStem.o rtrTech.o rtrVia.o rtrTravers.o

POBJS=	rtrMain.po rtrDcmpose.po ftrFdback.po rtrHazards.po rtrChannel.po \
	rtrPaint.po rtrPin.po rtrSide.po rtrStem.po rtrTech.po rtrVia.po rtrTravers.po

ALLSRCS= ${SRCS} ${HDRS} Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln ${LIB}/lint/dbwind.ln \
	${LIB}/lint/drc.ln ${LIB}/lint/gcr.ln ${LIB}/lint/grouter.ln \
	${LIB}/lint/signals.ln ${LIB}/lint/tech.ln ${LIB}/lint/textio.ln \
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

installhdrs: ${H}/${MODULE}.h ${H}/rtrDcmpose.h ${H}/routerInt.h 

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

${H}/rtrDcmpose.h: rtrDcmpose.h
	rm -f ${H}/rtrDcmpose.h 
	cp rtrDcmpose.h ${H}

${H}/routerInt.h: routerInt.h
	rm -f ${H}/routerInt.h 
	cp routerInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}


# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:11:14 PDT 1990.
 
# include file dependencies for "ftrFdback.c"
ftrFdback.o:	ftrFdback.c
ftrFdback.o:	${H}/magic.h
ftrFdback.o:	${H}/geometry.h
ftrFdback.o:	${H}/styles.h
ftrFdback.o:	${H}/tile.h
ftrFdback.o:	${H}/hash.h
ftrFdback.o:	${H}/database.h
ftrFdback.o:	${H}/windows.h
ftrFdback.o:	${H}/main.h
ftrFdback.o:	${H}/utils.h
ftrFdback.o:	${H}/gcr.h
ftrFdback.o:	router.h
ftrFdback.o:	${H}/doubleint.h
ftrFdback.o:	${H}/heap.h
ftrFdback.o:	${H}/grouter.h
ftrFdback.o:	${H}/netlist.h
ftrFdback.o:	${H}/textio.h
ftrFdback.o:	${H}/dbwind.h
ftrFdback.o:	${H}/malloc.h
 
# include file dependencies for "rtrChannel.c"
rtrChannel.o:	rtrChannel.c
rtrChannel.o:	${H}/magic.h
rtrChannel.o:	${H}/geometry.h
rtrChannel.o:	${H}/geofast.h
rtrChannel.o:	${H}/hash.h
rtrChannel.o:	${H}/doubleint.h
rtrChannel.o:	${H}/heap.h
rtrChannel.o:	${H}/tile.h
rtrChannel.o:	${H}/database.h
rtrChannel.o:	${H}/gcr.h
rtrChannel.o:	${H}/windows.h
rtrChannel.o:	${H}/dbwind.h
rtrChannel.o:	${H}/signals.h
rtrChannel.o:	${H}/main.h
rtrChannel.o:	router.h
rtrChannel.o:	rtrDcmpose.h
rtrChannel.o:	${H}/grouter.h
rtrChannel.o:	${H}/textio.h
rtrChannel.o:	${H}/styles.h
 
# include file dependencies for "rtrDcmpose.c"
rtrDcmpose.o:	rtrDcmpose.c
rtrDcmpose.o:	${H}/magic.h
rtrDcmpose.o:	${H}/textio.h
rtrDcmpose.o:	${H}/geometry.h
rtrDcmpose.o:	${H}/geofast.h
rtrDcmpose.o:	${H}/tile.h
rtrDcmpose.o:	${H}/hash.h
rtrDcmpose.o:	${H}/database.h
rtrDcmpose.o:	${H}/windows.h
rtrDcmpose.o:	${H}/main.h
rtrDcmpose.o:	${H}/dbwind.h
rtrDcmpose.o:	${H}/doubleint.h
rtrDcmpose.o:	${H}/heap.h
rtrDcmpose.o:	${H}/undo.h
rtrDcmpose.o:	router.h
rtrDcmpose.o:	rtrDcmpose.h
rtrDcmpose.o:	${H}/gcr.h
rtrDcmpose.o:	${H}/grouter.h
rtrDcmpose.o:	${H}/netlist.h
rtrDcmpose.o:	${H}/styles.h
rtrDcmpose.o:	${H}/malloc.h
rtrDcmpose.o:	${H}/netmenu.h
rtrDcmpose.o:	${H}/debug.h
 
# include file dependencies for "rtrHazards.c"
rtrHazards.o:	rtrHazards.c
rtrHazards.o:	${H}/magic.h
rtrHazards.o:	${H}/geometry.h
rtrHazards.o:	${H}/tile.h
rtrHazards.o:	${H}/gcr.h
rtrHazards.o:	${H}/malloc.h
 
# include file dependencies for "rtrMain.c"
rtrMain.o:	rtrMain.c
rtrMain.o:	${H}/magic.h
rtrMain.o:	${H}/geometry.h
rtrMain.o:	${H}/hash.h
rtrMain.o:	${H}/doubleint.h
rtrMain.o:	${H}/heap.h
rtrMain.o:	${H}/tile.h
rtrMain.o:	${H}/database.h
rtrMain.o:	${H}/gcr.h
rtrMain.o:	${H}/windows.h
rtrMain.o:	${H}/dbwind.h
rtrMain.o:	${H}/main.h
rtrMain.o:	${H}/signals.h
rtrMain.o:	rtrDcmpose.h
rtrMain.o:	${H}/netmenu.h
rtrMain.o:	router.h
rtrMain.o:	${H}/grouter.h
rtrMain.o:	${H}/netlist.h
rtrMain.o:	${H}/textio.h
rtrMain.o:	${H}/netmenu.h
rtrMain.o:	${H}/runstats.h
 
# include file dependencies for "rtrPaint.c"
rtrPaint.o:	rtrPaint.c
rtrPaint.o:	${H}/magic.h
rtrPaint.o:	${H}/geometry.h
rtrPaint.o:	${H}/hash.h
rtrPaint.o:	${H}/doubleint.h
rtrPaint.o:	${H}/heap.h
rtrPaint.o:	${H}/tile.h
rtrPaint.o:	${H}/database.h
rtrPaint.o:	${H}/gcr.h
rtrPaint.o:	${H}/signals.h
rtrPaint.o:	router.h
rtrPaint.o:	${H}/grouter.h
rtrPaint.o:	${H}/windows.h
rtrPaint.o:	${H}/main.h
 
# include file dependencies for "rtrPin.c"
rtrPin.o:	rtrPin.c
rtrPin.o:	${H}/magic.h
rtrPin.o:	${H}/geometry.h
rtrPin.o:	${H}/styles.h
rtrPin.o:	${H}/hash.h
rtrPin.o:	${H}/doubleint.h
rtrPin.o:	${H}/heap.h
rtrPin.o:	${H}/debug.h
rtrPin.o:	${H}/tile.h
rtrPin.o:	${H}/database.h
rtrPin.o:	${H}/gcr.h
rtrPin.o:	${H}/windows.h
rtrPin.o:	${H}/main.h
rtrPin.o:	${H}/dbwind.h
rtrPin.o:	${H}/signals.h
rtrPin.o:	router.h
rtrPin.o:	${H}/grouter.h
rtrPin.o:	${H}/textio.h
 
# include file dependencies for "rtrSide.c"
rtrSide.o:	rtrSide.c
rtrSide.o:	${H}/magic.h
rtrSide.o:	${H}/geometry.h
rtrSide.o:	${H}/hash.h
rtrSide.o:	${H}/doubleint.h
rtrSide.o:	${H}/heap.h
rtrSide.o:	${H}/malloc.h
rtrSide.o:	${H}/debug.h
rtrSide.o:	${H}/tile.h
rtrSide.o:	${H}/database.h
rtrSide.o:	router.h
rtrSide.o:	${H}/gcr.h
rtrSide.o:	${H}/grouter.h
rtrSide.o:	${H}/textio.h
rtrSide.o:	${H}/windows.h
rtrSide.o:	${H}/main.h
rtrSide.o:	${H}/signals.h
 
# include file dependencies for "rtrStem.c"
rtrStem.o:	rtrStem.c
rtrStem.o:	${H}/magic.h
rtrStem.o:	${H}/geometry.h
rtrStem.o:	${H}/hash.h
rtrStem.o:	${H}/tile.h
rtrStem.o:	${H}/database.h
rtrStem.o:	${H}/windows.h
rtrStem.o:	${H}/dbwind.h
rtrStem.o:	${H}/styles.h
rtrStem.o:	router.h
rtrStem.o:	${H}/gcr.h
rtrStem.o:	${H}/doubleint.h
rtrStem.o:	${H}/heap.h
rtrStem.o:	${H}/grouter.h
rtrStem.o:	${H}/netlist.h
rtrStem.o:	${H}/textio.h
rtrStem.o:	${H}/main.h
rtrStem.o:	${H}/malloc.h
rtrStem.o:	${H}/debug.h
rtrStem.o:	${H}/signals.h
 
# include file dependencies for "rtrTech.c"
rtrTech.o:	rtrTech.c
rtrTech.o:	${H}/magic.h
rtrTech.o:	${H}/geometry.h
rtrTech.o:	${H}/tile.h
rtrTech.o:	${H}/hash.h
rtrTech.o:	${H}/database.h
rtrTech.o:	${H}/tech.h
rtrTech.o:	${H}/textio.h
rtrTech.o:	router.h
 
# include file dependencies for "rtrTravers.c"
rtrTravers.o:	rtrTravers.c
rtrTravers.o:	${H}/magic.h
rtrTravers.o:	${H}/geometry.h
rtrTravers.o:	${H}/tile.h
rtrTravers.o:	${H}/hash.h
rtrTravers.o:	${H}/database.h
rtrTravers.o:	${H}/signals.h
rtrTravers.o:	${H}/malloc.h
rtrTravers.o:	router.h
rtrTravers.o:	${H}/windows.h
rtrTravers.o:	${H}/main.h
rtrTravers.o:	${H}/gcr.h
rtrTravers.o:	routerInt.h
 
# include file dependencies for "rtrVia.c"
rtrVia.o:	rtrVia.c
rtrVia.o:	${H}/magic.h
rtrVia.o:	${H}/geometry.h
rtrVia.o:	${H}/tile.h
rtrVia.o:	${H}/hash.h
rtrVia.o:	${H}/doubleint.h
rtrVia.o:	${H}/heap.h
rtrVia.o:	${H}/database.h
rtrVia.o:	${H}/windows.h
rtrVia.o:	${H}/main.h
rtrVia.o:	${H}/netmenu.h
rtrVia.o:	${H}/textio.h
rtrVia.o:	${H}/styles.h
rtrVia.o:	${H}/malloc.h
rtrVia.o:	${H}/gcr.h
rtrVia.o:	router.h
rtrVia.o:	routerInt.h
 
