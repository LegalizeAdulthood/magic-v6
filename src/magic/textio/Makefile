SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 13:12:16 mayo Exp $
#

MODULE=	textio
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
LLIB=	${MAGICDIR}/lib/lint
BIN=	${MAGICDIR}/bin


SRCS=	txCommands.c txInput.c txMain.c txMore.c  txOutput.c 
OBJS=	txCommands.o txInput.o txMain.o txMore.o  txOutput.o 
POBJS=	txCommands.po txInput.po txMain.po txMore.po txOutput.po 
ALLSRCS= ${SRCS} ${MODULE}.h textioInt.h  txcommands.h Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=${IFLAGS} -u
LINTLIBS= ${LLIB}/utils.ln ${LLIB}/windows.ln ${LLIB}/parser.ln \
    ${LLIB}/lintfix.c

.SUFFIXES: .po

${MODULE}.o: ${OBJS}
	rm -f ${MODULE}.o
	ld -r ${OBJS} -o ${MODULE}.o

${MODULE}.po: ${POBJS}
	rm -f ${MODULE}.po
	ld -r ${OBJS} -o ${MODULE}.po

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

installhdrs: ${H}/${MODULE}.h ${H}/txcommands.h ${H}/textioInt.h

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -r ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/${MODULE}.po: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${H}/txcommands.h: txcommands.h
	rm -f ${H}/txcommands.h 
	cp txcommands.h ${H}

${H}/textioInt.h: textioInt.h
	rm -f ${H}/textioInt.h 
	cp textioInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:12:11 PDT 1990.
 
# include file dependencies for "txCommands.c"
txCommands.o:	txCommands.c
txCommands.o:	${H}/magic.h
txCommands.o:	textio.h
txCommands.o:	${H}/geometry.h
txCommands.o:	txcommands.h
txCommands.o:	textioInt.h
txCommands.o:	${H}/macros.h
txCommands.o:	${H}/windows.h
txCommands.o:	${H}/tile.h
txCommands.o:	${H}/hash.h
txCommands.o:	${H}/database.h
txCommands.o:	${H}/drc.h
txCommands.o:	${H}/signals.h
txCommands.o:	${H}/graphics.h
txCommands.o:	${H}/dqueue.h
txCommands.o:	${H}/malloc.h
txCommands.o:	${H}/utils.h
 
# include file dependencies for "txInput.c"
txInput.o:	txInput.c
txInput.o:	${H}/magic.h
txInput.o:	textio.h
txInput.o:	${H}/geometry.h
txInput.o:	txcommands.h
txInput.o:	textioInt.h
txInput.o:	${H}/dqueue.h
txInput.o:	${H}/graphics.h
 
# include file dependencies for "txMain.c"
txMain.o:	txMain.c
txMain.o:	${H}/magic.h
txMain.o:	textio.h
txMain.o:	${H}/geometry.h
txMain.o:	txcommands.h
txMain.o:	textioInt.h
 
# include file dependencies for "txMore.c"
txMore.o:	txMore.c
txMore.o:	${H}/magic.h
 
# include file dependencies for "txOutput.c"
txOutput.o:	txOutput.c
txOutput.o:	${H}/magic.h
txOutput.o:	textio.h
txOutput.o:	${H}/geometry.h
txOutput.o:	txcommands.h
txOutput.o:	textioInt.h
txOutput.o:	${H}/graphics.h
txOutput.o:	${H}/paths.h
 
