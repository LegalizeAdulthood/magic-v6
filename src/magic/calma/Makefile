SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 11:39:52 mayo Exp $
#

MODULE=calma
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	CalmaRead.c CalmaRdcl.c CalmaRdio.c CalmaRdpt.c \
	CalmaWrite.c
OBJS=	CalmaRead.o CalmaRdcl.o CalmaRdio.o CalmaRdpt.o \
	CalmaWrite.o
POBJS=	CalmaRead.po CalmaRdcl.po CalmaRdio.po CalmaRdpt.po \
	CalmaWrite.po
ALLSRCS= ${SRCS} ${MODULE}.h calmaInt.h Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln \
	${LIB}/lint/textio.ln ${LIB}/lint/tiles.ln ${LIB}/lint/utils.ln \
	${LIB}/lint/tech.ln ${LIB}/lint/undo.ln ${LIB}/lint/cif.ln
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

lint:   *.h ${SRCS} ${LINTLIBS}
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

profile: installhdrs ${LIB}/${MODULE}.po ${LIB}/lint/${MODULE}.ln

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln

installhdrs: ${H}/${MODULE}.h ${H}/calmaInt.h


${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -x -r ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/${MODULE}.po: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${H}/calmaInt.h: calmaInt.h
	rm -f ${H}/calmaInt.h 
	cp calmaInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:39:44 PDT 1990.
 
# include file dependencies for "CalmaRdcl.c"
CalmaRdcl.o:	CalmaRdcl.c
CalmaRdcl.o:	${H}/magic.h
CalmaRdcl.o:	${H}/geometry.h
CalmaRdcl.o:	${H}/tile.h
CalmaRdcl.o:	${H}/utils.h
CalmaRdcl.o:	${H}/hash.h
CalmaRdcl.o:	${H}/database.h
CalmaRdcl.o:	${H}/databaseInt.h
CalmaRdcl.o:	${H}/malloc.h
CalmaRdcl.o:	${H}/tech.h
CalmaRdcl.o:	${H}/cif.h
CalmaRdcl.o:	${H}/CIFint.h
CalmaRdcl.o:	${H}/CIFread.h
CalmaRdcl.o:	${H}/signals.h
CalmaRdcl.o:	${H}/windows.h
CalmaRdcl.o:	${H}/dbwind.h
CalmaRdcl.o:	${H}/styles.h
CalmaRdcl.o:	${H}/textio.h
CalmaRdcl.o:	calmaInt.h
 
# include file dependencies for "CalmaRdio.c"
CalmaRdio.o:	CalmaRdio.c
CalmaRdio.o:	${H}/magic.h
CalmaRdio.o:	${H}/geometry.h
CalmaRdio.o:	${H}/tile.h
CalmaRdio.o:	${H}/utils.h
CalmaRdio.o:	${H}/hash.h
CalmaRdio.o:	${H}/database.h
CalmaRdio.o:	${H}/databaseInt.h
CalmaRdio.o:	${H}/malloc.h
CalmaRdio.o:	${H}/tech.h
CalmaRdio.o:	${H}/cif.h
CalmaRdio.o:	${H}/CIFint.h
CalmaRdio.o:	${H}/CIFread.h
CalmaRdio.o:	${H}/signals.h
CalmaRdio.o:	${H}/windows.h
CalmaRdio.o:	${H}/dbwind.h
CalmaRdio.o:	${H}/styles.h
CalmaRdio.o:	${H}/textio.h
CalmaRdio.o:	calmaInt.h
 
# include file dependencies for "CalmaRdpt.c"
CalmaRdpt.o:	CalmaRdpt.c
CalmaRdpt.o:	${H}/magic.h
CalmaRdpt.o:	${H}/geometry.h
CalmaRdpt.o:	${H}/tile.h
CalmaRdpt.o:	${H}/utils.h
CalmaRdpt.o:	${H}/hash.h
CalmaRdpt.o:	${H}/database.h
CalmaRdpt.o:	${H}/databaseInt.h
CalmaRdpt.o:	${H}/malloc.h
CalmaRdpt.o:	${H}/tech.h
CalmaRdpt.o:	${H}/cif.h
CalmaRdpt.o:	${H}/CIFint.h
CalmaRdpt.o:	${H}/CIFread.h
CalmaRdpt.o:	${H}/signals.h
CalmaRdpt.o:	${H}/windows.h
CalmaRdpt.o:	${H}/dbwind.h
CalmaRdpt.o:	${H}/styles.h
CalmaRdpt.o:	${H}/textio.h
CalmaRdpt.o:	calmaInt.h
 
# include file dependencies for "CalmaRead.c"
CalmaRead.o:	CalmaRead.c
CalmaRead.o:	${H}/magic.h
CalmaRead.o:	${H}/geometry.h
CalmaRead.o:	${H}/tile.h
CalmaRead.o:	${H}/utils.h
CalmaRead.o:	${H}/hash.h
CalmaRead.o:	${H}/database.h
CalmaRead.o:	${H}/databaseInt.h
CalmaRead.o:	${H}/malloc.h
CalmaRead.o:	${H}/tech.h
CalmaRead.o:	${H}/cif.h
CalmaRead.o:	${H}/CIFint.h
CalmaRead.o:	${H}/CIFread.h
CalmaRead.o:	${H}/signals.h
CalmaRead.o:	${H}/windows.h
CalmaRead.o:	${H}/dbwind.h
CalmaRead.o:	${H}/styles.h
CalmaRead.o:	${H}/textio.h
CalmaRead.o:	calmaInt.h
CalmaRead.o:	${H}/undo.h
 
# include file dependencies for "CalmaWrite.c"
CalmaWrite.o:	CalmaWrite.c
CalmaWrite.o:	${H}/magic.h
CalmaWrite.o:	${H}/geometry.h
CalmaWrite.o:	${H}/tile.h
CalmaWrite.o:	${H}/utils.h
CalmaWrite.o:	${H}/hash.h
CalmaWrite.o:	${H}/database.h
CalmaWrite.o:	${H}/databaseInt.h
CalmaWrite.o:	${H}/tech.h
CalmaWrite.o:	${H}/cif.h
CalmaWrite.o:	${H}/CIFint.h
CalmaWrite.o:	${H}/signals.h
CalmaWrite.o:	${H}/windows.h
CalmaWrite.o:	${H}/dbwind.h
CalmaWrite.o:	${H}/styles.h
CalmaWrite.o:	${H}/textio.h
CalmaWrite.o:	calmaInt.h
 
