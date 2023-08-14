SHELL=/bin/sh
# rcsid $Header: Makefile,v 6.0 90/08/29 11:41:32 mayo Exp $

MODULE= dbwind
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H} -DPARANOID
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

OBJS=	DBWdisplay.o DBWbuttons.o DBWfdback.o DBWhlights.o \
	DBWprocs.o DBWtools.o DBWundo.o
POBJS=	DBWdisplay.po DBWbuttons.po DBWfdback.o DBWhlights.po \
	DBWprocs.po DBWtools.po DBWundo.po
SRCS=	DBWdisplay.c DBWbuttons.c DBWfdback.c DBWhlights.c \
	DBWprocs.c DBWtools.c DBWundo.c
LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln ${LIB}/lint/graphics.ln \
	${LIB}/lint/tech.ln ${LIB}/lint/textio.ln \
	${LIB}/lint/tiles.ln ${LIB}/lint/undo.ln ${LIB}/lint/utils.ln \
	${LIB}/lint/windows.ln

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

dbwind.o: ${OBJS}
	rm -f dbwind.o
	ld -r ${OBJS} -o dbwind.o

${MODULE}.po: ${POBJS}
	rm -f ${MODULE}.po
	ld -r ${POBJS} -o ${MODULE}.po

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

install: installhdrs ${LIB}/dbwind.o ${LIB}/lint/dbwind.ln 

profile: installhdrs ${LIB}/${MODULE}.po ${LIB}/lint/${MODULE}.ln

installhdrs: ${H}/${MODULE}.h ${H}/dbwtech.h

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${H}/dbwtech.h: dbwtech.h
	rm -f ${H}/dbwtech.h 
	cp dbwtech.h ${H}

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -r -x ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/${MODULE}.po: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}


${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

.SUFFIXES: .po

.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:41:26 PDT 1990.
 
# include file dependencies for "DBWbuttons.c"
DBWbuttons.o:	DBWbuttons.c
DBWbuttons.o:	${H}/magic.h
DBWbuttons.o:	${H}/geometry.h
DBWbuttons.o:	${H}/graphics.h
DBWbuttons.o:	${H}/tile.h
DBWbuttons.o:	${H}/hash.h
DBWbuttons.o:	${H}/database.h
DBWbuttons.o:	${H}/windows.h
DBWbuttons.o:	dbwind.h
DBWbuttons.o:	${H}/styles.h
DBWbuttons.o:	${H}/textio.h
DBWbuttons.o:	${H}/txcommands.h
DBWbuttons.o:	${H}/utils.h
 
# include file dependencies for "DBWdisplay.c"
DBWdisplay.o:	DBWdisplay.c
DBWdisplay.o:	${H}/magic.h
DBWdisplay.o:	${H}/geometry.h
DBWdisplay.o:	${H}/graphics.h
DBWdisplay.o:	${H}/tile.h
DBWdisplay.o:	${H}/hash.h
DBWdisplay.o:	${H}/database.h
DBWdisplay.o:	${H}/windows.h
DBWdisplay.o:	dbwind.h
DBWdisplay.o:	${H}/utils.h
DBWdisplay.o:	dbwtech.h
DBWdisplay.o:	${H}/styles.h
DBWdisplay.o:	${H}/main.h
DBWdisplay.o:	${H}/tech.h
DBWdisplay.o:	${H}/signals.h
DBWdisplay.o:	${H}/router.h
 
# include file dependencies for "DBWfdback.c"
DBWfdback.o:	DBWfdback.c
DBWfdback.o:	${H}/magic.h
DBWfdback.o:	${H}/geometry.h
DBWfdback.o:	${H}/graphics.h
DBWfdback.o:	${H}/tile.h
DBWfdback.o:	${H}/hash.h
DBWfdback.o:	${H}/database.h
DBWfdback.o:	${H}/windows.h
DBWfdback.o:	dbwind.h
DBWfdback.o:	${H}/utils.h
DBWfdback.o:	${H}/styles.h
DBWfdback.o:	${H}/malloc.h
DBWfdback.o:	${H}/signals.h
 
# include file dependencies for "DBWhlights.c"
DBWhlights.o:	DBWhlights.c
DBWhlights.o:	${H}/magic.h
DBWhlights.o:	${H}/geometry.h
DBWhlights.o:	${H}/graphics.h
DBWhlights.o:	${H}/styles.h
DBWhlights.o:	${H}/tile.h
DBWhlights.o:	${H}/hash.h
DBWhlights.o:	${H}/database.h
DBWhlights.o:	${H}/windows.h
DBWhlights.o:	dbwind.h
DBWhlights.o:	${H}/textio.h
DBWhlights.o:	${H}/graphicsInt.h
 
# include file dependencies for "DBWprocs.c"
DBWprocs.o:	DBWprocs.c
DBWprocs.o:	${H}/magic.h
DBWprocs.o:	${H}/geometry.h
DBWprocs.o:	${H}/windows.h
DBWprocs.o:	${H}/tile.h
DBWprocs.o:	${H}/hash.h
DBWprocs.o:	${H}/database.h
DBWprocs.o:	${H}/main.h
DBWprocs.o:	${H}/commands.h
DBWprocs.o:	dbwind.h
DBWprocs.o:	${H}/graphics.h
DBWprocs.o:	${H}/textio.h
DBWprocs.o:	${H}/txcommands.h
DBWprocs.o:	${H}/utils.h
DBWprocs.o:	${H}/undo.h
DBWprocs.o:	${H}/glyphs.h
DBWprocs.o:	${H}/malloc.h
DBWprocs.o:	${H}/styles.h
 
# include file dependencies for "DBWtools.c"
DBWtools.o:	DBWtools.c
DBWtools.o:	${H}/magic.h
DBWtools.o:	${H}/geometry.h
DBWtools.o:	${H}/graphics.h
DBWtools.o:	${H}/styles.h
DBWtools.o:	${H}/tile.h
DBWtools.o:	${H}/hash.h
DBWtools.o:	${H}/database.h
DBWtools.o:	${H}/windows.h
DBWtools.o:	dbwind.h
DBWtools.o:	${H}/textio.h
DBWtools.o:	${H}/main.h
DBWtools.o:	${H}/txcommands.h
 
# include file dependencies for "DBWundo.c"
DBWundo.o:	DBWundo.c
DBWundo.o:	${H}/magic.h
DBWundo.o:	${H}/geometry.h
DBWundo.o:	${H}/windows.h
DBWundo.o:	${H}/tile.h
DBWundo.o:	${H}/hash.h
DBWundo.o:	${H}/database.h
DBWundo.o:	${H}/main.h
DBWundo.o:	dbwind.h
DBWundo.o:	${H}/undo.h
DBWundo.o:	${H}/textio.h
DBWundo.o:	${H}/commands.h
 
