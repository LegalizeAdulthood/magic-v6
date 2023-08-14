SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 13:13:12 mayo Exp $
#
MODULE=	windows
CADDIR  = /udir/mayo/cadhome
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
CADLIB=	${CADDIR}/lib/magic
BIN=	${MAGICDIR}/bin


SRCS=	windClient.c windCmdAM.c windCmdNR.c windCmdSZ.c \
	windSend.c windDebug.c windDisp.c windSun.c \
	windMain.c windMove.c windSearch.c windTrans.c \
	windView.c
OBJS=	windClient.o windCmdAM.o windCmdNR.o windCmdSZ.o \
	windSend.o windDebug.o windDisp.o windSun.o \
	windMain.o windMove.o windSearch.o windTrans.o \
	windView.o
POBJS=	windClient.po windCmdAM.po windCmdNR.po windCmdSZ.po \
	windSend.po windDebug.po windDisp.po windSun.po \
	windMain.po windMove.po windSearch.po windTrans.po \
	windView.po
ALLSRCS= ${SRCS} ${MODULE}.h windInt.h Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=${IFLAGS} -DPARANOID -u
LINTLIBS= ${LIB}/lint/textio.ln ${LIB}/lint/utils.ln ${LIB}/lint/graphics.ln \
	  ${LIB}/lint/macros.ln 
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

profile: installhdrs ${LIB}/${MODULE}.po ${LIB}/lint/${MODULE}.ln

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln 

installcad: ${CADLIB}/sys/windows7.glyphs ${CADLIB}/sys/windows11.glyphs \
	 ${CADLIB}/sys/windows14.glyphs ${CADLIB}/sys/windows22.glyphs

installhdrs: ${H}/${MODULE}.h ${H}/windInt.h

${CADLIB}/sys/windows7.glyphs: windows7.glyphs
	rm -f ${CADLIB}/sys/windows7.glyphs
	cp windows7.glyphs ${CADLIB}/sys/windows7.glyphs

${CADLIB}/sys/windows11.glyphs: windows11.glyphs
	rm -f ${CADLIB}/sys/windows11.glyphs
	cp windows11.glyphs ${CADLIB}/sys/windows11.glyphs

${CADLIB}/sys/windows14.glyphs: windows14.glyphs
	rm -f ${CADLIB}/sys/windows14.glyphs
	cp windows14.glyphs ${CADLIB}/sys/windows14.glyphs

${CADLIB}/sys/windows22.glyphs: windows22.glyphs
	rm -f ${CADLIB}/sys/windows22.glyphs
	cp windows22.glyphs ${CADLIB}/sys/windows22.glyphs

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -r ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/${MODULE}.po: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${H}/windInt.h: windInt.h
	rm -f ${H}/windInt.h 
	cp windInt.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:13:03 PDT 1990.
 
# include file dependencies for "windClient.c"
windClient.o:	windClient.c
windClient.o:	${H}/magic.h
windClient.o:	${H}/textio.h
windClient.o:	${H}/geometry.h
windClient.o:	windows.h
windClient.o:	${H}/glyphs.h
windClient.o:	windInt.h
windClient.o:	${H}/tile.h
windClient.o:	${H}/hash.h
windClient.o:	${H}/database.h
windClient.o:	${H}/main.h
windClient.o:	${H}/macros.h
windClient.o:	${H}/utils.h
windClient.o:	${H}/malloc.h
windClient.o:	${H}/graphics.h
windClient.o:	${H}/styles.h
windClient.o:	${H}/txcommands.h
windClient.o:	${H}/undo.h
 
# include file dependencies for "windCmdAM.c"
windCmdAM.o:	windCmdAM.c
windCmdAM.o:	${H}/magic.h
windCmdAM.o:	${H}/textio.h
windCmdAM.o:	${H}/geometry.h
windCmdAM.o:	windows.h
windCmdAM.o:	${H}/malloc.h
windCmdAM.o:	${H}/runstats.h
windCmdAM.o:	${H}/macros.h
windCmdAM.o:	${H}/signals.h
windCmdAM.o:	${H}/graphics.h
windCmdAM.o:	${H}/styles.h
windCmdAM.o:	${H}/txcommands.h
windCmdAM.o:	${H}/glyphs.h
windCmdAM.o:	windInt.h
windCmdAM.o:	${H}/tile.h
windCmdAM.o:	${H}/hash.h
windCmdAM.o:	${H}/database.h
windCmdAM.o:	${H}/dbwind.h
windCmdAM.o:	${H}/utils.h
 
# include file dependencies for "windCmdNR.c"
windCmdNR.o:	windCmdNR.c
windCmdNR.o:	${H}/magic.h
windCmdNR.o:	${H}/textio.h
windCmdNR.o:	${H}/geometry.h
windCmdNR.o:	windows.h
windCmdNR.o:	${H}/glyphs.h
windCmdNR.o:	windInt.h
windCmdNR.o:	${H}/tile.h
windCmdNR.o:	${H}/hash.h
windCmdNR.o:	${H}/database.h
windCmdNR.o:	${H}/main.h
windCmdNR.o:	${H}/tech.h
windCmdNR.o:	${H}/runstats.h
windCmdNR.o:	${H}/utils.h
windCmdNR.o:	${H}/graphics.h
windCmdNR.o:	${H}/txcommands.h
windCmdNR.o:	${H}/dbwind.h
 
# include file dependencies for "windCmdSZ.c"
windCmdSZ.o:	windCmdSZ.c
windCmdSZ.o:	${H}/magic.h
windCmdSZ.o:	${H}/textio.h
windCmdSZ.o:	${H}/geometry.h
windCmdSZ.o:	${H}/tile.h
windCmdSZ.o:	windows.h
windCmdSZ.o:	${H}/glyphs.h
windCmdSZ.o:	windInt.h
windCmdSZ.o:	${H}/utils.h
windCmdSZ.o:	${H}/signals.h
windCmdSZ.o:	${H}/txcommands.h
 
# include file dependencies for "windDebug.c"
windDebug.o:	windDebug.c
windDebug.o:	${H}/magic.h
windDebug.o:	${H}/geometry.h
windDebug.o:	windows.h
windDebug.o:	${H}/glyphs.h
windDebug.o:	windInt.h
windDebug.o:	${H}/textio.h
windDebug.o:	${H}/txcommands.h
 
# include file dependencies for "windDisp.c"
windDisp.o:	windDisp.c
windDisp.o:	${H}/magic.h
windDisp.o:	${H}/utils.h
windDisp.o:	${H}/textio.h
windDisp.o:	${H}/geometry.h
windDisp.o:	${H}/graphics.h
windDisp.o:	${H}/glyphs.h
windDisp.o:	${H}/styles.h
windDisp.o:	windows.h
windDisp.o:	windInt.h
windDisp.o:	${H}/tile.h
windDisp.o:	${H}/hash.h
windDisp.o:	${H}/database.h
windDisp.o:	${H}/undo.h
windDisp.o:	${H}/signals.h
windDisp.o:	${H}/malloc.h
 
# include file dependencies for "windMain.c"
windMain.o:	windMain.c
windMain.o:	${H}/magic.h
windMain.o:	${H}/geometry.h
windMain.o:	${H}/glyphs.h
windMain.o:	windows.h
windMain.o:	windInt.h
windMain.o:	${H}/stack.h
windMain.o:	${H}/tile.h
windMain.o:	${H}/hash.h
windMain.o:	${H}/database.h
windMain.o:	${H}/textio.h
windMain.o:	${H}/graphics.h
windMain.o:	${H}/malloc.h
 
# include file dependencies for "windMove.c"
windMove.o:	windMove.c
windMove.o:	${H}/magic.h
windMove.o:	${H}/textio.h
windMove.o:	${H}/geometry.h
windMove.o:	windows.h
windMove.o:	${H}/graphics.h
windMove.o:	${H}/glyphs.h
windMove.o:	windInt.h
windMove.o:	${H}/tile.h
windMove.o:	${H}/hash.h
windMove.o:	${H}/database.h
windMove.o:	${H}/malloc.h
 
# include file dependencies for "windSearch.c"
windSearch.o:	windSearch.c
windSearch.o:	${H}/magic.h
windSearch.o:	${H}/geometry.h
windSearch.o:	windows.h
windSearch.o:	${H}/glyphs.h
windSearch.o:	windInt.h
 
# include file dependencies for "windSend.c"
windSend.o:	windSend.c
windSend.o:	${H}/magic.h
windSend.o:	${H}/textio.h
windSend.o:	${H}/geometry.h
windSend.o:	windows.h
windSend.o:	${H}/glyphs.h
windSend.o:	windInt.h
windSend.o:	${H}/stack.h
windSend.o:	${H}/utils.h
windSend.o:	${H}/signals.h
windSend.o:	${H}/txcommands.h
 
# include file dependencies for "windSun.c"
windSun.o:	windSun.c
windSun.o:	${H}/magic.h
windSun.o:	${H}/textio.h
windSun.o:	${H}/geometry.h
windSun.o:	windows.h
windSun.o:	${H}/glyphs.h
windSun.o:	windInt.h
windSun.o:	${H}/utils.h
windSun.o:	${H}/graphics.h
windSun.o:	${H}/txcommands.h
windSun.o:	${H}/grSunInt.h
 
# include file dependencies for "windTrans.c"
windTrans.o:	windTrans.c
windTrans.o:	${H}/magic.h
windTrans.o:	${H}/geometry.h
windTrans.o:	windows.h
windTrans.o:	${H}/glyphs.h
windTrans.o:	windInt.h
 
# include file dependencies for "windView.c"
windView.o:	windView.c
windView.o:	${H}/magic.h
windView.o:	${H}/geometry.h
windView.o:	windows.h
windView.o:	${H}/glyphs.h
windView.o:	windInt.h
windView.o:	${H}/textio.h
 
