SHELL=/bin/sh
#
# Makefile	rcsid $Header: Makefile,v 6.0 90/08/29 11:41:15 mayo Exp $
#
MODULE=database
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}
RANLIB=`../:ranlib`


BIN=	${MAGICDIR}/bin

OBJS=	DBbound.o DBcell.o DBcellbox.o DBcellcopy.o \
	DBcellname.o DBcellsrch.o DBcellsel.o DBcellsubr.o \
	DBconnect.o DBcount.o DBexpand.o DBio.o DBlabel.o DBlabel2.o \
	DBpaint2.o DBpaint.o DBprop.o \
	DBtech.o DBtcontact.o DBtechname.o DBtpaint.o \
	DBtpaint2.o DBtechtype.o DBterminal.o \
	DBtiles.o DBtimestmp.o DBundo.o
LOBJS=	DBmain.o ../tiles/tiles.o ../tech/tech.o

POBJS=	DBbound.po DBcell.po DBcellbox.po DBcellcopy.po \
	DBcellname.po DBcellsrch.po DBcellsel.po DBcellsubr.po \
	DBconnect.po DBcount.po DBexpand.po DBio.po DBlabel.po DBlabel2.po \
	DBpaint2.po DBpaint.po DBprop.po \
	DBtech.po DBtcontact.po DBtechname.po DBtpaint.po \
	DBtpaint2.po DBtechtype.po DBterminal.po \
	DBtiles.po DBtimestmp.po DBundo.po
LPOBJS=	DBmain.po ../tiles/tiles.po ../tech/tech.po

LPSRCS=	DBmain.c
SRCS=	DBbound.c DBcell.c DBcellbox.c DBcellcopy.c \
	DBcellname.c DBcellsrch.c DBcellsel.c DBcellsubr.c \
	DBconnect.c DBcount.c DBexpand.c DBio.c DBlabel.c DBlabel2.c \
	DBpaint2.c DBpaint.c DBprop.c \
	DBtech.c DBtcontact.c DBtechname.c DBtpaint.c \
	DBtpaint2.c DBtechtype.c DBterminal.c \
	DBtiles.c DBtimestmp.c DBundo.c ${LPSRCS}

HDRS=	database.h databaseInt.h
LINTFLAGS=${IFLAGS} -u
LINTLIBS=${LIB}/lint/utils.ln ${LIB}/lint/tiles.ln ${LIB}/lint/textio.ln \
	${LIB}/lint/windows.ln ${LIB}/lint/drc.ln \
	${LIB}/lint/dbwind.ln ${LIB}/lint/tech.ln
ALLSRCS= ${SRCS} ${HDRS} Makefile

.SUFFIXES: .po

database.o: ${HDRS} ${OBJS}
	rm -f database.o
	ld -r ${OBJS} -o database.o

database.po: ${HDRS} ${POBJS}
	rm -f database.po
	ld -r ${POBJS} -o database.po

libmagicdb.a: ${OBJS} ${LOBJS}
	-ar rv libmagicdb.a ${OBJS} ${LOBJS}
	-${RANLIB} libmagicdb.a

libmagicdb_p.a: ${POBJS} ${LPOBJS}
	-ar rv libmagicdb_p.a ${POBJS} ${LPOBJS}
	-${RANLIB} libmagicdb_p.a

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

tags: ${HDRS} ${SRCS}
	ctags ${HDRS} ${SRCS}

.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

profile: installhdrs ${LIB}/database.po ${LIB}/lint/database.ln \
	${LIB}/libmagicdb_p.a

install: installhdrs ${LIB}/database.o ${LIB}/lint/database.ln \
	${LIB}/libmagicdb.a

installhdrs: ${H}/${MODULE}.h ${H}/databaseInt.h

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	cp ${MODULE}.o ${LIB}
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/database.po: database.po
	rm -f ${LIB}/database.po
	cp database.po ${LIB}

${LIB}/libmagicdb.a: libmagicdb.a
	rm -f ${LIB}/libmagicdb.a
	cp libmagicdb.a ${LIB}/libmagicdb.a
	${RANLIB} ${LIB}/libmagicdb.a

${LIB}/libmagicdb_p.a: libmagicdb_p.a
	rm -f ${LIB}/libmagicdb_p.a
	cp libmagicdb_p.a ${LIB}/libmagicdb_p.a
	${RANLIB} ${LIB}/libmagicdb_p.a

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${H}/databaseInt.h: databaseInt.h
	rm -f ${H}/databaseInt.h 
	cp databaseInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:40:54 PDT 1990.
 
# include file dependencies for "DBbound.c"
DBbound.o:	DBbound.c
DBbound.o:	${H}/magic.h
DBbound.o:	${H}/geometry.h
DBbound.o:	${H}/tile.h
 
# include file dependencies for "DBcell.c"
DBcell.o:	DBcell.c
DBcell.o:	${H}/magic.h
DBcell.o:	${H}/malloc.h
DBcell.o:	${H}/geometry.h
DBcell.o:	${H}/tile.h
DBcell.o:	${H}/hash.h
DBcell.o:	database.h
DBcell.o:	databaseInt.h
DBcell.o:	${H}/undo.h
DBcell.o:	${H}/signals.h
 
# include file dependencies for "DBcellbox.c"
DBcellbox.o:	DBcellbox.c
DBcellbox.o:	${H}/magic.h
DBcellbox.o:	${H}/geometry.h
DBcellbox.o:	${H}/geofast.h
DBcellbox.o:	${H}/tile.h
DBcellbox.o:	${H}/hash.h
DBcellbox.o:	database.h
DBcellbox.o:	databaseInt.h
DBcellbox.o:	${H}/windows.h
DBcellbox.o:	${H}/dbwind.h
DBcellbox.o:	${H}/undo.h
 
# include file dependencies for "DBcellcopy.c"
DBcellcopy.o:	DBcellcopy.c
DBcellcopy.o:	${H}/magic.h
DBcellcopy.o:	${H}/geometry.h
DBcellcopy.o:	${H}/geofast.h
DBcellcopy.o:	${H}/malloc.h
DBcellcopy.o:	${H}/tile.h
DBcellcopy.o:	${H}/hash.h
DBcellcopy.o:	database.h
DBcellcopy.o:	databaseInt.h
DBcellcopy.o:	${H}/textio.h
DBcellcopy.o:	${H}/windows.h
DBcellcopy.o:	${H}/dbwind.h
DBcellcopy.o:	${H}/commands.h
 
# include file dependencies for "DBcellname.c"
DBcellname.o:	DBcellname.c
DBcellname.o:	${H}/magic.h
DBcellname.o:	${H}/hash.h
DBcellname.o:	${H}/utils.h
DBcellname.o:	${H}/geometry.h
DBcellname.o:	${H}/tile.h
DBcellname.o:	database.h
DBcellname.o:	databaseInt.h
DBcellname.o:	${H}/signals.h
DBcellname.o:	${H}/undo.h
DBcellname.o:	${H}/malloc.h
DBcellname.o:	${H}/windows.h
DBcellname.o:	${H}/textio.h
DBcellname.o:	${H}/main.h
 
# include file dependencies for "DBcellsel.c"
DBcellsel.o:	DBcellsel.c
DBcellsel.o:	${H}/magic.h
DBcellsel.o:	${H}/geometry.h
DBcellsel.o:	${H}/tile.h
DBcellsel.o:	${H}/hash.h
DBcellsel.o:	database.h
DBcellsel.o:	databaseInt.h
DBcellsel.o:	${H}/utils.h
DBcellsel.o:	${H}/doubleint.h
 
# include file dependencies for "DBcellsrch.c"
DBcellsrch.o:	DBcellsrch.c
DBcellsrch.o:	${H}/magic.h
DBcellsrch.o:	${H}/geometry.h
DBcellsrch.o:	${H}/geofast.h
DBcellsrch.o:	${H}/tile.h
DBcellsrch.o:	${H}/hash.h
DBcellsrch.o:	database.h
DBcellsrch.o:	databaseInt.h
DBcellsrch.o:	${H}/textio.h
DBcellsrch.o:	${H}/signals.h
 
# include file dependencies for "DBcellsubr.c"
DBcellsubr.o:	DBcellsubr.c
DBcellsubr.o:	${H}/magic.h
DBcellsubr.o:	${H}/malloc.h
DBcellsubr.o:	${H}/hash.h
DBcellsubr.o:	${H}/utils.h
DBcellsubr.o:	${H}/geometry.h
DBcellsubr.o:	${H}/tile.h
DBcellsubr.o:	database.h
DBcellsubr.o:	databaseInt.h
DBcellsubr.o:	${H}/signals.h
 
# include file dependencies for "DBconnect.c"
DBconnect.o:	DBconnect.c
DBconnect.o:	${H}/magic.h
DBconnect.o:	${H}/geometry.h
DBconnect.o:	${H}/tile.h
DBconnect.o:	${H}/hash.h
DBconnect.o:	database.h
DBconnect.o:	databaseInt.h
DBconnect.o:	${H}/signals.h
DBconnect.o:	${H}/malloc.h
 
# include file dependencies for "DBcount.c"
DBcount.o:	DBcount.c
DBcount.o:	${H}/magic.h
DBcount.o:	${H}/hash.h
DBcount.o:	${H}/geometry.h
DBcount.o:	${H}/tile.h
DBcount.o:	database.h
DBcount.o:	databaseInt.h
 
# include file dependencies for "DBexpand.c"
DBexpand.o:	DBexpand.c
DBexpand.o:	${H}/magic.h
DBexpand.o:	${H}/geometry.h
DBexpand.o:	${H}/tile.h
DBexpand.o:	${H}/hash.h
DBexpand.o:	database.h
DBexpand.o:	databaseInt.h
DBexpand.o:	${H}/textio.h
DBexpand.o:	${H}/utils.h
DBexpand.o:	${H}/stack.h
 
# include file dependencies for "DBio.c"
DBio.o:	DBio.c
DBio.o:	${H}/magic.h
DBio.o:	${H}/geometry.h
DBio.o:	${H}/tile.h
DBio.o:	${H}/utils.h
DBio.o:	${H}/hash.h
DBio.o:	database.h
DBio.o:	databaseInt.h
DBio.o:	${H}/tech.h
DBio.o:	${H}/textio.h
DBio.o:	${H}/drc.h
DBio.o:	${H}/undo.h
DBio.o:	${H}/malloc.h
DBio.o:	${H}/signals.h
 
# include file dependencies for "DBlabel.c"
DBlabel.o:	DBlabel.c
DBlabel.o:	${H}/magic.h
DBlabel.o:	${H}/malloc.h
DBlabel.o:	${H}/geometry.h
DBlabel.o:	${H}/tile.h
DBlabel.o:	${H}/hash.h
DBlabel.o:	database.h
DBlabel.o:	databaseInt.h
DBlabel.o:	${H}/windows.h
DBlabel.o:	${H}/dbwind.h
DBlabel.o:	${H}/commands.h
DBlabel.o:	${H}/textio.h
 
# include file dependencies for "DBlabel2.c"
DBlabel2.o:	DBlabel2.c
DBlabel2.o:	${H}/magic.h
DBlabel2.o:	${H}/geometry.h
DBlabel2.o:	${H}/tile.h
DBlabel2.o:	${H}/hash.h
DBlabel2.o:	database.h
DBlabel2.o:	databaseInt.h
DBlabel2.o:	${H}/malloc.h
 
# include file dependencies for "DBmain.c"
DBmain.o:	DBmain.c
DBmain.o:	${H}/magic.h
DBmain.o:	${H}/geometry.h
DBmain.o:	${H}/utils.h
DBmain.o:	${H}/tech.h
DBmain.o:	${H}/hash.h
DBmain.o:	${H}/tile.h
DBmain.o:	database.h
DBmain.o:	${H}/undo.h
DBmain.o:	${H}/signals.h
 
# include file dependencies for "DBpaint.c"
DBpaint.o:	DBpaint.c
DBpaint.o:	${H}/magic.h
DBpaint.o:	${H}/malloc.h
DBpaint.o:	${H}/geometry.h
DBpaint.o:	${H}/tile.h
DBpaint.o:	${H}/hash.h
DBpaint.o:	database.h
DBpaint.o:	databaseInt.h
DBpaint.o:	${H}/graphics.h
DBpaint.o:	${H}/windows.h
DBpaint.o:	${H}/dbwind.h
DBpaint.o:	${H}/signals.h
DBpaint.o:	${H}/textio.h
DBpaint.o:	${H}/undo.h
DBpaint.o:	${H}/styles.h
 
# include file dependencies for "DBpaint2.c"
DBpaint2.o:	DBpaint2.c
DBpaint2.o:	${H}/magic.h
DBpaint2.o:	${H}/geometry.h
DBpaint2.o:	${H}/tile.h
DBpaint2.o:	${H}/hash.h
DBpaint2.o:	database.h
DBpaint2.o:	databaseInt.h
 
# include file dependencies for "DBprop.c"
DBprop.o:	DBprop.c
DBprop.o:	${H}/magic.h
DBprop.o:	${H}/geometry.h
DBprop.o:	${H}/tile.h
DBprop.o:	${H}/hash.h
DBprop.o:	database.h
DBprop.o:	${H}/malloc.h
 
# include file dependencies for "DBtcontact.c"
DBtcontact.o:	DBtcontact.c
DBtcontact.o:	${H}/magic.h
DBtcontact.o:	${H}/geometry.h
DBtcontact.o:	${H}/utils.h
DBtcontact.o:	${H}/tile.h
DBtcontact.o:	${H}/hash.h
DBtcontact.o:	database.h
DBtcontact.o:	databaseInt.h
DBtcontact.o:	${H}/tech.h
DBtcontact.o:	${H}/textio.h
 
# include file dependencies for "DBtech.c"
DBtech.o:	DBtech.c
DBtech.o:	${H}/magic.h
DBtech.o:	${H}/geometry.h
DBtech.o:	${H}/utils.h
DBtech.o:	${H}/tile.h
DBtech.o:	${H}/hash.h
DBtech.o:	database.h
DBtech.o:	databaseInt.h
DBtech.o:	${H}/tech.h
DBtech.o:	${H}/textio.h
DBtech.o:	${H}/malloc.h
 
# include file dependencies for "DBtechname.c"
DBtechname.o:	DBtechname.c
DBtechname.o:	${H}/magic.h
DBtechname.o:	${H}/geometry.h
DBtechname.o:	${H}/utils.h
DBtechname.o:	${H}/tile.h
DBtechname.o:	${H}/hash.h
DBtechname.o:	database.h
DBtechname.o:	databaseInt.h
DBtechname.o:	${H}/tech.h
DBtechname.o:	${H}/textio.h
DBtechname.o:	${H}/malloc.h
 
# include file dependencies for "DBtechtype.c"
DBtechtype.o:	DBtechtype.c
DBtechtype.o:	${H}/magic.h
DBtechtype.o:	${H}/geometry.h
DBtechtype.o:	${H}/utils.h
DBtechtype.o:	${H}/tile.h
DBtechtype.o:	${H}/hash.h
DBtechtype.o:	database.h
DBtechtype.o:	databaseInt.h
DBtechtype.o:	${H}/tech.h
DBtechtype.o:	${H}/textio.h
DBtechtype.o:	${H}/malloc.h
 
# include file dependencies for "DBterminal.c"
DBterminal.o:	DBterminal.c
DBterminal.o:	${H}/magic.h
DBterminal.o:	${H}/geometry.h
DBterminal.o:	${H}/tile.h
DBterminal.o:	${H}/hash.h
DBterminal.o:	database.h
DBterminal.o:	databaseInt.h
DBterminal.o:	${H}/tech.h
DBterminal.o:	${H}/textio.h
 
# include file dependencies for "DBtiles.c"
DBtiles.o:	DBtiles.c
DBtiles.o:	${H}/magic.h
DBtiles.o:	${H}/geometry.h
DBtiles.o:	${H}/tile.h
DBtiles.o:	${H}/signals.h
DBtiles.o:	${H}/hash.h
DBtiles.o:	database.h
DBtiles.o:	databaseInt.h
DBtiles.o:	${H}/malloc.h
 
# include file dependencies for "DBtimestmp.c"
DBtimestmp.o:	DBtimestmp.c
DBtimestmp.o:	${H}/magic.h
DBtimestmp.o:	${H}/geometry.h
DBtimestmp.o:	${H}/tile.h
DBtimestmp.o:	${H}/hash.h
DBtimestmp.o:	database.h
DBtimestmp.o:	databaseInt.h
DBtimestmp.o:	${H}/windows.h
DBtimestmp.o:	${H}/textio.h
DBtimestmp.o:	${H}/drc.h
DBtimestmp.o:	${H}/signals.h
DBtimestmp.o:	${H}/malloc.h
 
# include file dependencies for "DBtpaint.c"
DBtpaint.o:	DBtpaint.c
DBtpaint.o:	${H}/magic.h
DBtpaint.o:	${H}/geometry.h
DBtpaint.o:	${H}/utils.h
DBtpaint.o:	${H}/tile.h
DBtpaint.o:	${H}/hash.h
DBtpaint.o:	database.h
DBtpaint.o:	databaseInt.h
DBtpaint.o:	${H}/tech.h
DBtpaint.o:	${H}/textio.h
 
# include file dependencies for "DBtpaint2.c"
DBtpaint2.o:	DBtpaint2.c
DBtpaint2.o:	${H}/magic.h
DBtpaint2.o:	${H}/geometry.h
DBtpaint2.o:	${H}/utils.h
DBtpaint2.o:	${H}/tile.h
DBtpaint2.o:	${H}/hash.h
DBtpaint2.o:	database.h
DBtpaint2.o:	databaseInt.h
DBtpaint2.o:	${H}/tech.h
DBtpaint2.o:	${H}/textio.h
 
# include file dependencies for "DBundo.c"
DBundo.o:	DBundo.c
DBundo.o:	${H}/magic.h
DBundo.o:	${H}/malloc.h
DBundo.o:	${H}/geometry.h
DBundo.o:	${H}/tile.h
DBundo.o:	${H}/hash.h
DBundo.o:	database.h
DBundo.o:	databaseInt.h
DBundo.o:	${H}/undo.h
DBundo.o:	${H}/windows.h
DBundo.o:	${H}/dbwind.h
DBundo.o:	${H}/main.h
DBundo.o:	${H}/utils.h
DBundo.o:	${H}/drc.h
 
