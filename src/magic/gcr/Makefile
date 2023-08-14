SHELL=/bin/sh
#
# Makefile	rcsid "$Header: Makefile,v 6.0 90/08/29 11:43:06 mayo Exp $"
#
MODULE=gcr
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}


BIN=	${MAGICDIR}/bin

ALLSRCS= ${SRCS} ${HDRS} Makefile main.c

OBJS=	gcrChannel.o gcrColl.o gcrDebug.o gcrEdge.o gcrFeas.o gcrFlags.o \
	gcrInit.o gcrLib.o gcrRiver.o gcrRoute.o \
	gcrShwFlgs.o gcrUnsplit.o

SRCS=	gcrChannel.c gcrColl.c gcrDebug.c gcrEdge.c gcrFeas.c gcrFlags.c \
	gcrInit.c gcrLib.c gcrRiver.c gcrRoute.c \
	gcrShwFlgs.c gcrUnsplit.c

POBJS=	gcrChannel.po gcrColl.po gcrDebug.po gcrEdge.po gcrFeas.po gcrFlags.po \
	gcrInit.po gcrLib.po gcrRiver.po gcrRoute.po \
	gcrShwFlgs.po gcrUnsplit.po

HDRS=	gcr.h

LINTFLAGS=${IFLAGS} -u
LINTLIBS= ${LIB}/lint/lintfix.c ${LIB}/lint/database.ln \
	${LIB}/lint/grouter.ln ${LIB}/lint/router.ln \
	${LIB}/lint/signals.ln ${LIB}/lint/tiles.ln ${LIB}/lint/utils.ln

ALLSRCS= ${SRCS} ${HDRS} Makefile

.SUFFIXES: .po

gcr.o:	${OBJS}
	rm -f gcr.o
	ld -r -o gcr.o ${OBJS}

gcr.po:	${POBJS}
	rm -f gcr.po
	ld -r -o gcr.po ${POBJS}

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

profile: ${LIB}/gcr.po

tags:
	ctags ${SRCS}

.c.po :
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

profile: installhdrs ${LIB}/gcr.po ${LIB}/lint/gcr.ln

install: installhdrs ${LIB}/gcr.o ${LIB}/lint/gcr.ln

installhdrs: ${H}/${MODULE}.h

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -r -x ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/gcr.po: gcr.po
	rm -f ${LIB}/gcr.po
	cp gcr.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:43:01 PDT 1990.
 
# include file dependencies for "gcrChannel.c"
gcrChannel.o:	gcrChannel.c
gcrChannel.o:	${H}/magic.h
gcrChannel.o:	${H}/geometry.h
gcrChannel.o:	gcr.h
gcrChannel.o:	${H}/malloc.h
 
# include file dependencies for "gcrColl.c"
gcrColl.o:	gcrColl.c
gcrColl.o:	${H}/magic.h
gcrColl.o:	${H}/geometry.h
gcrColl.o:	${H}/tile.h
gcrColl.o:	gcr.h
gcrColl.o:	${H}/malloc.h
 
# include file dependencies for "gcrDebug.c"
gcrDebug.o:	gcrDebug.c
gcrDebug.o:	${H}/magic.h
gcrDebug.o:	${H}/geometry.h
gcrDebug.o:	${H}/textio.h
gcrDebug.o:	${H}/tile.h
gcrDebug.o:	${H}/hash.h
gcrDebug.o:	${H}/database.h
gcrDebug.o:	gcr.h
gcrDebug.o:	${H}/doubleint.h
gcrDebug.o:	${H}/heap.h
gcrDebug.o:	${H}/router.h
gcrDebug.o:	${H}/malloc.h
 
# include file dependencies for "gcrEdge.c"
gcrEdge.o:	gcrEdge.c
gcrEdge.o:	${H}/magic.h
gcrEdge.o:	${H}/geometry.h
gcrEdge.o:	${H}/tile.h
gcrEdge.o:	gcr.h
 
# include file dependencies for "gcrFeas.c"
gcrFeas.o:	gcrFeas.c
gcrFeas.o:	${H}/magic.h
gcrFeas.o:	${H}/geometry.h
gcrFeas.o:	${H}/tile.h
gcrFeas.o:	gcr.h
gcrFeas.o:	${H}/hash.h
gcrFeas.o:	${H}/database.h
gcrFeas.o:	${H}/router.h
 
# include file dependencies for "gcrFlags.c"
gcrFlags.o:	gcrFlags.c
gcrFlags.o:	${H}/magic.h
gcrFlags.o:	${H}/geometry.h
gcrFlags.o:	${H}/tile.h
gcrFlags.o:	gcr.h
 
# include file dependencies for "gcrInit.c"
gcrInit.o:	gcrInit.c
gcrInit.o:	${H}/magic.h
gcrInit.o:	${H}/geometry.h
gcrInit.o:	${H}/hash.h
gcrInit.o:	${H}/doubleint.h
gcrInit.o:	${H}/heap.h
gcrInit.o:	${H}/tile.h
gcrInit.o:	${H}/database.h
gcrInit.o:	${H}/router.h
gcrInit.o:	gcr.h
gcrInit.o:	${H}/malloc.h
 
# include file dependencies for "gcrLib.c"
gcrLib.o:	gcrLib.c
gcrLib.o:	${H}/magic.h
gcrLib.o:	${H}/geometry.h
gcrLib.o:	${H}/tile.h
gcrLib.o:	gcr.h
gcrLib.o:	${H}/malloc.h
 
# include file dependencies for "gcrRiver.c"
gcrRiver.o:	gcrRiver.c
gcrRiver.o:	${H}/magic.h
gcrRiver.o:	${H}/geometry.h
gcrRiver.o:	gcr.h
gcrRiver.o:	${H}/textio.h
gcrRiver.o:	${H}/malloc.h
 
# include file dependencies for "gcrRoute.c"
gcrRoute.o:	gcrRoute.c
gcrRoute.o:	${H}/magic.h
gcrRoute.o:	${H}/geometry.h
gcrRoute.o:	gcr.h
gcrRoute.o:	${H}/signals.h
gcrRoute.o:	${H}/malloc.h
gcrRoute.o:	${H}/styles.h
 
# include file dependencies for "gcrShwFlgs.c"
gcrShwFlgs.o:	gcrShwFlgs.c
gcrShwFlgs.o:	${H}/magic.h
gcrShwFlgs.o:	${H}/geometry.h
gcrShwFlgs.o:	${H}/hash.h
gcrShwFlgs.o:	${H}/tile.h
gcrShwFlgs.o:	${H}/database.h
gcrShwFlgs.o:	${H}/windows.h
gcrShwFlgs.o:	${H}/dbwind.h
gcrShwFlgs.o:	gcr.h
gcrShwFlgs.o:	${H}/doubleint.h
gcrShwFlgs.o:	${H}/heap.h
gcrShwFlgs.o:	${H}/router.h
gcrShwFlgs.o:	${H}/main.h
gcrShwFlgs.o:	${H}/styles.h
gcrShwFlgs.o:	${H}/textio.h
 
# include file dependencies for "gcrUnsplit.c"
gcrUnsplit.o:	gcrUnsplit.c
gcrUnsplit.o:	${H}/magic.h
gcrUnsplit.o:	${H}/geometry.h
gcrUnsplit.o:	${H}/tile.h
gcrUnsplit.o:	gcr.h
gcrUnsplit.o:	${H}/malloc.h
 
