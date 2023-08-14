SHELL=/bin/sh
#
# Makefile
# rcsid $Header: Makefile,v 6.0 90/08/29 13:12:54 mayo Exp $
#
# NOTE: the lists MOBJS and MPOBJS are those files that use MALLOC/FREE
#	in their macro forms.  These lists must be updated whenever any
#	new files use these macros, in order for the libmagictrace.a and
#	libmagictrace_p.a libraries to work properly.
#
MODULE=utils
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
IFLAGS=	-I. -I${H}
LINTFLAGS= ${DFLAGS} ${IFLAGS}
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}
RANLIB=`../:ranlib`


LINTSRCS=args.c doubleint.c finddisp.c flsbuf.c geometry.c getrect.c \
	hash.c heap.c \
	list.c lookup.c lookupany.c \
	lookupfull.c malloc.c match.c path.c pathvisit.c \
	printstuff.c stack.c strdup.c dqueue.c \
	runstats.c saveimage.c touchtypes.c
SRCS=	args.c doubleint.c finddisp.c flsbuf.c geometry.c getrect.c \
	hash.c heap.c \
	list.c lookup.c lookupany.c \
	lookupfull.c malloc.c match.c path.c pathvisit.c \
	printstuff.c stack.c strdup.c dqueue.c \
	runstats.c saveimage.c set.c show.c touchtypes.c \
	LIBmain.c LIBtextio.c
WSRCS=	whencefake.c

OBJS=	args.o doubleint.o finddisp.o flsbuf.o geometry.o getrect.o \
	hash.o heap.o \
	list.o lookup.o lookupany.o \
	lookupfull.o malloc.o match.o path.o pathvisit.o \
	printstuff.o stack.o strdup.o dqueue.o \
	runstats.o saveimage.o set.o show.o touchtypes.o 
MOBJS=	dqueue.mo hash.mo heap.mo list.mo malloc.mo stack.mo
LOBJS=	LIBmain.o LIBtextio.o
WOBJS=  whencefake.o

POBJS=	args.po doubleint.po finddisp.po flsbuf.po geometry.po getrect.po \
	hash.po heap.po \
	list.po lookup.po lookupany.po \
	lookupfull.po malloc.po match.po path.po pathvisit.po \
	printstuff.po stack.po strdup.po dqueue.po \
	runstats.po saveimage.po set.po show.po touchtypes.po 
MPOBJS=	dqueue.mpo hash.mpo heap.mpo list.mpo \
	malloc.mpo stack.mpo
LPOBJS=	LIBmain.po LIBtextio.po
WPOBJS=	whencefake.po

.SUFFIXES: .po .mo .mpo

.s.o:
	rm -f $*.o tmp.s tmp.o
	/lib/cpp ${DFLAGS} $*.s > tmp.s
	${CC} ${CFLAGS} -c tmp.s
	mv tmp.o $*.o
	rm -f tmp.s 

.c.o:
	rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

.c.po:
	rm -f $*.po
	-mv $*.o $*.so
	${CC} ${PCFLAGS} -c $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

#
# Modules compiled with -DMALLOCTRACE
#
.c.mo:
	rm -f $*.mo
	-mv $*.o $*.so
	${CC} ${CFLAGS} -DMALLOCTRACE -c $*.c
	mv $*.o $*.mo
	-mv $*.so $*.o

.c.mpo:
	rm -f $*.mpo
	-mv $*.o $*.so
	${CC} ${PCFLAGS} -DMALLOCTRACE -c $*.c
	mv $*.o $*.mpo
	-mv $*.so $*.o


HDRS=	doubleint.h geofast.h geometry.h hash.h heap.h \
	stack.h dqueue.h utils.h runstats.h touchingtypes.h malloc.h

utils.o: $(OBJS)
	rm -f utils.o
	rm -f LIBmain.o LIBtextio.o
	ld -r ${OBJS} -lm -o utils.o

utils.po: ${POBJS}
	rm -f utils.po
	ld -r ${POBJS} -o utils.po

libmagicutils.a: ${OBJS} ${LOBJS}
	-rm -f libmagicutils.a
	-ar rv libmagicutils.a ${OBJS} ${LOBJS}
	-${RANLIB} libmagicutils.a

MDOBJS=	`echo ${MOBJS} | sed "s,\.mo,.o,g"`
libmagictrace.a: ${OBJS} ${WOBJS} ${LOBJS} ${MOBJS}
	-rm -f libmagictrace.a
	-ar rv libmagictrace.a ${OBJS} ${WOBJS} ${LOBJS}
	-ar dv libmagictrace.a ${MDOBJS}
	-ar rv libmagictrace.a ${MOBJS}
	-${RANLIB} libmagictrace.a

libmagicutils_p.a: ${POBJS} ${LPOBJS}
	-ar rv libmagicutils_p.a ${POBJS} ${LPOBJS}
	-${RANLIB} libmagicutils_p.a

MDPOBJS=`echo ${MPOBJS} | sed "s,\.mpo,.po,g"`
libmagictrace_p.a: ${POBJS} ${WPOBJS} ${LPOBJS} ${MPOBJS}
	-ar rv libmagictrace_p.a ${POBJS} ${WPOBJS} ${LPOBJS}
	-ar dv libmagictrace_p.a ${MDPOBJS}
	-ar rv libmagictrace_p.a ${MPOBJS}
	-${RANLIB} libmagictrace_p.a

install: installhdrs ${LIB}/utils.o ${LIB}/lint/utils.ln \
	${LIB}/libmagicutils.a ${LIB}/libmagictrace.a

installhdrs: ${H}/${MODULE}.h ${H}/geofast.h ${H}/geometry.h \
	${H}/hash.h ${H}/heap.h ${H}/list.h \
	${H}/stack.h ${H}/dqueue.h ${H}/runstats.h \
	${H}/malloc.h ${H}/pathvisit.h ${H}/doubleint.h ${H}/touchingtypes.h

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o

lint:   forceit
	lint ${LINTFLAGS} ${SRCS} ${LINTLIBS} 

forceit:

profile: ${LIB}/utils.po ${LIB}/libmagicutils_p.a ${LIB}/libmagictrace_p.a

${LIB}/${MODULE}.o: ${MODULE}.o
	rm -f ${LIB}/${MODULE}.o
	ld -x -r utils.o -o ${LIB}/utils.o
	../:instclean ${MODULE} ${SRCS} Makefile READ_ME displays.proto

${LIB}/utils.po: utils.po
	rm -f ${LIB}/utils.po
	cp utils.po ${LIB}/utils.po

${LIB}/libmagicutils.a: libmagicutils.a
	rm -f ${LIB}/libmagicutils.a
	cp libmagicutils.a ${LIB}/libmagicutils.a
	${RANLIB} ${LIB}/libmagicutils.a

${LIB}/libmagictrace.a: libmagictrace.a
	rm -f ${LIB}/libmagictrace.a
	cp libmagictrace.a ${LIB}/libmagictrace.a
	${RANLIB} ${LIB}/libmagictrace.a

${LIB}/libmagicutils_p.a: libmagicutils_p.a
	rm -f ${LIB}/libmagicutils_p.a
	cp libmagicutils_p.a ${LIB}/libmagicutils_p.a
	${RANLIB} ${LIB}/libmagicutils_p.a

${LIB}/libmagictrace_p.a: libmagictrace_p.a
	rm -f ${LIB}/libmagictrace_p.a
	cp libmagictrace_p.a ${LIB}/libmagictrace_p.a
	${RANLIB} ${LIB}/libmagictrace_p.a

${H}/utils.h: utils.h
	rm -f ${H}/utils.h 
	cp utils.h ${H}

${H}/geofast.h: geofast.h
	rm -f ${H}/geofast.h 
	cp geofast.h ${H}

${H}/geometry.h: geometry.h
	rm -f ${H}/geometry.h 
	cp geometry.h ${H}

${H}/hash.h: hash.h
	rm -f ${H}/hash.h 
	cp hash.h ${H}

${H}/heap.h: heap.h
	rm -f ${H}/heap.h 
	cp heap.h ${H}
    
${H}/list.h: list.h
	rm -f ${H}/list.h 
	cp list.h ${H}

${H}/pathvisit.h: pathvisit.h
	rm -f ${H}/pathvisit.h 
	cp pathvisit.h ${H}

${H}/touchingtypes.h: touchingtypes.h
	rm -f ${H}/touchingtypes.h 
	cp touchingtypes.h ${H}

${H}/doubleint.h: doubleint.h
	rm -f ${H}/doubleint.h 
	cp doubleint.h ${H}

${H}/runstats.h: runstats.h
	rm -f ${H}/runstats.h 
	cp runstats.h ${H}

${H}/malloc.h: malloc.h
	rm -f ${H}/malloc.h 
	cp malloc.h ${H}

${H}/stack.h: stack.h
	rm -f ${H}/stack.h 
	cp stack.h ${H}

${H}/dqueue.h: dqueue.h
	rm -f ${H}/dqueue.h 
	cp dqueue.h ${H}

${LIB}/lint/${MODULE}.ln: ${SRCS}
	../:lintlib ${MODULE} ${SRCS}


# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:12:36 PDT 1990.
 
# include file dependencies for "LIBmain.c"
LIBmain.o:	LIBmain.c
 
# include file dependencies for "LIBtextio.c"
LIBtextio.o:	LIBtextio.c
 
# include file dependencies for "args.c"
args.o:	args.c
args.o:	${H}/magic.h
args.o:	utils.h
 
# include file dependencies for "doubleint.c"
doubleint.o:	doubleint.c
doubleint.o:	${H}/magic.h
doubleint.o:	${H}/textio.h
doubleint.o:	doubleint.h
 
# include file dependencies for "doubletest.c"
doubletest.o:	doubletest.c
doubletest.o:	${H}/magic.h
doubletest.o:	doubleint.h
 
# include file dependencies for "dqueue.c"
dqueue.o:	dqueue.c
dqueue.o:	${H}/magic.h
dqueue.o:	dqueue.h
dqueue.o:	malloc.h
 
# include file dependencies for "finddisp.c"
finddisp.o:	finddisp.c
finddisp.o:	utils.h
 
# include file dependencies for "flsbuf.c"
flsbuf.o:	flsbuf.c
 
# include file dependencies for "geometry.c"
geometry.o:	geometry.c
geometry.o:	${H}/magic.h
geometry.o:	geometry.h
geometry.o:	utils.h
geometry.o:	${H}/textio.h
 
# include file dependencies for "getrect.c"
getrect.o:	getrect.c
getrect.o:	${H}/magic.h
getrect.o:	geometry.h
 
# include file dependencies for "hash.c"
hash.o:	hash.c
hash.o:	${H}/magic.h
hash.o:	hash.h
hash.o:	malloc.h
 
# include file dependencies for "heap.c"
heap.o:	heap.c
heap.o:	${H}/magic.h
heap.o:	doubleint.h
heap.o:	heap.h
heap.o:	utils.h
heap.o:	malloc.h
 
# include file dependencies for "list.c"
list.o:	list.c
list.o:	${H}/magic.h
list.o:	utils.h
list.o:	malloc.h
list.o:	list.h
 
# include file dependencies for "lookup.c"
lookup.o:	lookup.c
lookup.o:	utils.h
 
# include file dependencies for "lookupany.c"
lookupany.o:	lookupany.c
 
# include file dependencies for "lookupfull.c"
lookupfull.o:	lookupfull.c
 
# include file dependencies for "malloc.c"
malloc.o:	malloc.c
malloc.o:	${H}/magic.h
malloc.o:	malloc.h
 
# include file dependencies for "match.c"
match.o:	match.c
match.o:	${H}/magic.h
match.o:	${H}/textio.h
 
# include file dependencies for "path.c"
path.o:	path.c
path.o:	${H}/magic.h
path.o:	hash.h
path.o:	malloc.h
 
# include file dependencies for "pathvisit.c"
pathvisit.o:	pathvisit.c
pathvisit.o:	${H}/magic.h
pathvisit.o:	hash.h
pathvisit.o:	malloc.h
pathvisit.o:	pathvisit.h
pathvisit.o:	utils.h
 
# include file dependencies for "printstuff.c"
printstuff.o:	printstuff.c
printstuff.o:	${H}/magic.h
printstuff.o:	geometry.h
 
# include file dependencies for "runstats.c"
runstats.o:	runstats.c
runstats.o:	${H}/magic.h
runstats.o:	runstats.h
 
# include file dependencies for "saveimage.c"
saveimage.o:	saveimage.c
saveimage.o:	${H}/magic.h
 
# include file dependencies for "set.c"
set.o:	set.c
set.o:	utils.h
set.o:	${H}/magic.h
set.o:	geometry.h
set.o:	${H}/tile.h
set.o:	hash.h
set.o:	${H}/database.h
set.o:	doubleint.h
set.o:	list.h
 
# include file dependencies for "show.c"
show.o:	show.c
show.o:	${H}/magic.h
show.o:	geometry.h
show.o:	${H}/graphics.h
show.o:	${H}/windows.h
show.o:	hash.h
show.o:	${H}/tile.h
show.o:	${H}/database.h
show.o:	${H}/dbwind.h
 
# include file dependencies for "stack.c"
stack.o:	stack.c
stack.o:	${H}/magic.h
stack.o:	utils.h
stack.o:	stack.h
stack.o:	malloc.h
 
# include file dependencies for "strdup.c"
strdup.o:	strdup.c
strdup.o:	${H}/magic.h
strdup.o:	malloc.h
 
# include file dependencies for "touchtypes.c"
touchtypes.o:	touchtypes.c
touchtypes.o:	${H}/magic.h
touchtypes.o:	geometry.h
touchtypes.o:	geofast.h
touchtypes.o:	${H}/tile.h
touchtypes.o:	hash.h
touchtypes.o:	${H}/database.h
 
# include file dependencies for "whencefake.c"
whencefake.o:	whencefake.c
 
