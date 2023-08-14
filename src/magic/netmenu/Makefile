SHELL=/bin/sh
#
# rcsid =$Header: Makefile,v 6.0 90/08/29 13:09:54 mayo Exp $
#

MODULE=	netmenu
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin


SRCS=	NMbutton.c NMcmdAK.c NMcmdLZ.c NMlabel.c NMmain.c \
	NMnetlist.c NMshowpt.c NMshowcell.c \
	NMundo.c NMwiring.c
OBJS=	NMbutton.o NMcmdAK.o NMcmdLZ.o NMlabel.o NMmain.o \
	NMnetlist.o NMshowpt.o NMshowcell.o \
	NMundo.o NMwiring.o
POBJS=	NMbutton.po NMcmdAK.po NMcmdLZ.po NMlabel.po NMmain.po \
	NMnetlist.po NMshowpt.po NMshowcell.po \
	NMundo.po NMwiring.po
ALLSRCS= ${SRCS} nmInt.h netmenu.h Makefile
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

IFLAGS=	-I. -I${H}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln ${LIB}/lint/dbwind.ln \
	${LIB}/lint/graphics.ln ${LIB}/lint/textio.ln \
	${LIB}/lint/tiles.ln ${LIB}/lint/undo.ln \
	${LIB}/lint/utils.ln ${LIB}/lint/windows.ln
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

installhdrs: ${H}/${MODULE}.h ${H}/nmInt.h

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

${H}/nmInt.h: nmInt.h
	rm -f ${H}/nmInt.h 
	cp nmInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:09:49 PDT 1990.
 
# include file dependencies for "NMbutton.c"
NMbutton.o:	NMbutton.c
NMbutton.o:	${H}/magic.h
NMbutton.o:	${H}/geometry.h
NMbutton.o:	${H}/graphics.h
NMbutton.o:	${H}/tile.h
NMbutton.o:	${H}/hash.h
NMbutton.o:	${H}/database.h
NMbutton.o:	${H}/windows.h
NMbutton.o:	${H}/dbwind.h
NMbutton.o:	${H}/textio.h
NMbutton.o:	${H}/txcommands.h
NMbutton.o:	nmInt.h
NMbutton.o:	netmenu.h
NMbutton.o:	${H}/styles.h
NMbutton.o:	${H}/main.h
 
# include file dependencies for "NMcmdAK.c"
NMcmdAK.o:	NMcmdAK.c
NMcmdAK.o:	${H}/magic.h
NMcmdAK.o:	${H}/geometry.h
NMcmdAK.o:	${H}/tile.h
NMcmdAK.o:	${H}/hash.h
NMcmdAK.o:	${H}/database.h
NMcmdAK.o:	${H}/windows.h
NMcmdAK.o:	${H}/txcommands.h
NMcmdAK.o:	nmInt.h
NMcmdAK.o:	netmenu.h
NMcmdAK.o:	${H}/main.h
NMcmdAK.o:	${H}/textio.h
NMcmdAK.o:	${H}/malloc.h
 
# include file dependencies for "NMcmdLZ.c"
NMcmdLZ.o:	NMcmdLZ.c
NMcmdLZ.o:	${H}/magic.h
NMcmdLZ.o:	${H}/geometry.h
NMcmdLZ.o:	${H}/tile.h
NMcmdLZ.o:	${H}/hash.h
NMcmdLZ.o:	${H}/database.h
NMcmdLZ.o:	${H}/windows.h
NMcmdLZ.o:	${H}/txcommands.h
NMcmdLZ.o:	nmInt.h
NMcmdLZ.o:	netmenu.h
NMcmdLZ.o:	${H}/dbwind.h
NMcmdLZ.o:	${H}/styles.h
NMcmdLZ.o:	${H}/main.h
NMcmdLZ.o:	${H}/textio.h
 
# include file dependencies for "NMlabel.c"
NMlabel.o:	NMlabel.c
NMlabel.o:	${H}/magic.h
NMlabel.o:	${H}/geometry.h
NMlabel.o:	${H}/utils.h
NMlabel.o:	${H}/tile.h
NMlabel.o:	${H}/hash.h
NMlabel.o:	${H}/database.h
NMlabel.o:	${H}/windows.h
NMlabel.o:	${H}/main.h
NMlabel.o:	${H}/dbwind.h
NMlabel.o:	nmInt.h
NMlabel.o:	${H}/textio.h
NMlabel.o:	${H}/txcommands.h
NMlabel.o:	${H}/commands.h
NMlabel.o:	${H}/styles.h
NMlabel.o:	${H}/malloc.h
 
# include file dependencies for "NMmain.c"
NMmain.o:	NMmain.c
NMmain.o:	${H}/magic.h
NMmain.o:	${H}/geometry.h
NMmain.o:	${H}/windows.h
NMmain.o:	${H}/graphics.h
NMmain.o:	${H}/glyphs.h
NMmain.o:	${H}/tile.h
NMmain.o:	${H}/hash.h
NMmain.o:	${H}/database.h
NMmain.o:	${H}/dbwind.h
NMmain.o:	${H}/textio.h
NMmain.o:	${H}/styles.h
NMmain.o:	${H}/txcommands.h
NMmain.o:	nmInt.h
NMmain.o:	netmenu.h
NMmain.o:	${H}/undo.h
 
# include file dependencies for "NMnetlist.c"
NMnetlist.o:	NMnetlist.c
NMnetlist.o:	${H}/magic.h
NMnetlist.o:	${H}/utils.h
NMnetlist.o:	${H}/geometry.h
NMnetlist.o:	${H}/tile.h
NMnetlist.o:	${H}/hash.h
NMnetlist.o:	${H}/database.h
NMnetlist.o:	${H}/windows.h
NMnetlist.o:	${H}/main.h
NMnetlist.o:	${H}/textio.h
NMnetlist.o:	nmInt.h
NMnetlist.o:	${H}/undo.h
NMnetlist.o:	${H}/malloc.h
NMnetlist.o:	netmenu.h
 
# include file dependencies for "NMshowcell.c"
NMshowcell.o:	NMshowcell.c
NMshowcell.o:	${H}/magic.h
NMshowcell.o:	${H}/geometry.h
NMshowcell.o:	${H}/graphics.h
NMshowcell.o:	${H}/tile.h
NMshowcell.o:	${H}/hash.h
NMshowcell.o:	${H}/database.h
NMshowcell.o:	${H}/windows.h
NMshowcell.o:	${H}/dbwind.h
NMshowcell.o:	${H}/styles.h
NMshowcell.o:	${H}/textio.h
NMshowcell.o:	${H}/main.h
NMshowcell.o:	nmInt.h
 
# include file dependencies for "NMshowpt.c"
NMshowpt.o:	NMshowpt.c
NMshowpt.o:	${H}/magic.h
NMshowpt.o:	${H}/geometry.h
NMshowpt.o:	${H}/graphics.h
NMshowpt.o:	${H}/tile.h
NMshowpt.o:	${H}/hash.h
NMshowpt.o:	${H}/database.h
NMshowpt.o:	${H}/windows.h
NMshowpt.o:	${H}/dbwind.h
NMshowpt.o:	${H}/styles.h
NMshowpt.o:	${H}/main.h
NMshowpt.o:	${H}/malloc.h
 
# include file dependencies for "NMundo.c"
NMundo.o:	NMundo.c
NMundo.o:	${H}/magic.h
NMundo.o:	${H}/geometry.h
NMundo.o:	${H}/windows.h
NMundo.o:	netmenu.h
NMundo.o:	nmInt.h
NMundo.o:	${H}/undo.h
NMundo.o:	${H}/utils.h
 
# include file dependencies for "NMwiring.c"
NMwiring.o:	NMwiring.c
NMwiring.o:	${H}/magic.h
NMwiring.o:	${H}/geometry.h
NMwiring.o:	${H}/tile.h
NMwiring.o:	${H}/hash.h
NMwiring.o:	${H}/database.h
NMwiring.o:	${H}/windows.h
NMwiring.o:	${H}/dbwind.h
NMwiring.o:	${H}/main.h
NMwiring.o:	netmenu.h
NMwiring.o:	nmInt.h
NMwiring.o:	${H}/textio.h
NMwiring.o:	${H}/styles.h
NMwiring.o:	${H}/drc.h
NMwiring.o:	${H}/malloc.h
NMwiring.o:	${H}/router.h
NMwiring.o:	${H}/utils.h
 
