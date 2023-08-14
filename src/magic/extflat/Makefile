SHELL=/bin/sh
#
# rcsid "$Header: Makefile,v 6.0 90/08/29 11:42:13 mayo Exp $"
#

MODULE=extflat
CADDIR  =`../:findcad`
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
BIN=	${MAGICDIR}/bin

HDRS=	EFint.h extflat.h
SRCS=	EFargs.c EFbuild.c EFdef.c EFflat.c EFhier.c \
	EFname.c EFread.c EFsym.c EFvisit.c
OBJS=	EFargs.o EFbuild.o EFdef.o EFflat.o EFhier.o \
	EFname.o EFread.o EFsym.o EFvisit.o
POBJS=	EFargs.po EFbuild.po EFdef.po EFflat.po EFhier.po \
	EFname.po EFread.po EFsym.po EFvisit.po

IFLAGS=	-I. -I${H}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}
RANLIB=`../:ranlib`


LINTFLAGS= ${IFLAGS} -u
LINTLIBS= ${LIB}/lint/utils.ln

${MODULE}.o: libextflat.a

libextflat.a: ${OBJS}
	-rm -f libextflat.a
	-ar rv libextflat.a ${OBJS}
	-${RANLIB} libextflat.a

libextflat_p.a: ${POBJS}
	-rm -f libextflat.a
	-ar rv libextflat_p.a ${POBJS}
	-${RANLIB} libextflat_p.a

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

install: ${LIB}/libextflat.a 

installhdrs: ${H}/extflat.h ${H}/EFint.h

${H}/extflat.h: extflat.h
	rm -f ${H}/extflat.h 
	cp extflat.h ${H}/extflat.h

${H}/EFint.h: EFint.h
	rm -f ${H}/EFint.h 
	cp EFint.h ${H}/EFint.h

${LIB}/libextflat.a: libextflat.a
	rm -f ${LIB}/libextflat.a
	cp libextflat.a ${LIB}/libextflat.a
	${RANLIB} ${LIB}/libextflat.a
	../:instclean ${MODULE} ${SRCS} Makefile

${LIB}/libextflat_p.a: libextflat_p.a
	rm -f ${LIB}/libextflat_p.a
	cp libextflat_p.a ${LIB}/libextflat_p.a
	${RANLIB} ${LIB}/libextflat_p.a

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

.SUFFIXES: .po

.c.po:
	-mv $*.o $*.so
	${CC} ${PCFLAGS} -c $*.c
	rm -f $*.po
	mv $*.o $*.po
	-mv $*.so $*.o
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 11:42:06 PDT 1990.
 
# include file dependencies for "EFargs.c"
EFargs.o:	EFargs.c
EFargs.o:	${H}/magic.h
EFargs.o:	${H}/paths.h
EFargs.o:	${H}/geometry.h
EFargs.o:	${H}/hash.h
EFargs.o:	${H}/utils.h
EFargs.o:	${H}/malloc.h
EFargs.o:	${H}/pathvisit.h
EFargs.o:	extflat.h
EFargs.o:	EFint.h
 
# include file dependencies for "EFbuild.c"
EFbuild.o:	EFbuild.c
EFbuild.o:	${H}/magic.h
EFbuild.o:	${H}/geometry.h
EFbuild.o:	${H}/hash.h
EFbuild.o:	${H}/utils.h
EFbuild.o:	${H}/malloc.h
EFbuild.o:	extflat.h
EFbuild.o:	EFint.h
 
# include file dependencies for "EFdef.c"
EFdef.o:	EFdef.c
EFdef.o:	${H}/magic.h
EFdef.o:	${H}/geometry.h
EFdef.o:	${H}/hash.h
EFdef.o:	${H}/utils.h
EFdef.o:	${H}/malloc.h
EFdef.o:	extflat.h
EFdef.o:	EFint.h
 
# include file dependencies for "EFflat.c"
EFflat.o:	EFflat.c
EFflat.o:	${H}/magic.h
EFflat.o:	${H}/geometry.h
EFflat.o:	${H}/geofast.h
EFflat.o:	${H}/hash.h
EFflat.o:	${H}/malloc.h
EFflat.o:	${H}/utils.h
EFflat.o:	extflat.h
EFflat.o:	EFint.h
 
# include file dependencies for "EFhier.c"
EFhier.o:	EFhier.c
EFhier.o:	${H}/magic.h
EFhier.o:	${H}/geometry.h
EFhier.o:	${H}/geofast.h
EFhier.o:	${H}/hash.h
EFhier.o:	${H}/malloc.h
EFhier.o:	${H}/utils.h
EFhier.o:	extflat.h
EFhier.o:	EFint.h
 
# include file dependencies for "EFname.c"
EFname.o:	EFname.c
EFname.o:	${H}/magic.h
EFname.o:	${H}/geometry.h
EFname.o:	${H}/geofast.h
EFname.o:	${H}/hash.h
EFname.o:	${H}/malloc.h
EFname.o:	${H}/utils.h
EFname.o:	extflat.h
EFname.o:	EFint.h
 
# include file dependencies for "EFread.c"
EFread.o:	EFread.c
EFread.o:	${H}/magic.h
EFread.o:	${H}/geometry.h
EFread.o:	${H}/hash.h
EFread.o:	${H}/utils.h
EFread.o:	extflat.h
EFread.o:	EFint.h
EFread.o:	${H}/paths.h
 
# include file dependencies for "EFsym.c"
EFsym.o:	EFsym.c
EFsym.o:	${H}/magic.h
EFsym.o:	${H}/geometry.h
EFsym.o:	${H}/geofast.h
EFsym.o:	${H}/hash.h
EFsym.o:	${H}/malloc.h
EFsym.o:	${H}/utils.h
EFsym.o:	extflat.h
EFsym.o:	EFint.h
 
# include file dependencies for "EFvisit.c"
EFvisit.o:	EFvisit.c
EFvisit.o:	${H}/magic.h
EFvisit.o:	${H}/geometry.h
EFvisit.o:	${H}/geofast.h
EFvisit.o:	${H}/hash.h
EFvisit.o:	${H}/malloc.h
EFvisit.o:	${H}/utils.h
EFvisit.o:	extflat.h
EFvisit.o:	EFint.h
 
