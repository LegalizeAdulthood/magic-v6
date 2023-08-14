SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 13:08:35 mayo Exp $
#

MODULE=	irouter
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

HDRS=   irouter.h irInternal.h irDebug.h
SRCS=	irCommand.c  irMain.c  irRoute.c  irTestCmd.c  irUtils.c
OBJS=	irCommand.o  irMain.o  irRoute.o  irTestCmd.o  irUtils.o
POBJS=	irCommand.po irMain.po irRoute.po irTestCmd.po irUtils.po

ALLSRCS= ${SRCS} ${HDRS} Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= \
	${LIB}/lint/commands.ln ${LIB}/lint/database.ln ${LIB}/lint/dbwind.ln \
	${LIB}/lint/debug.ln ${LIB}/lint/drc.ln ${LIB}/lint/main.ln \
	${LIB}/lint/misc.ln ${LIB}/lint/mzrouter.ln \
	${LIB}/lint/select.ln ${LIB}/lint/textio.ln ${LIB}/lint/utils.ln \
	${LIB}/lint/windows.ln
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

installhdrs: ${H}/${MODULE}.h ${H}/irInternal.h ${H}/irDebug.h

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

${H}/irInternal.h: irInternal.h
	rm -f ${H}/irInternal.h 
	cp irInternal.h ${H}

${H}/irDebug.h: irDebug.h
	rm -f ${H}/irDebug.h 
	cp irDebug.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:08:30 PDT 1990.
 
# include file dependencies for "irCommand.c"
irCommand.o:	irCommand.c
irCommand.o:	${H}/magic.h
irCommand.o:	${H}/geometry.h
irCommand.o:	${H}/hash.h
irCommand.o:	${H}/tile.h
irCommand.o:	${H}/database.h
irCommand.o:	${H}/signals.h
irCommand.o:	${H}/textio.h
irCommand.o:	${H}/graphics.h
irCommand.o:	${H}/windows.h
irCommand.o:	${H}/dbwind.h
irCommand.o:	${H}/dbwtech.h
irCommand.o:	${H}/txcommands.h
irCommand.o:	${H}/main.h
irCommand.o:	${H}/utils.h
irCommand.o:	${H}/commands.h
irCommand.o:	${H}/styles.h
irCommand.o:	${H}/malloc.h
irCommand.o:	${H}/list.h
irCommand.o:	${H}/doubleint.h
irCommand.o:	${H}/../mzrouter/mzrouter.h
irCommand.o:	irouter.h
irCommand.o:	irInternal.h
 
# include file dependencies for "irMain.c"
irMain.o:	irMain.c
irMain.o:	${H}/magic.h
irMain.o:	${H}/geometry.h
irMain.o:	${H}/tile.h
irMain.o:	${H}/hash.h
irMain.o:	${H}/database.h
irMain.o:	${H}/drc.h
irMain.o:	${H}/select.h
irMain.o:	${H}/signals.h
irMain.o:	${H}/textio.h
irMain.o:	${H}/windows.h
irMain.o:	${H}/dbwind.h
irMain.o:	${H}/styles.h
irMain.o:	${H}/debug.h
irMain.o:	${H}/undo.h
irMain.o:	${H}/txcommands.h
irMain.o:	${H}/malloc.h
irMain.o:	${H}/list.h
irMain.o:	${H}/doubleint.h
irMain.o:	${H}/../mzrouter/mzrouter.h
irMain.o:	irouter.h
irMain.o:	irInternal.h
 
# include file dependencies for "irRoute.c"
irRoute.o:	irRoute.c
irRoute.o:	${H}/magic.h
irRoute.o:	${H}/geometry.h
irRoute.o:	${H}/hash.h
irRoute.o:	${H}/tile.h
irRoute.o:	${H}/database.h
irRoute.o:	${H}/drc.h
irRoute.o:	${H}/textio.h
irRoute.o:	${H}/windows.h
irRoute.o:	${H}/main.h
irRoute.o:	${H}/dbwind.h
irRoute.o:	${H}/debug.h
irRoute.o:	${H}/undo.h
irRoute.o:	${H}/signals.h
irRoute.o:	${H}/malloc.h
irRoute.o:	${H}/list.h
irRoute.o:	${H}/doubleint.h
irRoute.o:	${H}/geofast.h
irRoute.o:	${H}/touchingtypes.h
irRoute.o:	${H}/select.h
irRoute.o:	${H}/../mzrouter/mzrouter.h
irRoute.o:	irouter.h
irRoute.o:	irInternal.h
 
# include file dependencies for "irTestCmd.c"
irTestCmd.o:	irTestCmd.c
irTestCmd.o:	${H}/magic.h
irTestCmd.o:	${H}/geometry.h
irTestCmd.o:	${H}/hash.h
irTestCmd.o:	${H}/tile.h
irTestCmd.o:	${H}/database.h
irTestCmd.o:	${H}/signals.h
irTestCmd.o:	${H}/textio.h
irTestCmd.o:	irouter.h
irTestCmd.o:	${H}/graphics.h
irTestCmd.o:	${H}/windows.h
irTestCmd.o:	${H}/dbwind.h
irTestCmd.o:	${H}/dbwtech.h
irTestCmd.o:	${H}/txcommands.h
irTestCmd.o:	${H}/main.h
irTestCmd.o:	${H}/utils.h
irTestCmd.o:	${H}/commands.h
irTestCmd.o:	${H}/styles.h
irTestCmd.o:	${H}/malloc.h
irTestCmd.o:	${H}/list.h
irTestCmd.o:	${H}/doubleint.h
irTestCmd.o:	${H}/../mzrouter/mzrouter.h
irTestCmd.o:	irInternal.h
 
# include file dependencies for "irUtils.c"
irUtils.o:	irUtils.c
irUtils.o:	${H}/magic.h
irUtils.o:	${H}/geometry.h
irUtils.o:	${H}/tile.h
irUtils.o:	${H}/hash.h
irUtils.o:	${H}/database.h
irUtils.o:	${H}/doubleint.h
irUtils.o:	${H}/list.h
irUtils.o:	${H}/../mzrouter/mzrouter.h
irUtils.o:	irouter.h
irUtils.o:	irInternal.h
 
