SHELL=/bin/sh
#
# rcsid "$Header: Makefile,v 6.0 90/08/29 11:42:35 mayo Exp $"
#

MODULE=	extract
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	ExtArray.c ExtBasic.c ExtCell.c ExtCouple.c ExtHard.c ExtHier.c \
	ExtLength.c ExtMain.c ExtNghbors.c ExtPerim.c ExtRegion.c \
	ExtSubtree.c ExtTech.c ExtTest.c ExtTimes.c ExtYank.c \
	ExtInter.c ExtUnique.c
OBJS=	ExtArray.o ExtBasic.o ExtCell.o ExtCouple.o ExtHard.o ExtHier.o \
	ExtLength.o ExtMain.o ExtNghbors.o ExtPerim.o ExtRegion.o \
	ExtSubtree.o ExtTech.o ExtTest.o ExtTimes.o ExtYank.o \
	ExtInter.o ExtUnique.o
POBJS=	ExtArray.po ExtBasic.po ExtCell.po ExtCouple.po ExtHard.po ExtHier.po \
	ExtLength.po ExtMain.po ExtNghbors.po ExtPerim.po ExtRegion.po \
	ExtSubtree.po ExtTech.po ExtTest.po ExtTimes.po ExtYank.po \
	ExtInter.po ExtUnique.po
ALLSRCS= ${SRCS} extract.h extDebugInt.h extractInt.h Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln ${LIB}/lint/textio.ln \
	${LIB}/lint/tiles.ln ${LIB}/lint/utils.ln ${LIB}/lint/tech.ln
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

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

profile: installhdrs ${LIB}/${MODULE}.po

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln

installhdrs: ${H}/${MODULE}.h ${H}/extractInt.h ${H}/extDebugInt.h

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

${H}/extractInt.h: extractInt.h
	rm -f ${H}/extractInt.h 
	cp extractInt.h ${H}

${H}/extDebugInt.h: extDebugInt.h
	rm -f ${H}/extDebugInt.h 
	cp extDebugInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:42:22 PDT 1990.
 
# include file dependencies for "ExtArray.c"
ExtArray.o:	ExtArray.c
ExtArray.o:	${H}/magic.h
ExtArray.o:	${H}/geometry.h
ExtArray.o:	${H}/geofast.h
ExtArray.o:	${H}/tile.h
ExtArray.o:	${H}/hash.h
ExtArray.o:	${H}/database.h
ExtArray.o:	${H}/malloc.h
ExtArray.o:	${H}/textio.h
ExtArray.o:	${H}/debug.h
ExtArray.o:	extract.h
ExtArray.o:	extractInt.h
ExtArray.o:	${H}/signals.h
ExtArray.o:	${H}/styles.h
ExtArray.o:	${H}/windows.h
ExtArray.o:	${H}/dbwind.h
 
# include file dependencies for "ExtBasic.c"
ExtBasic.o:	ExtBasic.c
ExtBasic.o:	${H}/magic.h
ExtBasic.o:	${H}/geometry.h
ExtBasic.o:	${H}/geofast.h
ExtBasic.o:	${H}/tile.h
ExtBasic.o:	${H}/hash.h
ExtBasic.o:	${H}/database.h
ExtBasic.o:	${H}/malloc.h
ExtBasic.o:	${H}/textio.h
ExtBasic.o:	${H}/debug.h
ExtBasic.o:	extract.h
ExtBasic.o:	extractInt.h
ExtBasic.o:	${H}/signals.h
ExtBasic.o:	${H}/windows.h
ExtBasic.o:	${H}/dbwind.h
ExtBasic.o:	${H}/styles.h
ExtBasic.o:	${H}/stack.h
 
# include file dependencies for "ExtCell.c"
ExtCell.o:	ExtCell.c
ExtCell.o:	${H}/magic.h
ExtCell.o:	${H}/geometry.h
ExtCell.o:	${H}/styles.h
ExtCell.o:	${H}/tile.h
ExtCell.o:	${H}/hash.h
ExtCell.o:	${H}/database.h
ExtCell.o:	${H}/malloc.h
ExtCell.o:	${H}/textio.h
ExtCell.o:	${H}/debug.h
ExtCell.o:	extract.h
ExtCell.o:	extractInt.h
ExtCell.o:	${H}/signals.h
ExtCell.o:	${H}/stack.h
ExtCell.o:	${H}/utils.h
ExtCell.o:	${H}/windows.h
ExtCell.o:	${H}/main.h
ExtCell.o:	${H}/undo.h
 
# include file dependencies for "ExtCouple.c"
ExtCouple.o:	ExtCouple.c
ExtCouple.o:	${H}/magic.h
ExtCouple.o:	${H}/geometry.h
ExtCouple.o:	${H}/geofast.h
ExtCouple.o:	${H}/tile.h
ExtCouple.o:	${H}/hash.h
ExtCouple.o:	${H}/database.h
ExtCouple.o:	extract.h
ExtCouple.o:	extractInt.h
 
# include file dependencies for "ExtHard.c"
ExtHard.o:	ExtHard.c
ExtHard.o:	${H}/magic.h
ExtHard.o:	${H}/geometry.h
ExtHard.o:	${H}/tile.h
ExtHard.o:	${H}/hash.h
ExtHard.o:	${H}/database.h
ExtHard.o:	${H}/malloc.h
ExtHard.o:	${H}/textio.h
ExtHard.o:	${H}/styles.h
ExtHard.o:	${H}/debug.h
ExtHard.o:	extract.h
ExtHard.o:	extractInt.h
ExtHard.o:	${H}/geofast.h
 
# include file dependencies for "ExtHier.c"
ExtHier.o:	ExtHier.c
ExtHier.o:	${H}/magic.h
ExtHier.o:	${H}/geometry.h
ExtHier.o:	${H}/geofast.h
ExtHier.o:	${H}/tile.h
ExtHier.o:	${H}/hash.h
ExtHier.o:	${H}/database.h
ExtHier.o:	${H}/malloc.h
ExtHier.o:	${H}/textio.h
ExtHier.o:	${H}/styles.h
ExtHier.o:	${H}/windows.h
ExtHier.o:	${H}/dbwind.h
ExtHier.o:	${H}/debug.h
ExtHier.o:	extract.h
ExtHier.o:	extractInt.h
 
# include file dependencies for "ExtInter.c"
ExtInter.o:	ExtInter.c
ExtInter.o:	${H}/magic.h
ExtInter.o:	${H}/geometry.h
ExtInter.o:	${H}/geofast.h
ExtInter.o:	${H}/undo.h
ExtInter.o:	${H}/tile.h
ExtInter.o:	${H}/hash.h
ExtInter.o:	${H}/database.h
ExtInter.o:	${H}/malloc.h
ExtInter.o:	${H}/textio.h
ExtInter.o:	${H}/debug.h
ExtInter.o:	extract.h
ExtInter.o:	extractInt.h
ExtInter.o:	${H}/signals.h
ExtInter.o:	${H}/styles.h
 
# include file dependencies for "ExtLength.c"
ExtLength.o:	ExtLength.c
ExtLength.o:	${H}/magic.h
ExtLength.o:	${H}/geometry.h
ExtLength.o:	${H}/geofast.h
ExtLength.o:	${H}/tile.h
ExtLength.o:	${H}/hash.h
ExtLength.o:	${H}/database.h
ExtLength.o:	${H}/malloc.h
ExtLength.o:	${H}/textio.h
ExtLength.o:	${H}/debug.h
ExtLength.o:	extract.h
ExtLength.o:	extractInt.h
ExtLength.o:	${H}/signals.h
ExtLength.o:	${H}/windows.h
ExtLength.o:	${H}/dbwind.h
ExtLength.o:	${H}/styles.h
ExtLength.o:	${H}/stack.h
ExtLength.o:	${H}/main.h
ExtLength.o:	${H}/utils.h
 
# include file dependencies for "ExtMain.c"
ExtMain.o:	ExtMain.c
ExtMain.o:	${H}/magic.h
ExtMain.o:	${H}/geometry.h
ExtMain.o:	${H}/styles.h
ExtMain.o:	${H}/tile.h
ExtMain.o:	${H}/hash.h
ExtMain.o:	${H}/database.h
ExtMain.o:	${H}/malloc.h
ExtMain.o:	${H}/textio.h
ExtMain.o:	${H}/debug.h
ExtMain.o:	extract.h
ExtMain.o:	extractInt.h
ExtMain.o:	${H}/signals.h
ExtMain.o:	${H}/stack.h
ExtMain.o:	${H}/utils.h
ExtMain.o:	${H}/windows.h
ExtMain.o:	${H}/dbwind.h
ExtMain.o:	${H}/main.h
ExtMain.o:	${H}/undo.h
 
# include file dependencies for "ExtNghbors.c"
ExtNghbors.o:	ExtNghbors.c
ExtNghbors.o:	${H}/magic.h
ExtNghbors.o:	${H}/geometry.h
ExtNghbors.o:	${H}/geofast.h
ExtNghbors.o:	${H}/tile.h
ExtNghbors.o:	${H}/hash.h
ExtNghbors.o:	${H}/database.h
ExtNghbors.o:	${H}/malloc.h
ExtNghbors.o:	${H}/debug.h
ExtNghbors.o:	extract.h
ExtNghbors.o:	extractInt.h
ExtNghbors.o:	${H}/signals.h
ExtNghbors.o:	${H}/stack.h
 
# include file dependencies for "ExtPerim.c"
ExtPerim.o:	ExtPerim.c
ExtPerim.o:	${H}/magic.h
ExtPerim.o:	${H}/geometry.h
ExtPerim.o:	${H}/tile.h
ExtPerim.o:	${H}/hash.h
ExtPerim.o:	${H}/database.h
ExtPerim.o:	${H}/malloc.h
ExtPerim.o:	${H}/stack.h
ExtPerim.o:	${H}/debug.h
ExtPerim.o:	extract.h
ExtPerim.o:	extractInt.h
 
# include file dependencies for "ExtRegion.c"
ExtRegion.o:	ExtRegion.c
ExtRegion.o:	${H}/magic.h
ExtRegion.o:	${H}/geometry.h
ExtRegion.o:	${H}/tile.h
ExtRegion.o:	${H}/hash.h
ExtRegion.o:	${H}/database.h
ExtRegion.o:	${H}/malloc.h
ExtRegion.o:	${H}/textio.h
ExtRegion.o:	${H}/debug.h
ExtRegion.o:	extract.h
ExtRegion.o:	extractInt.h
ExtRegion.o:	${H}/signals.h
 
# include file dependencies for "ExtSubtree.c"
ExtSubtree.o:	ExtSubtree.c
ExtSubtree.o:	${H}/magic.h
ExtSubtree.o:	${H}/geometry.h
ExtSubtree.o:	${H}/geofast.h
ExtSubtree.o:	${H}/tile.h
ExtSubtree.o:	${H}/hash.h
ExtSubtree.o:	${H}/database.h
ExtSubtree.o:	${H}/malloc.h
ExtSubtree.o:	${H}/textio.h
ExtSubtree.o:	${H}/debug.h
ExtSubtree.o:	extract.h
ExtSubtree.o:	extractInt.h
ExtSubtree.o:	${H}/signals.h
ExtSubtree.o:	${H}/windows.h
ExtSubtree.o:	${H}/dbwind.h
ExtSubtree.o:	${H}/styles.h
 
# include file dependencies for "ExtTech.c"
ExtTech.o:	ExtTech.c
ExtTech.o:	${H}/magic.h
ExtTech.o:	${H}/utils.h
ExtTech.o:	${H}/geometry.h
ExtTech.o:	${H}/tile.h
ExtTech.o:	${H}/hash.h
ExtTech.o:	${H}/database.h
ExtTech.o:	${H}/databaseInt.h
ExtTech.o:	${H}/malloc.h
ExtTech.o:	${H}/textio.h
ExtTech.o:	${H}/tech.h
ExtTech.o:	${H}/debug.h
ExtTech.o:	extract.h
ExtTech.o:	extractInt.h
 
# include file dependencies for "ExtTest.c"
ExtTest.o:	ExtTest.c
ExtTest.o:	${H}/magic.h
ExtTest.o:	${H}/utils.h
ExtTest.o:	${H}/geometry.h
ExtTest.o:	${H}/graphics.h
ExtTest.o:	${H}/styles.h
ExtTest.o:	${H}/tile.h
ExtTest.o:	${H}/hash.h
ExtTest.o:	${H}/database.h
ExtTest.o:	${H}/malloc.h
ExtTest.o:	${H}/windows.h
ExtTest.o:	${H}/dbwind.h
ExtTest.o:	${H}/main.h
ExtTest.o:	${H}/commands.h
ExtTest.o:	${H}/textio.h
ExtTest.o:	${H}/txcommands.h
ExtTest.o:	${H}/debug.h
ExtTest.o:	extract.h
ExtTest.o:	extractInt.h
 
# include file dependencies for "ExtTimes.c"
ExtTimes.o:	ExtTimes.c
ExtTimes.o:	${H}/magic.h
ExtTimes.o:	${H}/utils.h
ExtTimes.o:	${H}/geometry.h
ExtTimes.o:	${H}/geofast.h
ExtTimes.o:	${H}/tile.h
ExtTimes.o:	${H}/hash.h
ExtTimes.o:	${H}/database.h
ExtTimes.o:	${H}/malloc.h
ExtTimes.o:	${H}/textio.h
ExtTimes.o:	extract.h
ExtTimes.o:	extractInt.h
 
# include file dependencies for "ExtUnique.c"
ExtUnique.o:	ExtUnique.c
ExtUnique.o:	${H}/magic.h
ExtUnique.o:	${H}/geometry.h
ExtUnique.o:	${H}/styles.h
ExtUnique.o:	${H}/tile.h
ExtUnique.o:	${H}/hash.h
ExtUnique.o:	${H}/database.h
ExtUnique.o:	${H}/malloc.h
ExtUnique.o:	${H}/textio.h
ExtUnique.o:	${H}/debug.h
ExtUnique.o:	extract.h
ExtUnique.o:	extractInt.h
ExtUnique.o:	${H}/signals.h
ExtUnique.o:	${H}/stack.h
ExtUnique.o:	${H}/utils.h
ExtUnique.o:	${H}/windows.h
ExtUnique.o:	${H}/dbwind.h
ExtUnique.o:	${H}/main.h
ExtUnique.o:	${H}/undo.h
 
# include file dependencies for "ExtYank.c"
ExtYank.o:	ExtYank.c
ExtYank.o:	${H}/magic.h
ExtYank.o:	${H}/geometry.h
ExtYank.o:	${H}/geofast.h
ExtYank.o:	${H}/tile.h
ExtYank.o:	${H}/hash.h
ExtYank.o:	${H}/database.h
ExtYank.o:	${H}/malloc.h
ExtYank.o:	${H}/textio.h
ExtYank.o:	${H}/debug.h
ExtYank.o:	${H}/styles.h
ExtYank.o:	extract.h
ExtYank.o:	extractInt.h
 
