SHELL=/bin/sh
#
# rcsid="$Header: Makefile,v 6.1 90/08/31 11:34:08 mayo Exp $"
#
MODULE=main
CADDIR  =`../:findcad`
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin
IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS=${IFLAGS} -u
LINTDIR=${LIB}/lint

# Lint main.c and with every module except main.
LINTLIBS= ${LINTDIR}/cif.ln ${LINTDIR}/cmwind.ln \
    ${LINTDIR}/commands.ln ${LINTDIR}/database.ln ${LINTDIR}/dbwind.ln \
    ${LINTDIR}/debug.ln ${LINTDIR}/drc.ln ${LINTDIR}/extract.ln \
    ${LINTDIR}/gcr.ln ${LINTDIR}/graphics.ln ${LINTDIR}/grouter.ln \
    ${LINTDIR}/macros.ln \
    ${LINTDIR}/misc.ln ${LINTDIR}/netmenu.ln ${LINTDIR}/parser.ln \
    ${LINTDIR}/router.ln ${LINTDIR}/signals.ln \
    ${LINTDIR}/tech.ln ${LINTDIR}/textio.ln ${LINTDIR}/tiles.ln \
    ${LINTDIR}/undo.ln ${LINTDIR}/utils.ln ${LINTDIR}/windows.ln

SRCS=	main.c 
OBJS=	main.o
POBJS=	main.po
ALLSRCS= ${SRCS} Makefile

.SUFFIXES: .po

all:	main.o

${LIB}/main.po: main.po
	rm -f ${LIB}/main.po
	cp main.po ${LIB}/main.po

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

install: installhdrs ${LIB}/main.o ${LIB}/lint/main.ln

installhdrs: ${H}/${MODULE}.h

profile: ${LIB}/main.po ${H}/main.h ${LIB}/lint/main.ln

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -r -x ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:08:55 PDT 1990.
 
# include file dependencies for "main.c"
main.o:	main.c
main.o:	${H}/magic.h
main.o:	${H}/hash.h
main.o:	${H}/textio.h
main.o:	${H}/geometry.h
main.o:	${H}/txcommands.h
main.o:	${H}/graphics.h
main.o:	${H}/tile.h
main.o:	${H}/tech.h
main.o:	${H}/database.h
main.o:	${H}/drc.h
main.o:	${H}/windows.h
main.o:	${H}/dbwind.h
main.o:	${H}/commands.h
main.o:	${H}/signals.h
main.o:	${H}/utils.h
main.o:	${H}/runstats.h
main.o:	${H}/cif.h
main.o:	${H}/router.h
main.o:	${H}/extract.h
main.o:	${H}/undo.h
main.o:	${H}/netmenu.h
main.o:	${H}/plow.h
main.o:	${H}/paths.h
main.o:	${H}/wiring.h
main.o:	${H}/plot.h
main.o:	${H}/sim.h
main.o:	${H}/doubleint.h
main.o:	${H}/list.h
main.o:	${H}/mzrouter.h
 
