SHELL=/bin/sh
#
# Makefile	sccsid @(#)Makefile	4.2 MAGIC (Berkeley) 8/5/85
#
CC  = cc
MODULE=sim
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}  
DFLAGS= `cat ../misc/DFLAGS`
CFLAGS= `cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

BIN=	${MAGICDIR}/bin

OBJS=	SimDBstuff.o SimSelect.o SimRsim.o SimExtract.o

POBJS=	SimDBstuff.po SimSelect.po SimRsim.po SimExtract.po

SRCS=	SimDBstuff.c SimSelect.c SimRsim.c SimExtract.c

HDRS=	sim.h
LINTFLAGS=${IFLAGS} -u 
LINTLIBS=${LIB}/lint/utils.ln ${LIB}/lint/tiles.ln ${LIB}/lint/textio.ln \
	${LIB}/lint/windows.ln ${LIB}/lint/drc.ln \
	${LIB}/lint/dbwind.ln ${LIB}/lint/tech.ln
ALLSRCS= ${SRCS} ${HDRS} Makefile

.SUFFIXES: .po


sim.o: ${HDRS} ${OBJS}
	rm -f sim.o
	ld -r ${OBJS} -o sim.o

sim.po: ${HDRS} ${POBJS}
	rm -f sim.po
	ld -r ${POBJS} -o sim.po

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

profile: installhdrs ${LIB}/lint/sim.ln

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/sim.ln 

installhdrs: ${H}/${MODULE}.h 

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -r -x ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/sim.po: sim.po
	rm -f ${LIB}/sim.po
	cp sim.po ${LIB}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}


${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

lint:	${SRCS} ${LINTLIBS}
	rm -f lint
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} > lint

clean:
	rm -f *.o *.po *.CKP *.BAK core
	
 
 
# dependency files for SimDBstuff.o
SimDBstuff.o: SimDBstuff.c 
SimDBstuff.o:  ${H}/magic.h
SimDBstuff.o:  ${H}/geometry.h
SimDBstuff.o:  ${H}/geofast.h
SimDBstuff.o:  ${H}/tile.h
SimDBstuff.o:  ${H}/hash.h
SimDBstuff.o:  ${H}/database.h
SimDBstuff.o:  ${H}/databaseInt.h
SimDBstuff.o:  ${H}/textio.h
SimDBstuff.o:  ${H}/signals.h
SimDBstuff.o:  ${H}/malloc.h
SimDBstuff.o:  ${H}/extractInt.h
SimDBstuff.o:  ./sim.h
SimDBstuff.o:  ${H}/windows.h
SimDBstuff.o:  ${H}/dbwind.h
SimDBstuff.o:  ${H}/commands.h
SimDBstuff.o:  ${H}/txcommands.h
SimDBstuff.o:  ${H}/styles.h
SimDBstuff.o:  ${H}/graphics.h
 
# dependency files for SimExtract.o
SimExtract.o: SimExtract.c 
SimExtract.o:  ${H}/magic.h
SimExtract.o:  ${H}/geometry.h
SimExtract.o:  ${H}/geofast.h
SimExtract.o:  ${H}/tile.h
SimExtract.o:  ${H}/hash.h
SimExtract.o:  ${H}/database.h
SimExtract.o:  ${H}/malloc.h
SimExtract.o:  ${H}/textio.h
SimExtract.o:  ${H}/debug.h
SimExtract.o:  ${H}/extract.h
SimExtract.o:  ${H}/extractInt.h
SimExtract.o:  ${H}/signals.h
SimExtract.o:  ${H}/windows.h
SimExtract.o:  ${H}/dbwind.h
SimExtract.o:  ${H}/styles.h
SimExtract.o:  ${H}/stack.h
SimExtract.o:  ./sim.h
 
# dependency files for SimRsim.o
SimRsim.o: SimRsim.c 
SimRsim.o:  ${H}/magic.h
SimRsim.o:  ${H}/stack.h
SimRsim.o:  ${H}/geometry.h
SimRsim.o:  ${H}/utils.h
SimRsim.o:  ${H}/tile.h
SimRsim.o:  ${H}/hash.h
SimRsim.o:  ${H}/database.h
SimRsim.o:  ${H}/signals.h
 
# dependency files for SimSelect.o
SimSelect.o: SimSelect.c 
SimSelect.o:  ${H}/magic.h
SimSelect.o:  ${H}/geometry.h
SimSelect.o:  ${H}/tile.h
SimSelect.o:  ${H}/hash.h
SimSelect.o:  ${H}/database.h
SimSelect.o:  ${H}/windows.h
SimSelect.o:  ${H}/dbwind.h
SimSelect.o:  ${H}/undo.h
SimSelect.o:  ${H}/commands.h
SimSelect.o:  ${H}/selInt.h
SimSelect.o:  ${H}/main.h
SimSelect.o:  ${H}/malloc.h
SimSelect.o:  ${H}/signals.h
SimSelect.o:  ./sim.h
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:11:58 PDT 1990.
 
# include file dependencies for "SimDBstuff.c"
SimDBstuff.o:	SimDBstuff.c
SimDBstuff.o:	${H}/magic.h
SimDBstuff.o:	${H}/geometry.h
SimDBstuff.o:	${H}/geofast.h
SimDBstuff.o:	${H}/tile.h
SimDBstuff.o:	${H}/hash.h
SimDBstuff.o:	${H}/database.h
SimDBstuff.o:	${H}/databaseInt.h
SimDBstuff.o:	${H}/textio.h
SimDBstuff.o:	${H}/signals.h
SimDBstuff.o:	${H}/malloc.h
SimDBstuff.o:	${H}/extractInt.h
SimDBstuff.o:	sim.h
SimDBstuff.o:	${H}/windows.h
SimDBstuff.o:	${H}/dbwind.h
SimDBstuff.o:	${H}/commands.h
SimDBstuff.o:	${H}/txcommands.h
SimDBstuff.o:	${H}/styles.h
SimDBstuff.o:	${H}/graphics.h
 
# include file dependencies for "SimExtract.c"
SimExtract.o:	SimExtract.c
SimExtract.o:	${H}/magic.h
SimExtract.o:	${H}/geometry.h
SimExtract.o:	${H}/geofast.h
SimExtract.o:	${H}/tile.h
SimExtract.o:	${H}/hash.h
SimExtract.o:	${H}/database.h
SimExtract.o:	${H}/malloc.h
SimExtract.o:	${H}/textio.h
SimExtract.o:	${H}/debug.h
SimExtract.o:	${H}/extract.h
SimExtract.o:	${H}/extractInt.h
SimExtract.o:	${H}/signals.h
SimExtract.o:	${H}/windows.h
SimExtract.o:	${H}/dbwind.h
SimExtract.o:	${H}/styles.h
SimExtract.o:	${H}/stack.h
SimExtract.o:	sim.h
 
# include file dependencies for "SimRsim.c"
SimRsim.o:	SimRsim.c
SimRsim.o:	${H}/magic.h
SimRsim.o:	${H}/stack.h
SimRsim.o:	${H}/geometry.h
SimRsim.o:	${H}/utils.h
SimRsim.o:	${H}/tile.h
SimRsim.o:	${H}/hash.h
SimRsim.o:	${H}/database.h
SimRsim.o:	${H}/signals.h
SimRsim.o:	${H}/styles.h
SimRsim.o:	${H}/windows.h
SimRsim.o:	${H}/dbwind.h
SimRsim.o:	sim.h
 
# include file dependencies for "SimSelect.c"
SimSelect.o:	SimSelect.c
SimSelect.o:	${H}/magic.h
SimSelect.o:	${H}/geometry.h
SimSelect.o:	${H}/tile.h
SimSelect.o:	${H}/hash.h
SimSelect.o:	${H}/database.h
SimSelect.o:	${H}/windows.h
SimSelect.o:	${H}/dbwind.h
SimSelect.o:	${H}/undo.h
SimSelect.o:	${H}/commands.h
SimSelect.o:	${H}/selInt.h
SimSelect.o:	${H}/main.h
SimSelect.o:	${H}/malloc.h
SimSelect.o:	${H}/signals.h
SimSelect.o:	sim.h
 
