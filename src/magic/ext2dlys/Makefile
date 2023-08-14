SHELL=/bin/sh
#
# Makefile
#
CADDIR 	= /udir/mayo/cadhome
MAGICDIR= ${CADDIR}/src/magic
H=	../include
LIB=	../lib
IFLAGS=	-I. -I${H}
MACHINEFLAGS= -DSUN2
LINTFLAGS= ${MACHINEFLAGS} ${IFLAGS}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}


SRCS	= ext2dlys.c
OBJS	= ext2dlys.o
POBJS	= ext2dlys.po
LIBS	= ${LIB}/libextflat.a ${LIB}/libmagicutils.a
PLIBS	= ${LIB}/libextflat_p.a ${LIB}/libmagicutils_p.a

ext2dlys: ${OBJS}
	${CC} ${CFLAGS} ${OBJS} ${LIBS} -o ext2dlys -lm

pext2dlys: ${POBJS}
	rm -f pext2dlys
	${CC} ${PCFLAGS} ${POBJS} ${PLIBS} -o pext2dlys -lm

install: ${CADDIR}/bin/ext2dlys


${CADDIR}/bin/ext2dlys: ext2dlys
	-rm -f ${CADDIR}/bin/ext2dlys
	cp ext2dlys ${CADDIR}/bin/ext2dlys

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

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
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:13:37 PDT 1990.
 
# include file dependencies for "ext2dlys.c"
ext2dlys.o:	ext2dlys.c
ext2dlys.o:	${H}/magic.h
ext2dlys.o:	${H}/hash.h
ext2dlys.o:	${H}/utils.h
ext2dlys.o:	${H}/geometry.h
ext2dlys.o:	${H}/malloc.h
ext2dlys.o:	${H}/extflat.h
 
