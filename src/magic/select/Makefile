SHELL=/bin/sh
#
# rcsid="$Header: Makefile,v 6.0 90/08/29 13:11:36 mayo Exp $"
#

MODULE=	select
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	selCreate.c selDisplay.c selEnum.c selOps.c selUndo.c selUnselect.c
OBJS=	selCreate.o selDisplay.o selEnum.o selOps.o selUndo.o selUnselect.o
HDRS=   select.h selInt.h
POBJS=	selCreate.po selDisplay.po selEnum.po selOps.po selUndo.po selUnselect.po
ALLSRCS= ${SRCS} ${HDRS} Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln ${LIB}/lint/dbwind.ln \
	${LIB}/lint/graphics.ln ${LIB}/lint/signals.ln \
	${LIB}/lint/tech.ln ${LIB}/lint/textio.ln ${LIB}/lint/tiles.ln \
	${LIB}/lint/utils.ln ${LIB}/lint/undo.ln ${LIB}/lint/windows.ln
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

installhdrs: ${H}/${MODULE}.h ${H}/selInt.h

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

${H}/selInt.h: selInt.h
	rm -f ${H}/selInt.h 
	cp selInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:11:31 PDT 1990.
 
# include file dependencies for "selCreate.c"
selCreate.o:	selCreate.c
selCreate.o:	${H}/magic.h
selCreate.o:	${H}/geometry.h
selCreate.o:	${H}/tile.h
selCreate.o:	${H}/hash.h
selCreate.o:	${H}/database.h
selCreate.o:	${H}/windows.h
selCreate.o:	${H}/dbwind.h
selCreate.o:	${H}/undo.h
selCreate.o:	${H}/commands.h
selCreate.o:	selInt.h
selCreate.o:	${H}/drc.h
selCreate.o:	${H}/main.h
selCreate.o:	${H}/signals.h
 
# include file dependencies for "selDisplay.c"
selDisplay.o:	selDisplay.c
selDisplay.o:	${H}/magic.h
selDisplay.o:	${H}/geometry.h
selDisplay.o:	${H}/graphics.h
selDisplay.o:	${H}/tile.h
selDisplay.o:	${H}/hash.h
selDisplay.o:	${H}/database.h
selDisplay.o:	${H}/windows.h
selDisplay.o:	${H}/dbwind.h
selDisplay.o:	${H}/styles.h
selDisplay.o:	${H}/textio.h
selDisplay.o:	${H}/signals.h
 
# include file dependencies for "selEnum.c"
selEnum.o:	selEnum.c
selEnum.o:	${H}/magic.h
selEnum.o:	${H}/geometry.h
selEnum.o:	${H}/tile.h
selEnum.o:	${H}/hash.h
selEnum.o:	${H}/database.h
selEnum.o:	${H}/windows.h
selEnum.o:	${H}/main.h
selEnum.o:	select.h
selEnum.o:	selInt.h
selEnum.o:	${H}/malloc.h
selEnum.o:	${H}/textio.h
 
# include file dependencies for "selOps.c"
selOps.o:	selOps.c
selOps.o:	${H}/magic.h
selOps.o:	${H}/geometry.h
selOps.o:	${H}/tile.h
selOps.o:	${H}/hash.h
selOps.o:	${H}/database.h
selOps.o:	${H}/windows.h
selOps.o:	${H}/dbwind.h
selOps.o:	${H}/main.h
selOps.o:	select.h
selOps.o:	selInt.h
selOps.o:	${H}/textio.h
selOps.o:	${H}/undo.h
selOps.o:	${H}/plow.h
selOps.o:	${H}/malloc.h
selOps.o:	${H}/drc.h
 
# include file dependencies for "selUndo.c"
selUndo.o:	selUndo.c
selUndo.o:	${H}/magic.h
selUndo.o:	${H}/geometry.h
selUndo.o:	${H}/graphics.h
selUndo.o:	${H}/tile.h
selUndo.o:	${H}/hash.h
selUndo.o:	${H}/database.h
selUndo.o:	${H}/windows.h
selUndo.o:	${H}/dbwind.h
selUndo.o:	${H}/undo.h
selUndo.o:	${H}/textio.h
selUndo.o:	select.h
selUndo.o:	selInt.h
 
# include file dependencies for "selUnselect.c"
selUnselect.o:	selUnselect.c
selUnselect.o:	${H}/magic.h
selUnselect.o:	${H}/geometry.h
selUnselect.o:	${H}/geofast.h
selUnselect.o:	${H}/tile.h
selUnselect.o:	${H}/hash.h
selUnselect.o:	${H}/database.h
selUnselect.o:	${H}/windows.h
selUnselect.o:	${H}/dbwind.h
selUnselect.o:	${H}/commands.h
selUnselect.o:	${H}/main.h
selUnselect.o:	select.h
selUnselect.o:	selInt.h
selUnselect.o:	${H}/malloc.h
selUnselect.o:	${H}/textio.h
 
