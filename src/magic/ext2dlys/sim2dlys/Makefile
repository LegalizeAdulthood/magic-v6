#
# Makefile
#
MAGICDIR=/bi/cad/src/magic
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
MACHINEFLAGS= -DSUN2
LINTFLAGS= ${MACHINEFLAGS} ${IFLAGS}
CFLAGS= -g -DSUN2   ${MACHINEFLAGS} ${IFLAGS} -DUNIX43
PCFLAGS= -g -DSUN2 ${IFLAGS} -pg

LIBUTILS= ${MAGICDIR}/lib/libmagicutils.a

sim2dlys:	sim2dlys.o
	${CC} sim2dlys.o ${LIBUTILS} -o sim2dlys
 
 
# include file dependencies for "sim2dlys.c"
sim2dlys.o:	sim2dlys.c
sim2dlys.o:	${H}/magic.h
sim2dlys.o:	${H}/hash.h
sim2dlys.o:	${H}/utils.h
sim2dlys.o:	${H}/malloc.h
