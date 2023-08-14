SHELL=/bin/sh
#
# rcsid="$Header: Makefile,v 6.0 90/08/29 13:11:48 mayo Exp $"
#

MODULE=	signals
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin


SRCS=	${MODULE}.c
OBJS=	${MODULE}.o
POBJS=	${MODULE}.po
ALLSRCS= ${SRCS} ${MODULE}.h Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=${IFLAGS} -u
LINTLIBS= ${LIB}/lint/textio.ln ${LIB}/lint/utils.ln ${LIB}/lint/lintfix.c

.SUFFIXES: .po

${MODULE}.o: ${MODULE}.h

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

 
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:11:45 PDT 1990.
 
# include file dependencies for "signals.c"
signals.o:	signals.c
signals.o:	${H}/magic.h
signals.o:	${H}/textio.h
signals.o:	${H}/geometry.h
signals.o:	signals.h
signals.o:	${H}/graphics.h
 
# include file dependencies for "test.c"
test.o:	test.c
 
