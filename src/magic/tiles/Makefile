SHELL=/bin/sh
#
# Makefile
# rcsid "$Header: Makefile,v 6.0 90/08/29 11:39:19 mayo Exp $"
#
MODULE=	tiles
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

OBJS=	tile.o search.o search2.o
POBJS=	tile.po search.po search2.po
SRCS=	tile.c search.c search2.c
LINTFLAGS=-I. -I${H} -u
LINTLIBS=${LIB}/lint/lintfix.c
ALLSRCS= ${SRCS} Makefile test.c tile.h

.SUFFIXES: .po

tiles.o: ${OBJS}
	rm -f tiles.o
	ld -r ${OBJS} -o tiles.o

tiles.po: ${POBJS}
	rm -f tiles.po
	ld -r ${POBJS} -o tiles.po

.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

profile: tiles.po
	rm -f ${LIB}/tiles.po
	cp tiles.po ${LIB}/tiles.po

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

install: installhdrs ${LIB}/tiles.o ${LIB}/lint/tiles.ln

installhdrs: ${H}/tile.h

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -x -r tiles.o -o ${LIB}/tiles.o
	../:instclean ${MODULE} ${SRCS} Makefile

${H}/tile.h: tile.h
	rm -f ${H}/tile.h 
	cp tile.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS} 

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:39:15 PDT 1990.
 
# include file dependencies for "search.c"
search.o:	search.c
search.o:	${H}/magic.h
search.o:	${H}/geometry.h
search.o:	tile.h
 
# include file dependencies for "search2.c"
search2.o:	search2.c
search2.o:	${H}/magic.h
search2.o:	${H}/geometry.h
search2.o:	tile.h
search2.o:	${H}/signals.h
 
# include file dependencies for "tile.c"
tile.o:	tile.c
tile.o:	${H}/magic.h
tile.o:	${H}/malloc.h
tile.o:	${H}/geometry.h
tile.o:	tile.h
 
