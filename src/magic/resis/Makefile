SHELL=/bin/sh
# sccsid "@(#)Makefile	4.5 MAGIC (Berkeley) 11/1/85" 

CC = cc
MODULE=	resis
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
DFLAGS= `cat ../misc/DFLAGS`  
CFLAGS= `cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

OBJS_SIG=   ResMain.o ResJunct.o ResMakeRes.o ResSimple.o \
	ResPrint.o ResReadSim.o ResConDCS.o ResRex.o \
	ResBasic.o ResMerge.o ResChecks.o ResFract.o ResUtils.o

POBJS_SIG=  ResMain.po ResJunct.po ResMakeRes.po ResSimple.po \
	ResPrint.po  ResReadSim.po ResConDCS.po ResRex.po \
	ResBasic.po ResMerge.po ResChecks.po ResFract.po ResUtils.po

SRCS_SIG=  ResMain.c ResJunct.c ResMakeRes.c ResSimple.c \
	ResPrint.c  ResReadSim.c ResConDCS.c ResRex.c \
	ResBasic.c ResMerge.c ResChecks.c ResFract.c ResUtils.c

SRCS_DEBUG= ResDebug.c
OBJS_DEBUG= ResDebug.o
POBJS_DEBUG= ResDebug.po

SRCS  = ${SRCS_SIG} ${SRCS_DEBUG}
OBJS  = ${OBJS_SIG} ${OBJS_DEBUG}
POBJS = ${POBJS_SIG} ${POBJS_DEBUG}
HDRS= resis.h

LINTFLAGS= ${IFLAGS} -u 
LINTLIBS= 
ALLSRCS= ${SRCS} ${HDRS} Makefile

.SUFFIXES: .po

resis.o: ${HDRS} ${OBJS}
	rm -f resis.o
	ld -r ${OBJS} -o resis.o

resis.po: ${HDRS} ${POBJS}
	rm -f resis.po
	ld -r ${POBJS} -o resis.po

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

lint:	forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} >lint

forceit:

tags: ${HDRS} ${SRCS}
	ctags ${HDRS} ${SRCS}

install: installhdrs  

installhdrs: ${H}/${MODULE}.h


profile: installhdrs ${LIB}/resis.po ${LIB}/lint/resis.ln 

install: installhdrs ${LIB}/resis.o ${LIB}/lint/resis.ln 

installhdrs: ${H}/${MODULE}.h 

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	cp ${MODULE}.o ${LIB}
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/resis.po: resis.po
	rm -f ${LIB}/resis.po
	cp resis.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}


#include dependencies 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:10:51 PDT 1990.
 
# include file dependencies for "ResBasic.c"
ResBasic.o:	ResBasic.c
ResBasic.o:	${H}/magic.h
ResBasic.o:	${H}/geometry.h
ResBasic.o:	${H}/geofast.h
ResBasic.o:	${H}/tile.h
ResBasic.o:	${H}/hash.h
ResBasic.o:	${H}/database.h
ResBasic.o:	${H}/malloc.h
ResBasic.o:	${H}/textio.h
ResBasic.o:	${H}/extract.h
ResBasic.o:	${H}/extractInt.h
ResBasic.o:	${H}/windows.h
ResBasic.o:	${H}/dbwind.h
ResBasic.o:	${H}/tech.h
ResBasic.o:	${H}/txcommands.h
ResBasic.o:	resis.h
 
# include file dependencies for "ResChecks.c"
ResChecks.o:	ResChecks.c
ResChecks.o:	${H}/magic.h
ResChecks.o:	${H}/geometry.h
ResChecks.o:	${H}/geofast.h
ResChecks.o:	${H}/tile.h
ResChecks.o:	${H}/hash.h
ResChecks.o:	${H}/database.h
ResChecks.o:	${H}/malloc.h
ResChecks.o:	${H}/textio.h
ResChecks.o:	${H}/extract.h
ResChecks.o:	${H}/extractInt.h
ResChecks.o:	${H}/windows.h
ResChecks.o:	${H}/dbwind.h
ResChecks.o:	${H}/utils.h
ResChecks.o:	${H}/tech.h
ResChecks.o:	${H}/txcommands.h
ResChecks.o:	${H}/stack.h
ResChecks.o:	resis.h
 
# include file dependencies for "ResConDCS.c"
ResConDCS.o:	ResConDCS.c
ResConDCS.o:	${H}/magic.h
ResConDCS.o:	${H}/geometry.h
ResConDCS.o:	${H}/geofast.h
ResConDCS.o:	${H}/tile.h
ResConDCS.o:	${H}/hash.h
ResConDCS.o:	${H}/database.h
ResConDCS.o:	${H}/databaseInt.h
ResConDCS.o:	${H}/malloc.h
ResConDCS.o:	${H}/textio.h
ResConDCS.o:	${H}/extract.h
ResConDCS.o:	${H}/extractInt.h
ResConDCS.o:	${H}/signals.h
ResConDCS.o:	${H}/windows.h
ResConDCS.o:	${H}/dbwind.h
ResConDCS.o:	${H}/tech.h
ResConDCS.o:	${H}/txcommands.h
ResConDCS.o:	resis.h
 
# include file dependencies for "ResDebug.c"
ResDebug.o:	ResDebug.c
ResDebug.o:	${H}/magic.h
ResDebug.o:	${H}/geometry.h
ResDebug.o:	${H}/geofast.h
ResDebug.o:	${H}/tile.h
ResDebug.o:	${H}/hash.h
ResDebug.o:	${H}/database.h
ResDebug.o:	${H}/malloc.h
ResDebug.o:	${H}/textio.h
ResDebug.o:	${H}/extract.h
ResDebug.o:	${H}/extractInt.h
ResDebug.o:	${H}/windows.h
ResDebug.o:	${H}/dbwind.h
ResDebug.o:	${H}/utils.h
ResDebug.o:	${H}/tech.h
ResDebug.o:	${H}/txcommands.h
ResDebug.o:	${H}/stack.h
ResDebug.o:	resis.h
 
# include file dependencies for "ResFract.c"
ResFract.o:	ResFract.c
ResFract.o:	${H}/magic.h
ResFract.o:	${H}/geometry.h
ResFract.o:	${H}/txcommands.h
ResFract.o:	${H}/tile.h
ResFract.o:	${H}/signals.h
ResFract.o:	${H}/hash.h
ResFract.o:	${H}/database.h
ResFract.o:	${H}/databaseInt.h
ResFract.o:	${H}/malloc.h
ResFract.o:	${H}/windows.h
ResFract.o:	${H}/main.h
 
# include file dependencies for "ResJunct.c"
ResJunct.o:	ResJunct.c
ResJunct.o:	${H}/magic.h
ResJunct.o:	${H}/geometry.h
ResJunct.o:	${H}/geofast.h
ResJunct.o:	${H}/tile.h
ResJunct.o:	${H}/hash.h
ResJunct.o:	${H}/database.h
ResJunct.o:	${H}/databaseInt.h
ResJunct.o:	${H}/malloc.h
ResJunct.o:	${H}/textio.h
ResJunct.o:	${H}/extract.h
ResJunct.o:	${H}/extractInt.h
ResJunct.o:	${H}/windows.h
ResJunct.o:	${H}/dbwind.h
ResJunct.o:	${H}/stack.h
ResJunct.o:	${H}/tech.h
ResJunct.o:	${H}/txcommands.h
ResJunct.o:	resis.h
 
# include file dependencies for "ResMain.c"
ResMain.o:	ResMain.c
ResMain.o:	${H}/magic.h
ResMain.o:	${H}/geometry.h
ResMain.o:	${H}/geofast.h
ResMain.o:	${H}/tile.h
ResMain.o:	${H}/hash.h
ResMain.o:	${H}/database.h
ResMain.o:	${H}/databaseInt.h
ResMain.o:	${H}/malloc.h
ResMain.o:	${H}/textio.h
ResMain.o:	${H}/extract.h
ResMain.o:	${H}/extractInt.h
ResMain.o:	${H}/windows.h
ResMain.o:	${H}/dbwind.h
ResMain.o:	${H}/tech.h
ResMain.o:	${H}/txcommands.h
ResMain.o:	resis.h
 
# include file dependencies for "ResMakeRes.c"
ResMakeRes.o:	ResMakeRes.c
ResMakeRes.o:	${H}/magic.h
ResMakeRes.o:	${H}/geometry.h
ResMakeRes.o:	${H}/geofast.h
ResMakeRes.o:	${H}/tile.h
ResMakeRes.o:	${H}/hash.h
ResMakeRes.o:	${H}/database.h
ResMakeRes.o:	${H}/malloc.h
ResMakeRes.o:	${H}/textio.h
ResMakeRes.o:	${H}/extract.h
ResMakeRes.o:	${H}/extractInt.h
ResMakeRes.o:	${H}/windows.h
ResMakeRes.o:	${H}/dbwind.h
ResMakeRes.o:	${H}/tech.h
ResMakeRes.o:	${H}/txcommands.h
ResMakeRes.o:	resis.h
 
# include file dependencies for "ResMerge.c"
ResMerge.o:	ResMerge.c
ResMerge.o:	${H}/magic.h
ResMerge.o:	${H}/geometry.h
ResMerge.o:	${H}/geofast.h
ResMerge.o:	${H}/tile.h
ResMerge.o:	${H}/hash.h
ResMerge.o:	${H}/database.h
ResMerge.o:	${H}/malloc.h
ResMerge.o:	${H}/textio.h
ResMerge.o:	resis.h
 
# include file dependencies for "ResPrint.c"
ResPrint.o:	ResPrint.c
ResPrint.o:	${H}/magic.h
ResPrint.o:	${H}/geometry.h
ResPrint.o:	${H}/geofast.h
ResPrint.o:	${H}/tile.h
ResPrint.o:	${H}/hash.h
ResPrint.o:	${H}/database.h
ResPrint.o:	${H}/malloc.h
ResPrint.o:	${H}/textio.h
ResPrint.o:	${H}/extract.h
ResPrint.o:	${H}/extractInt.h
ResPrint.o:	${H}/windows.h
ResPrint.o:	${H}/dbwind.h
ResPrint.o:	${H}/utils.h
ResPrint.o:	${H}/tech.h
ResPrint.o:	${H}/txcommands.h
ResPrint.o:	${H}/stack.h
ResPrint.o:	resis.h
 
# include file dependencies for "ResReadSim.c"
ResReadSim.o:	ResReadSim.c
ResReadSim.o:	${H}/magic.h
ResReadSim.o:	${H}/geometry.h
ResReadSim.o:	${H}/geofast.h
ResReadSim.o:	${H}/tile.h
ResReadSim.o:	${H}/hash.h
ResReadSim.o:	${H}/database.h
ResReadSim.o:	${H}/malloc.h
ResReadSim.o:	${H}/textio.h
ResReadSim.o:	${H}/extract.h
ResReadSim.o:	${H}/extractInt.h
ResReadSim.o:	${H}/windows.h
ResReadSim.o:	${H}/dbwind.h
ResReadSim.o:	${H}/utils.h
ResReadSim.o:	${H}/tech.h
ResReadSim.o:	${H}/txcommands.h
ResReadSim.o:	resis.h
 
# include file dependencies for "ResRex.c"
ResRex.o:	ResRex.c
ResRex.o:	${H}/magic.h
ResRex.o:	${H}/geometry.h
ResRex.o:	${H}/geofast.h
ResRex.o:	${H}/tile.h
ResRex.o:	${H}/hash.h
ResRex.o:	${H}/database.h
ResRex.o:	${H}/malloc.h
ResRex.o:	${H}/textio.h
ResRex.o:	${H}/extract.h
ResRex.o:	${H}/extractInt.h
ResRex.o:	${H}/windows.h
ResRex.o:	${H}/dbwind.h
ResRex.o:	${H}/utils.h
ResRex.o:	${H}/tech.h
ResRex.o:	${H}/txcommands.h
ResRex.o:	resis.h
 
# include file dependencies for "ResSimple.c"
ResSimple.o:	ResSimple.c
ResSimple.o:	${H}/magic.h
ResSimple.o:	${H}/geometry.h
ResSimple.o:	${H}/geofast.h
ResSimple.o:	${H}/tile.h
ResSimple.o:	${H}/hash.h
ResSimple.o:	${H}/doubleint.h
ResSimple.o:	${H}/heap.h
ResSimple.o:	${H}/database.h
ResSimple.o:	${H}/malloc.h
ResSimple.o:	${H}/textio.h
ResSimple.o:	${H}/extract.h
ResSimple.o:	${H}/extractInt.h
ResSimple.o:	${H}/windows.h
ResSimple.o:	${H}/dbwind.h
ResSimple.o:	${H}/stack.h
ResSimple.o:	${H}/tech.h
ResSimple.o:	${H}/txcommands.h
ResSimple.o:	resis.h
 
# include file dependencies for "ResUtils.c"
ResUtils.o:	ResUtils.c
ResUtils.o:	${H}/magic.h
ResUtils.o:	${H}/geometry.h
ResUtils.o:	${H}/geofast.h
ResUtils.o:	${H}/tile.h
ResUtils.o:	${H}/hash.h
ResUtils.o:	${H}/database.h
ResUtils.o:	${H}/databaseInt.h
ResUtils.o:	${H}/malloc.h
ResUtils.o:	${H}/textio.h
ResUtils.o:	${H}/extract.h
ResUtils.o:	${H}/extractInt.h
ResUtils.o:	${H}/windows.h
ResUtils.o:	${H}/dbwind.h
ResUtils.o:	${H}/stack.h
ResUtils.o:	${H}/tech.h
ResUtils.o:	${H}/txcommands.h
ResUtils.o:	resis.h
 
# include file dependencies for "ResWrite.c"
ResWrite.o:	ResWrite.c
ResWrite.o:	${H}/magic.h
ResWrite.o:	${H}/geometry.h
ResWrite.o:	${H}/geofast.h
ResWrite.o:	${H}/tile.h
ResWrite.o:	${H}/hash.h
ResWrite.o:	${H}/database.h
ResWrite.o:	${H}/malloc.h
ResWrite.o:	${H}/textio.h
ResWrite.o:	${H}/extract.h
ResWrite.o:	${H}/extractInt.h
ResWrite.o:	${H}/windows.h
ResWrite.o:	${H}/dbwind.h
ResWrite.o:	${H}/stack.h
ResWrite.o:	${H}/tech.h
ResWrite.o:	${H}/txcommands.h
ResWrite.o:	resis.h
 
