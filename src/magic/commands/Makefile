SHELL=/bin/sh
# rcsid $Header: Makefile,v 6.1 90/08/29 14:48:43 mayo Exp $

MODULE=	commands
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}


OBJS=	CmdINTERP.o CmdSubrs.o SWCmds.o \
	CmdAB.o CmdCD.o CmdE.o CmdFI.o CmdLQ.o CmdRS.o CmdTZ.o CmdWizard.o
POBJS=	CmdINTERP.po CmdSubrs.po SWCmds.po \
	CmdAB.po CmdCD.po CmdE.po CmdFI.po CmdLQ.po CmdRS.po \
	CmdTZ.po CmdWizard.po
SRCS=	CmdINTERP.c CmdSubrs.c SWCmds.c \
	CmdAB.c CmdCD.c CmdE.c CmdFI.c CmdLQ.c CmdRS.c CmdTZ.c CmdWizard.c
HDRS=	commands.h

LINTFLAGS= ${IFLAGS} -u -DPARANOID
LINTLIBS= ${LIB}/lint/cif.ln ${LIB}/lint/database.ln ${LIB}/lint/drc.ln \
	${LIB}/lint/dbwind.ln ${LIB}/lint/gcr.ln\
	${LIB}/lint/router.ln ${LIB}/lint/textio.ln ${LIB}/lint/tiles.ln \
	${LIB}/lint/undo.ln ${LIB}/lint/utils.ln ${LIB}/lint/windows.ln
ALLSRCS= ${SRCS} ${HDRS} Makefile
RHOST=	dali
.SUFFIXES: .po

commands.o: ${OBJS}
	rm -f commands.o
	ld -r ${OBJS} -o commands.o

commands.po: ${POBJS}
	rm -f commands.po
	ld -r ${POBJS} -o commands.po

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

tags: ${HDRS} ${SRCS}
	-ctags ${HDRS} ${SRCS}

install: installhdrs ${LIB}/commands.o ${LIB}/lint/commands.ln

installhdrs: ${H}/${MODULE}.h


${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	cp ${MODULE}.o ${LIB}
	../:instclean ${MODULE} ${SRCS} Makefile

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

profile: commands.po
	rm -f ${LIB}/commands.po
	cp commands.po ${LIB}/commands.po

 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:40:35 PDT 1990.
 
# include file dependencies for "CmdAB.c"
CmdAB.o:	CmdAB.c
CmdAB.o:	${H}/magic.h
CmdAB.o:	${H}/geometry.h
CmdAB.o:	${H}/tile.h
CmdAB.o:	${H}/hash.h
CmdAB.o:	${H}/database.h
CmdAB.o:	${H}/windows.h
CmdAB.o:	${H}/dbwind.h
CmdAB.o:	${H}/main.h
CmdAB.o:	commands.h
CmdAB.o:	${H}/utils.h
CmdAB.o:	${H}/textio.h
CmdAB.o:	${H}/drc.h
CmdAB.o:	${H}/graphics.h
CmdAB.o:	${H}/txcommands.h
CmdAB.o:	${H}/malloc.h
CmdAB.o:	${H}/netlist.h
 
# include file dependencies for "CmdCD.c"
CmdCD.o:	CmdCD.c
CmdCD.o:	${H}/magic.h
CmdCD.o:	${H}/geometry.h
CmdCD.o:	${H}/tile.h
CmdCD.o:	${H}/hash.h
CmdCD.o:	${H}/database.h
CmdCD.o:	${H}/windows.h
CmdCD.o:	${H}/dbwind.h
CmdCD.o:	${H}/main.h
CmdCD.o:	commands.h
CmdCD.o:	${H}/utils.h
CmdCD.o:	${H}/textio.h
CmdCD.o:	${H}/drc.h
CmdCD.o:	${H}/graphics.h
CmdCD.o:	${H}/txcommands.h
CmdCD.o:	${H}/cif.h
CmdCD.o:	${H}/calma.h
CmdCD.o:	${H}/styles.h
CmdCD.o:	${H}/rtrDcmpose.h
CmdCD.o:	${H}/select.h
CmdCD.o:	${H}/malloc.h
 
# include file dependencies for "CmdE.c"
CmdE.o:	CmdE.c
CmdE.o:	${H}/magic.h
CmdE.o:	${H}/geometry.h
CmdE.o:	${H}/utils.h
CmdE.o:	${H}/tile.h
CmdE.o:	${H}/hash.h
CmdE.o:	${H}/database.h
CmdE.o:	${H}/windows.h
CmdE.o:	${H}/dbwind.h
CmdE.o:	${H}/main.h
CmdE.o:	commands.h
CmdE.o:	${H}/textio.h
CmdE.o:	${H}/macros.h
CmdE.o:	${H}/drc.h
CmdE.o:	${H}/txcommands.h
CmdE.o:	${H}/extract.h
CmdE.o:	${H}/select.h
 
# include file dependencies for "CmdFI.c"
CmdFI.o:	CmdFI.c
CmdFI.o:	${H}/magic.h
CmdFI.o:	${H}/geometry.h
CmdFI.o:	${H}/utils.h
CmdFI.o:	${H}/tile.h
CmdFI.o:	${H}/hash.h
CmdFI.o:	${H}/database.h
CmdFI.o:	${H}/windows.h
CmdFI.o:	${H}/dbwind.h
CmdFI.o:	${H}/main.h
CmdFI.o:	commands.h
CmdFI.o:	${H}/textio.h
CmdFI.o:	${H}/macros.h
CmdFI.o:	${H}/drc.h
CmdFI.o:	${H}/txcommands.h
CmdFI.o:	${H}/styles.h
CmdFI.o:	${H}/extract.h
CmdFI.o:	${H}/malloc.h
CmdFI.o:	${H}/select.h
CmdFI.o:	${H}/sim.h
CmdFI.o:	${H}/gcr.h
 
# include file dependencies for "CmdINTERP.c"
CmdINTERP.o:	CmdINTERP.c
CmdINTERP.o:	${H}/magic.h
CmdINTERP.o:	${H}/geometry.h
CmdINTERP.o:	${H}/tile.h
CmdINTERP.o:	${H}/hash.h
CmdINTERP.o:	${H}/database.h
CmdINTERP.o:	${H}/windows.h
CmdINTERP.o:	${H}/dbwind.h
CmdINTERP.o:	${H}/main.h
CmdINTERP.o:	commands.h
CmdINTERP.o:	${H}/textio.h
CmdINTERP.o:	${H}/txcommands.h
 
# include file dependencies for "CmdLQ.c"
CmdLQ.o:	CmdLQ.c
CmdLQ.o:	${H}/magic.h
CmdLQ.o:	${H}/geometry.h
CmdLQ.o:	${H}/utils.h
CmdLQ.o:	${H}/tile.h
CmdLQ.o:	${H}/hash.h
CmdLQ.o:	${H}/database.h
CmdLQ.o:	${H}/windows.h
CmdLQ.o:	${H}/dbwind.h
CmdLQ.o:	${H}/main.h
CmdLQ.o:	commands.h
CmdLQ.o:	${H}/textio.h
CmdLQ.o:	${H}/graphics.h
CmdLQ.o:	${H}/drc.h
CmdLQ.o:	${H}/txcommands.h
CmdLQ.o:	${H}/undo.h
CmdLQ.o:	${H}/router.h
CmdLQ.o:	${H}/plow.h
CmdLQ.o:	${H}/select.h
CmdLQ.o:	${H}/plot.h
 
# include file dependencies for "CmdRS.c"
CmdRS.o:	CmdRS.c
CmdRS.o:	${H}/magic.h
CmdRS.o:	${H}/stack.h
CmdRS.o:	${H}/geometry.h
CmdRS.o:	${H}/utils.h
CmdRS.o:	${H}/tile.h
CmdRS.o:	${H}/hash.h
CmdRS.o:	${H}/database.h
CmdRS.o:	${H}/windows.h
CmdRS.o:	${H}/dbwind.h
CmdRS.o:	${H}/main.h
CmdRS.o:	commands.h
CmdRS.o:	${H}/textio.h
CmdRS.o:	${H}/graphics.h
CmdRS.o:	${H}/tech.h
CmdRS.o:	${H}/drc.h
CmdRS.o:	${H}/txcommands.h
CmdRS.o:	${H}/router.h
CmdRS.o:	${H}/gcr.h
CmdRS.o:	${H}/doubleint.h
CmdRS.o:	${H}/heap.h
CmdRS.o:	${H}/grouter.h
CmdRS.o:	${H}/netlist.h
CmdRS.o:	${H}/netmenu.h
CmdRS.o:	${H}/select.h
CmdRS.o:	${H}/sim.h
 
# include file dependencies for "CmdSubrs.c"
CmdSubrs.o:	CmdSubrs.c
CmdSubrs.o:	${H}/magic.h
CmdSubrs.o:	${H}/geometry.h
CmdSubrs.o:	${H}/utils.h
CmdSubrs.o:	${H}/tile.h
CmdSubrs.o:	${H}/hash.h
CmdSubrs.o:	${H}/database.h
CmdSubrs.o:	${H}/windows.h
CmdSubrs.o:	${H}/dbwind.h
CmdSubrs.o:	${H}/main.h
CmdSubrs.o:	commands.h
CmdSubrs.o:	${H}/textio.h
CmdSubrs.o:	${H}/drc.h
CmdSubrs.o:	${H}/txcommands.h
CmdSubrs.o:	${H}/undo.h
 
# include file dependencies for "CmdTZ.c"
CmdTZ.o:	CmdTZ.c
CmdTZ.o:	${H}/magic.h
CmdTZ.o:	${H}/geometry.h
CmdTZ.o:	${H}/utils.h
CmdTZ.o:	${H}/tile.h
CmdTZ.o:	${H}/hash.h
CmdTZ.o:	${H}/database.h
CmdTZ.o:	${H}/windows.h
CmdTZ.o:	${H}/dbwind.h
CmdTZ.o:	${H}/main.h
CmdTZ.o:	commands.h
CmdTZ.o:	${H}/textio.h
CmdTZ.o:	${H}/txcommands.h
CmdTZ.o:	${H}/signals.h
CmdTZ.o:	${H}/router.h
CmdTZ.o:	${H}/gcr.h
CmdTZ.o:	${H}/undo.h
CmdTZ.o:	${H}/select.h
CmdTZ.o:	${H}/styles.h
CmdTZ.o:	${H}/wiring.h
CmdTZ.o:	${H}/netlist.h
CmdTZ.o:	${H}/netmenu.h
 
# include file dependencies for "CmdWizard.c"
CmdWizard.o:	CmdWizard.c
CmdWizard.o:	${H}/magic.h
CmdWizard.o:	${H}/geometry.h
CmdWizard.o:	${H}/malloc.h
CmdWizard.o:	${H}/tile.h
CmdWizard.o:	${H}/hash.h
CmdWizard.o:	${H}/database.h
CmdWizard.o:	${H}/windows.h
CmdWizard.o:	${H}/dbwind.h
CmdWizard.o:	${H}/main.h
CmdWizard.o:	commands.h
CmdWizard.o:	${H}/runstats.h
CmdWizard.o:	${H}/textio.h
CmdWizard.o:	${H}/graphics.h
CmdWizard.o:	${H}/signals.h
CmdWizard.o:	${H}/utils.h
CmdWizard.o:	${H}/txcommands.h
 
# include file dependencies for "SWCmds.c"
SWCmds.o:	SWCmds.c
SWCmds.o:	${H}/magic.h
SWCmds.o:	${H}/geometry.h
SWCmds.o:	${H}/utils.h
SWCmds.o:	${H}/tile.h
SWCmds.o:	${H}/hash.h
SWCmds.o:	${H}/database.h
SWCmds.o:	${H}/windows.h
SWCmds.o:	${H}/dbwind.h
SWCmds.o:	${H}/main.h
SWCmds.o:	commands.h
SWCmds.o:	${H}/textio.h
SWCmds.o:	${H}/txcommands.h
 
