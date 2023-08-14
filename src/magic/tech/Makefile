SHELL=/bin/sh
#
# rcsid = "$Header: Makefile,v 6.0 90/08/29 11:39:34 mayo Exp $";
#
CADDIR= /udir/mayo/cadhome
MODULE=	tech
VERSION=26
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
SYSLIB= ${CADDIR}/lib/magic/sys
IFLAGS=	-I. -I${H}/
DFLAGS=	`cat ../misc/DFLAGS` -DVERSION=${VERSION}
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}
OBJS=	${MODULE}.o
SRCS=	${MODULE}.c
LINTFLAGS=${IFLAGS} -u -DVERSION=${VERSION}
LINTLIBS=${LIB}/lint/utils.ln ${LIB}/lint/tiles.ln ${LIB}/lint/database.ln \
	 ${LIB}/lint/lintfix.c
ALLSRCS= ${SRCS} Makefile nmos.tech

${MODULE}.o: ${MODULE}.c

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

install: installhdrs ${LIB}/${MODULE}.o ${LIB}/lint/${MODULE}.ln  

installcad: $(SYSLIB)/nmos.tech${VERSION} $(SYSLIB)/scmos.tech${VERSION} 

installhdrs: ${H}/${MODULE}.h

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -x -r ${MODULE}.o -o ${LIB}/${MODULE}.o
	../:instclean ${MODULE} ${SRCS} Makefile *.tech :techinstall

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} -DVERSION=${VERSION} ${SRCS}

${H}/${MODULE}.h: ${MODULE}.h
	rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

$(SYSLIB)/nmos.tech${VERSION}: nmos.tech
	csh -f :techinstall nmos.tech ${VERSION} $(SYSLIB)

$(SYSLIB)/scmos.tech${VERSION}: scmos.tech
	csh -f :techinstall scmos.tech ${VERSION} $(SYSLIB)

$(SYSLIB)/bipolar.tech${VERSION}: bipolar.tech
	csh -f :techinstall bipolar.tech ${VERSION} $(SYSLIB)

POBJS=	${MODULE}.po

.SUFFIXES: .po

.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

profile: installhdrs ${LIB}/${MODULE}.po ${LIB}/lint/${MODULE}.ln

${LIB}/${MODULE}.po: ${MODULE}.po
	rm -f ${LIB}/${MODULE}.po
	cp ${MODULE}.po ${LIB}

 
# Force tech.o to be remade when we bump the version number
tech.o: Makefile
Makefile:

 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:39:30 PDT 1990.
 
# include file dependencies for "tech.c"
tech.o:	tech.c
tech.o:	${H}/magic.h
tech.o:	${H}/geometry.h
tech.o:	${H}/utils.h
tech.o:	tech.h
tech.o:	${H}/textio.h
tech.o:	${H}/graphics.h
tech.o:	${H}/malloc.h
 
