SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 11:40:25 mayo Exp $
#

MODULE=	cmwind
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin


SRCS=	CMWmain.c CMWcmmnds.c CMWundo.c CMWrgbhsv.c
OBJS=	CMWmain.o CMWcmmnds.o CMWundo.o CMWrgbhsv.o
POBJS=	CMWmain.po CMWcmmnds.po CMWundo.po CMWrgbhsv.po
ALLSRCS= ${SRCS} ${MODULE}.h Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=${IFLAGS} -DPARANOID -u
LINTLIBS= ${MAGICDIR}/lib/lint/database.ln ${MAGICDIR}/lib/lint/graphics.ln \
	${MAGICDIR}/lib/lint/textio.ln \
	${MAGICDIR}/lib/lint/tiles.ln ${MAGICDIR}/lib/lint/utils.ln \
	${MAGICDIR}/lib/lint/windows.ln
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

installhdrs: ${H}/${MODULE}.h

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

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

rcp:	${ALLSRCS}
	rcp ${ALLSRCS} ${RHOST}:"~cad/magic/src/${MODULE}"
	rsh ${RHOST} "(cd ~cad/magic/src/${MODULE} ; make -k install)"
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:40:21 PDT 1990.
 
# include file dependencies for "CMWcmmnds.c"
CMWcmmnds.o:	CMWcmmnds.c
CMWcmmnds.o:	${H}/magic.h
CMWcmmnds.o:	${H}/geometry.h
CMWcmmnds.o:	${H}/windows.h
CMWcmmnds.o:	${H}/tile.h
CMWcmmnds.o:	${H}/hash.h
CMWcmmnds.o:	${H}/database.h
CMWcmmnds.o:	${H}/dbwind.h
CMWcmmnds.o:	${H}/main.h
CMWcmmnds.o:	${H}/commands.h
CMWcmmnds.o:	cmwind.h
CMWcmmnds.o:	${H}/graphics.h
CMWcmmnds.o:	${H}/textio.h
CMWcmmnds.o:	${H}/utils.h
CMWcmmnds.o:	${H}/styles.h
CMWcmmnds.o:	${H}/txcommands.h
CMWcmmnds.o:	${H}/undo.h
 
# include file dependencies for "CMWmain.c"
CMWmain.o:	CMWmain.c
CMWmain.o:	${H}/magic.h
CMWmain.o:	${H}/geometry.h
CMWmain.o:	${H}/windows.h
CMWmain.o:	${H}/tile.h
CMWmain.o:	${H}/hash.h
CMWmain.o:	${H}/database.h
CMWmain.o:	${H}/main.h
CMWmain.o:	${H}/commands.h
CMWmain.o:	cmwind.h
CMWmain.o:	${H}/graphicsInt.h
CMWmain.o:	${H}/graphics.h
CMWmain.o:	${H}/textio.h
CMWmain.o:	${H}/utils.h
CMWmain.o:	${H}/styles.h
CMWmain.o:	${H}/glyphs.h
CMWmain.o:	${H}/malloc.h
 
# include file dependencies for "CMWrgbhsv.c"
CMWrgbhsv.o:	CMWrgbhsv.c
 
# include file dependencies for "CMWundo.c"
CMWundo.o:	CMWundo.c
CMWundo.o:	${H}/magic.h
CMWundo.o:	${H}/geometry.h
CMWundo.o:	${H}/windows.h
CMWundo.o:	cmwind.h
CMWundo.o:	${H}/undo.h
 
