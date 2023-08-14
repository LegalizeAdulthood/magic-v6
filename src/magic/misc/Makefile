SHELL=/bin/sh
#
# rcsid="$Header: Makefile,v 6.0 90/08/29 13:09:10 mayo Exp $"
#

MODULE=	misc
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin
SRCS=	niceabort.c
OBJS=	niceabort.o
POBJS=	niceabort.po
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=${IFLAGS} -u
LINTLIBS=

${MODULE}.o:	${OBJS}
	rm -f ${MODULE}.o
	ld -r ${OBJS} -o ${MODULE}.o

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln

installhdrs: ${H}/magic.h ${H}/styles.h ${H}/paths.h

${H}/magic.h: magic.h
	rm -f ${H}/magic.h 
	cp magic.h ${H}

${H}/styles.h: styles.h
	rm -f ${H}/styles.h 
	cp styles.h ${H}

${H}/paths.h: paths.h
	rm -f ${H}/paths.h 
	cp paths.h ${H}

$(LIB)/lint/lintfix.c: lintfix.c
	rm -f $(LIB)/lint/lintfix.c
	cp lintfix.c $(LIB)/lint/lintfix.c

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	cp ${MODULE}.o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

.SUFFIXES: .po

.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

.c.o:
	rm -f $*.o
	$(CC) -c $(CFLAGS) $*.c

profile: ${LIB}/${MODULE}.po ${LIB}/lint/${MODULE}.ln

${LIB}/${MODULE}.po: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}/${MODULE}.po

${MODULE}.po:	${POBJS}
	rm -f ${MODULE}.po
	ld -r ${POBJS} -o ${MODULE}.po
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:09:06 PDT 1990.
 
# include file dependencies for "lintfix.c"
lintfix.o:	lintfix.c
 
# include file dependencies for "niceabort.c"
niceabort.o:	niceabort.c
niceabort.o:	magic.h
niceabort.o:	${H}/textio.h
niceabort.o:	${H}/utils.h
niceabort.o:	${H}/geometry.h
niceabort.o:	${H}/tile.h
niceabort.o:	${H}/hash.h
niceabort.o:	${H}/database.h
niceabort.o:	${H}/windows.h
niceabort.o:	${H}/main.h
 
