SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.1 90/09/13 12:14:33 mayo Exp $
#
CADDIR  = /udir/mayo/cadhome
MAGICDIR= ..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=-I. -I${H} -u
LINTLIBS=
RHOST=	ucbrenoir
LIBS= `cat ../magic/LIBS`

CORE=	${LIB}/cif.o ${LIB}/cmwind.o ${LIB}/commands.o \
	${LIB}/database.o ${LIB}/dbwind.o ${LIB}/drc.o \
	${LIB}/debug.o ${LIB}/extract.o ${LIB}/graphics.o \
	${LIB}/macros.o ${LIB}/main.o ${LIB}/misc.o \
	${LIB}/parser.o ${LIB}/select.o ${LIB}/signals.o \
	${LIB}/tech.o ${LIB}/textio.o ${LIB}/tiles.o \
	${LIB}/undo.o ${LIB}/utils.o ${LIB}/windows.o \
	${LIB}/wiring.o 

PLOW=   ${LIB}/plow.o
PLOT=   ${LIB}/plot.o
RESIS=  ${LIB}/resis.o
SIM=	${LIB}/sim.o
CALMA=	${LIB}/calma.o
ROUTE=	${LIB}/mzrouter.o ${LIB}/irouter.o ${LIB}/garouter.o ${LIB}/router.o \
	${LIB}/grouter.o ${LIB}/gcr.o ${LIB}/netmenu.o ${LIB}/netlist.o

LNKS= ${CORE} ${PLOW} ${PLOT} ${RESIS} ${SIM} ${CALMA} ${ROUTE} 

ALLSRCS= Makefile

.SUFFIXES: .po

magic: ${LNKS} magicTop.o
	rm -f magic
	${CC} ${CFLAGS} ${LNKS} magicTop.o ${LIBS} -o magic

magicp: ${PLNKS} magicTop.o
	rm -f magicp
	${CC} ${PCFLAGS} ${POBJS} ${PLNKS} ${LIBS} -o magicp

magicTop.o: 
	rm -f magicTop.o
	${CC} ${CFLAGS} -c magicTop.c

version: forceit
	csh -f :newvers

forceit:
	
install: ${LIB}/magicTop.o ${CADDIR}/bin/magic ${CADDIR}/lib/magic/sys/.magic

${LIB}/magicTop.o: magicTop.o
	rm -f ${LIB}/magicTop.o
	ld -x -r magicTop.o -o ${LIB}/magicTop.o

${CADDIR}/bin/magic: magic
	-mkdir ${CADDIR}/bin
	-mv -f ${CADDIR}/bin/magic ${CADDIR}/bin/magic.old
	cp magic ${CADDIR}/bin/magic

${CADDIR}/lib/magic/sys/.magic: proto.magic
	-mkdir ${CADDIR}/lib
	-mkdir ${CADDIR}/lib/magic
	-mkdir ${CADDIR}/lib/magic/sys
	-mv -f ${CADDIR}/lib/magic/sys/.magic ${CADDIR}/lib/magic/sys/old.magic
	cp proto.magic ${CADDIR}/lib/magic/sys/.magic

.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:14:25 PDT 1990.
 
# include file dependencies for "magicTop.c"
magicTop.o:	magicTop.c
 
