SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.0 90/08/29 11:40:12 mayo Exp $
#

MODULE=cif
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

SRCS=	CIFgen.c CIFhier.c CIFmain.c CIFrdcl.c CIFrdpt.c \
    	CIFrdpoly.c CIFrdtech.c CIFrdutils.c CIFsee.c CIFtech.c \
    	CIFwrite.c
OBJS=	CIFgen.o CIFhier.o CIFmain.o CIFrdcl.o CIFrdpt.o \
    	CIFrdpoly.o CIFrdtech.o CIFrdutils.o CIFsee.o CIFtech.o \
	CIFwrite.o
POBJS=	CIFgen.po CIFhier.po CIFmain.po CIFrdcl.po CIFrdpt.po \
    	CIFrdpoly.po CIFrdtech.po CIFrdutils.po CIFsee.po CIFtech.po \
	CIFwrite.po
ALLSRCS= ${SRCS} cif.h CIFint.h Makefile

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}

LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/database.ln ${LIB}/lint/drc.ln \
	${LIB}/lint/textio.ln ${LIB}/lint/tiles.ln ${LIB}/lint/utils.ln \
	${LIB}/lint/tech.ln ${LIB}/lint/undo.ln
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

installhdrs: ${H}/${MODULE}.h ${H}/CIFint.h ${H}/CIFread.h


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

${H}/CIFint.h: CIFint.h
	rm -f ${H}/CIFint.h 
	cp CIFint.h ${H}

${H}/CIFread.h: CIFread.h
	rm -f ${H}/CIFread.h 
	cp CIFread.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}

tags:	${SRCS}
	ctags ${SRCS}

 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:40:03 PDT 1990.
 
# include file dependencies for "CIFBasic.c"
CIFBasic.o:	CIFBasic.c
 
# include file dependencies for "CIFgen.c"
CIFgen.o:	CIFgen.c
CIFgen.o:	${H}/magic.h
CIFgen.o:	${H}/geometry.h
CIFgen.o:	${H}/tile.h
CIFgen.o:	${H}/hash.h
CIFgen.o:	${H}/database.h
CIFgen.o:	CIFint.h
 
# include file dependencies for "CIFhier.c"
CIFhier.o:	CIFhier.c
CIFhier.o:	${H}/magic.h
CIFhier.o:	${H}/geometry.h
CIFhier.o:	${H}/tile.h
CIFhier.o:	${H}/hash.h
CIFhier.o:	${H}/database.h
CIFhier.o:	CIFint.h
CIFhier.o:	cif.h
CIFhier.o:	${H}/drc.h
CIFhier.o:	${H}/textio.h
CIFhier.o:	${H}/undo.h
CIFhier.o:	${H}/malloc.h
CIFhier.o:	${H}/signals.h
 
# include file dependencies for "CIFmain.c"
CIFmain.o:	CIFmain.c
CIFmain.o:	${H}/magic.h
CIFmain.o:	${H}/geometry.h
CIFmain.o:	${H}/tile.h
CIFmain.o:	${H}/hash.h
CIFmain.o:	${H}/database.h
CIFmain.o:	CIFint.h
CIFmain.o:	${H}/textio.h
CIFmain.o:	${H}/windows.h
CIFmain.o:	${H}/dbwind.h
CIFmain.o:	${H}/styles.h
 
# include file dependencies for "CIFrdcl.c"
CIFrdcl.o:	CIFrdcl.c
CIFrdcl.o:	${H}/magic.h
CIFrdcl.o:	${H}/geometry.h
CIFrdcl.o:	${H}/tile.h
CIFrdcl.o:	${H}/hash.h
CIFrdcl.o:	${H}/database.h
CIFrdcl.o:	CIFint.h
CIFrdcl.o:	CIFread.h
CIFrdcl.o:	${H}/utils.h
CIFrdcl.o:	${H}/windows.h
CIFrdcl.o:	${H}/dbwind.h
CIFrdcl.o:	${H}/main.h
CIFrdcl.o:	${H}/drc.h
 
# include file dependencies for "CIFrdpoly.c"
CIFrdpoly.o:	CIFrdpoly.c
CIFrdpoly.o:	${H}/magic.h
CIFrdpoly.o:	${H}/geometry.h
CIFrdpoly.o:	${H}/tile.h
CIFrdpoly.o:	${H}/hash.h
CIFrdpoly.o:	${H}/database.h
CIFrdpoly.o:	CIFint.h
CIFrdpoly.o:	CIFread.h
CIFrdpoly.o:	${H}/malloc.h
 
# include file dependencies for "CIFrdpt.c"
CIFrdpt.o:	CIFrdpt.c
CIFrdpt.o:	${H}/magic.h
CIFrdpt.o:	${H}/geometry.h
CIFrdpt.o:	${H}/tile.h
CIFrdpt.o:	${H}/hash.h
CIFrdpt.o:	${H}/database.h
CIFrdpt.o:	${H}/windows.h
CIFrdpt.o:	${H}/main.h
CIFrdpt.o:	CIFint.h
CIFrdpt.o:	CIFread.h
 
# include file dependencies for "CIFrdtech.c"
CIFrdtech.o:	CIFrdtech.c
CIFrdtech.o:	${H}/magic.h
CIFrdtech.o:	${H}/geometry.h
CIFrdtech.o:	${H}/tile.h
CIFrdtech.o:	${H}/hash.h
CIFrdtech.o:	${H}/database.h
CIFrdtech.o:	${H}/tech.h
CIFrdtech.o:	${H}/textio.h
CIFrdtech.o:	${H}/utils.h
CIFrdtech.o:	CIFint.h
CIFrdtech.o:	CIFread.h
CIFrdtech.o:	${H}/calmaInt.h
CIFrdtech.o:	${H}/malloc.h
 
# include file dependencies for "CIFrdutils.c"
CIFrdutils.o:	CIFrdutils.c
CIFrdutils.o:	${H}/magic.h
CIFrdutils.o:	${H}/geometry.h
CIFrdutils.o:	${H}/tile.h
CIFrdutils.o:	${H}/hash.h
CIFrdutils.o:	${H}/database.h
CIFrdutils.o:	CIFint.h
CIFrdutils.o:	CIFread.h
CIFrdutils.o:	${H}/textio.h
CIFrdutils.o:	${H}/signals.h
CIFrdutils.o:	${H}/undo.h
CIFrdutils.o:	${H}/malloc.h
 
# include file dependencies for "CIFsee.c"
CIFsee.o:	CIFsee.c
CIFsee.o:	${H}/magic.h
CIFsee.o:	${H}/geometry.h
CIFsee.o:	${H}/graphics.h
CIFsee.o:	${H}/tile.h
CIFsee.o:	${H}/hash.h
CIFsee.o:	${H}/database.h
CIFsee.o:	${H}/windows.h
CIFsee.o:	${H}/dbwind.h
CIFsee.o:	${H}/styles.h
CIFsee.o:	CIFint.h
CIFsee.o:	${H}/textio.h
CIFsee.o:	${H}/undo.h
 
# include file dependencies for "CIFtech.c"
CIFtech.o:	CIFtech.c
CIFtech.o:	${H}/magic.h
CIFtech.o:	${H}/geometry.h
CIFtech.o:	${H}/tile.h
CIFtech.o:	${H}/hash.h
CIFtech.o:	${H}/database.h
CIFtech.o:	${H}/tech.h
CIFtech.o:	${H}/utils.h
CIFtech.o:	CIFint.h
CIFtech.o:	${H}/calmaInt.h
CIFtech.o:	${H}/textio.h
CIFtech.o:	${H}/malloc.h
 
# include file dependencies for "CIFwrite.c"
CIFwrite.o:	CIFwrite.c
CIFwrite.o:	${H}/magic.h
CIFwrite.o:	${H}/geometry.h
CIFwrite.o:	${H}/tile.h
CIFwrite.o:	${H}/utils.h
CIFwrite.o:	${H}/hash.h
CIFwrite.o:	${H}/database.h
CIFwrite.o:	${H}/databaseInt.h
CIFwrite.o:	${H}/tech.h
CIFwrite.o:	${H}/stack.h
CIFwrite.o:	cif.h
CIFwrite.o:	CIFint.h
CIFwrite.o:	${H}/signals.h
CIFwrite.o:	${H}/windows.h
CIFwrite.o:	${H}/dbwind.h
CIFwrite.o:	${H}/styles.h
CIFwrite.o:	${H}/textio.h
 
