SHELL=/bin/sh
#
# rcsid "$Header: Makefile,v 6.0 90/08/29 11:41:58 mayo Exp $"
#
MODULE=	drc
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

OBJS=	DRCarray.o DRCbasic.o DRCcontin.o DRCmain.o\
	DRCsubcell.o DRCtech.o DRCprint.o
POBJS=	DRCarray.po DRCbasic.po DRCcontin.po DRCmain.po\
	DRCsubcell.po DRCtech.po DRCprint.po
SRCS=	DRCarray.c DRCbasic.c DRCcontin.c DRCmain.c\
	DRCsubcell.c DRCtech.c DRCprint.c
HDRS=	drc.h
LINTFLAGS=${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln \
	${LIB}/lint/dbwind.ln ${LIB}/lint/signals.ln \
	${LIB}/lint/tech.ln ${LIB}/lint/textio.ln ${LIB}/lint/tiles.ln \
	${LIB}/lint/undo.ln ${LIB}/lint/utils.ln ${LIB}/lint/windows.ln
ALLSRCS= ${SRCS} Makefile

.SUFFIXES: .po

drc.o: ${OBJS}
	rm -f drc.o
	ld -r ${OBJS} -o drc.o

drc.po: ${POBJS}
	rm -f drc.po
	ld -r ${POBJS} -o drc.po

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

install: installhdrs ${LIB}/drc.o ${LIB}/lint/drc.ln

installhdrs: ${H}/${MODULE}.h

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -x -r drc.o -o ${LIB}/drc.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

profile: drc.po
	rm -f ${LIB}/drc.po
	cp drc.po ${LIB}/drc.po

tags:
	ctags ${SRCS} ${H}database.h \
		../tiles/*.c ../utils/*.c ../geometry/*.c \
		${H}/magic.h ${H}/geometry.h ${H}/graphics.h \
		${H}/hash.h ${H}/stack.h ${H}/tech.h ${H}/tile.h ${H}/utils.h
 
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:41:54 PDT 1990.
 
# include file dependencies for "DRCarray.c"
DRCarray.o:	DRCarray.c
DRCarray.o:	${H}/magic.h
DRCarray.o:	${H}/geometry.h
DRCarray.o:	${H}/tile.h
DRCarray.o:	${H}/hash.h
DRCarray.o:	${H}/database.h
DRCarray.o:	drc.h
DRCarray.o:	${H}/windows.h
DRCarray.o:	${H}/commands.h
 
# include file dependencies for "DRCbasic.c"
DRCbasic.o:	DRCbasic.c
DRCbasic.o:	${H}/magic.h
DRCbasic.o:	${H}/geometry.h
DRCbasic.o:	${H}/tile.h
DRCbasic.o:	${H}/hash.h
DRCbasic.o:	${H}/database.h
DRCbasic.o:	drc.h
DRCbasic.o:	${H}/signals.h
 
# include file dependencies for "DRCcontin.c"
DRCcontin.o:	DRCcontin.c
DRCcontin.o:	${H}/magic.h
DRCcontin.o:	${H}/textio.h
DRCcontin.o:	${H}/geometry.h
DRCcontin.o:	${H}/tile.h
DRCcontin.o:	${H}/hash.h
DRCcontin.o:	${H}/database.h
DRCcontin.o:	${H}/windows.h
DRCcontin.o:	${H}/dbwind.h
DRCcontin.o:	${H}/dbwtech.h
DRCcontin.o:	${H}/main.h
DRCcontin.o:	${H}/commands.h
DRCcontin.o:	drc.h
DRCcontin.o:	${H}/signals.h
DRCcontin.o:	${H}/undo.h
DRCcontin.o:	${H}/malloc.h
 
# include file dependencies for "DRCmain.c"
DRCmain.o:	DRCmain.c
DRCmain.o:	${H}/magic.h
DRCmain.o:	${H}/textio.h
DRCmain.o:	${H}/geometry.h
DRCmain.o:	${H}/tile.h
DRCmain.o:	${H}/hash.h
DRCmain.o:	${H}/database.h
DRCmain.o:	${H}/windows.h
DRCmain.o:	${H}/dbwind.h
DRCmain.o:	drc.h
DRCmain.o:	${H}/undo.h
 
# include file dependencies for "DRCprint.c"
DRCprint.o:	DRCprint.c
DRCprint.o:	${H}/magic.h
DRCprint.o:	${H}/geometry.h
DRCprint.o:	${H}/tile.h
DRCprint.o:	${H}/hash.h
DRCprint.o:	${H}/database.h
DRCprint.o:	drc.h
 
# include file dependencies for "DRCsubcell.c"
DRCsubcell.o:	DRCsubcell.c
DRCsubcell.o:	${H}/magic.h
DRCsubcell.o:	${H}/textio.h
DRCsubcell.o:	${H}/geometry.h
DRCsubcell.o:	${H}/tile.h
DRCsubcell.o:	${H}/hash.h
DRCsubcell.o:	${H}/database.h
DRCsubcell.o:	drc.h
DRCsubcell.o:	${H}/windows.h
DRCsubcell.o:	${H}/commands.h
DRCsubcell.o:	${H}/undo.h
 
# include file dependencies for "DRCtech.c"
DRCtech.o:	DRCtech.c
DRCtech.o:	${H}/magic.h
DRCtech.o:	${H}/geometry.h
DRCtech.o:	${H}/utils.h
DRCtech.o:	${H}/tile.h
DRCtech.o:	${H}/hash.h
DRCtech.o:	${H}/database.h
DRCtech.o:	drc.h
DRCtech.o:	${H}/tech.h
DRCtech.o:	${H}/textio.h
DRCtech.o:	${H}/malloc.h
 
