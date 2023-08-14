SHELL=/bin/sh
#
# Makefile
# rcsid $Header: Makefile,v 6.0 90/08/29 13:10:41 mayo Exp $
#
MODULE=	plow
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

OBJS=	PlowJogs.o PlowMain.o PlowQueue.o PlowRandom.o \
	PlowRules1.o PlowRules2.o PlowRules3.o PlowSearch.o \
	PlowTech.o PlowTest.o PlowWidth.o PlowYank.o
POBJS=	PlowJogs.po PlowMain.po PlowQueue.po PlowRandom.po \
	PlowRules1.po PlowRules2.po PlowRules3.po PlowSearch.po \
	PlowTech.po PlowTest.po PlowWidth.o PlowYank.po
SRCS=	PlowJogs.c PlowMain.c PlowQueue.c PlowRandom.c \
	PlowRules1.c PlowRules2.c PlowRules3.c PlowSearch.c \
	PlowTech.c PlowTest.c PlowWidth.c PlowYank.c
HDRS=	plow.h plowInt.h
LINTFLAGS=-I. -I${H} -u
LINTLIBS=${LIB}/lint/database.ln ${LIB}/lint/tiles.ln \
	 ${LIB}/lint/commands.ln ${LIB}/lint/utils.ln \
	 ${LIB}/lint/dbwind.ln ${LIB}/lint/drc.ln \
	 ${LIB}/lint/extract.ln ${LIB}/lint/textio.ln  \
	 ${LIB}/lint/tech.ln
ALLSRCS= ${SRCS} ${HDRS} Makefile

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

profile: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}/${MODULE}.po

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln

installhdrs: ${H}/plow.h ${H}/plowInt.h ${H}/plowDebugInt.h

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -x -r ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${H}/plow.h: plow.h
	rm -f ${H}/plow.h 
	cp plow.h ${H}

${H}/plowInt.h: plowInt.h
	rm -f ${H}/plowInt.h 
	cp plowInt.h ${H}

${H}/plowDebugInt.h: plowDebugInt.h
	rm -f ${H}/plowDebugInt.h 
	cp plowDebugInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:10:32 PDT 1990.
 
# include file dependencies for "PlowJogs.c"
PlowJogs.o:	PlowJogs.c
PlowJogs.o:	${H}/magic.h
PlowJogs.o:	${H}/geometry.h
PlowJogs.o:	${H}/tile.h
PlowJogs.o:	${H}/hash.h
PlowJogs.o:	${H}/database.h
PlowJogs.o:	${H}/undo.h
PlowJogs.o:	plow.h
PlowJogs.o:	plowInt.h
PlowJogs.o:	${H}/malloc.h
PlowJogs.o:	${H}/debug.h
 
# include file dependencies for "PlowMain.c"
PlowMain.o:	PlowMain.c
PlowMain.o:	${H}/magic.h
PlowMain.o:	${H}/geometry.h
PlowMain.o:	${H}/tile.h
PlowMain.o:	${H}/hash.h
PlowMain.o:	${H}/database.h
PlowMain.o:	${H}/undo.h
PlowMain.o:	${H}/debug.h
PlowMain.o:	plow.h
PlowMain.o:	plowInt.h
PlowMain.o:	${H}/textio.h
PlowMain.o:	${H}/windows.h
PlowMain.o:	${H}/dbwind.h
PlowMain.o:	${H}/drc.h
PlowMain.o:	${H}/styles.h
PlowMain.o:	${H}/malloc.h
PlowMain.o:	${H}/signals.h
PlowMain.o:	${H}/main.h
 
# include file dependencies for "PlowQueue.c"
PlowQueue.o:	PlowQueue.c
PlowQueue.o:	${H}/magic.h
PlowQueue.o:	${H}/geometry.h
PlowQueue.o:	${H}/tile.h
PlowQueue.o:	${H}/hash.h
PlowQueue.o:	${H}/database.h
PlowQueue.o:	${H}/debug.h
PlowQueue.o:	plowInt.h
PlowQueue.o:	${H}/malloc.h
 
# include file dependencies for "PlowRandom.c"
PlowRandom.o:	PlowRandom.c
PlowRandom.o:	${H}/magic.h
PlowRandom.o:	${H}/geometry.h
PlowRandom.o:	${H}/tile.h
PlowRandom.o:	${H}/hash.h
PlowRandom.o:	${H}/database.h
PlowRandom.o:	plowInt.h
PlowRandom.o:	${H}/textio.h
PlowRandom.o:	${H}/undo.h
PlowRandom.o:	${H}/signals.h
 
# include file dependencies for "PlowRules1.c"
PlowRules1.o:	PlowRules1.c
PlowRules1.o:	${H}/magic.h
PlowRules1.o:	${H}/geometry.h
PlowRules1.o:	${H}/tile.h
PlowRules1.o:	${H}/hash.h
PlowRules1.o:	${H}/database.h
PlowRules1.o:	${H}/undo.h
PlowRules1.o:	plow.h
PlowRules1.o:	plowInt.h
PlowRules1.o:	${H}/drc.h
 
# include file dependencies for "PlowRules2.c"
PlowRules2.o:	PlowRules2.c
PlowRules2.o:	${H}/magic.h
PlowRules2.o:	${H}/geometry.h
PlowRules2.o:	${H}/tile.h
PlowRules2.o:	${H}/hash.h
PlowRules2.o:	${H}/database.h
PlowRules2.o:	${H}/undo.h
PlowRules2.o:	plow.h
PlowRules2.o:	plowInt.h
PlowRules2.o:	${H}/drc.h
 
# include file dependencies for "PlowRules3.c"
PlowRules3.o:	PlowRules3.c
PlowRules3.o:	${H}/magic.h
PlowRules3.o:	${H}/geometry.h
PlowRules3.o:	${H}/tile.h
PlowRules3.o:	${H}/hash.h
PlowRules3.o:	${H}/database.h
PlowRules3.o:	${H}/undo.h
PlowRules3.o:	plow.h
PlowRules3.o:	plowInt.h
PlowRules3.o:	${H}/drc.h
 
# include file dependencies for "PlowSearch.c"
PlowSearch.o:	PlowSearch.c
PlowSearch.o:	${H}/magic.h
PlowSearch.o:	${H}/geometry.h
PlowSearch.o:	${H}/tile.h
PlowSearch.o:	${H}/hash.h
PlowSearch.o:	${H}/database.h
PlowSearch.o:	plowInt.h
PlowSearch.o:	${H}/stack.h
PlowSearch.o:	${H}/textio.h
 
# include file dependencies for "PlowTech.c"
PlowTech.o:	PlowTech.c
PlowTech.o:	${H}/magic.h
PlowTech.o:	${H}/geometry.h
PlowTech.o:	${H}/tile.h
PlowTech.o:	${H}/hash.h
PlowTech.o:	${H}/database.h
PlowTech.o:	${H}/utils.h
PlowTech.o:	${H}/malloc.h
PlowTech.o:	plowInt.h
PlowTech.o:	${H}/drc.h
 
# include file dependencies for "PlowTest.c"
PlowTest.o:	PlowTest.c
PlowTest.o:	${H}/magic.h
PlowTest.o:	${H}/styles.h
PlowTest.o:	${H}/utils.h
PlowTest.o:	${H}/geometry.h
PlowTest.o:	${H}/graphics.h
PlowTest.o:	${H}/tile.h
PlowTest.o:	${H}/hash.h
PlowTest.o:	${H}/database.h
PlowTest.o:	${H}/windows.h
PlowTest.o:	${H}/dbwind.h
PlowTest.o:	${H}/main.h
PlowTest.o:	${H}/commands.h
PlowTest.o:	${H}/drc.h
PlowTest.o:	${H}/debug.h
PlowTest.o:	plowInt.h
PlowTest.o:	${H}/textio.h
PlowTest.o:	${H}/txcommands.h
 
# include file dependencies for "PlowWidth.c"
PlowWidth.o:	PlowWidth.c
PlowWidth.o:	${H}/magic.h
PlowWidth.o:	${H}/geometry.h
PlowWidth.o:	${H}/tile.h
PlowWidth.o:	${H}/hash.h
PlowWidth.o:	${H}/database.h
PlowWidth.o:	plowInt.h
 
# include file dependencies for "PlowYank.c"
PlowYank.o:	PlowYank.c
PlowYank.o:	${H}/magic.h
PlowYank.o:	${H}/geometry.h
PlowYank.o:	${H}/geofast.h
PlowYank.o:	${H}/tile.h
PlowYank.o:	${H}/hash.h
PlowYank.o:	${H}/database.h
PlowYank.o:	${H}/undo.h
PlowYank.o:	${H}/debug.h
PlowYank.o:	plow.h
PlowYank.o:	plowInt.h
PlowYank.o:	${H}/textio.h
PlowYank.o:	${H}/windows.h
PlowYank.o:	${H}/dbwind.h
PlowYank.o:	${H}/drc.h
 
