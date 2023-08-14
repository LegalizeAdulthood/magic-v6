SHELL=/bin/sh
#
# rcsid $Header: Makefile,v 6.5 90/09/13 10:31:12 mayo Exp $
#

MODULE=	graphics
CADDIR = /foo/bla/defined_by_caller
MAGICDIR=..
H=	${MAGICDIR}/include
LIB=	${MAGICDIR}/lib
SYSLIB=	${CADDIR}/lib/magic/sys
BIN=	${CADDIR}/bin
IFLAGS=	-I. -I${H} -I/usr/local/openwin/include
#
# Configuration for flags and libraries
#
DFLAGS=	`cat ../misc/DFLAGS`
CFLAGS=	`cat ../misc/CFLAGS` ${DFLAGS} ${IFLAGS}
PCFLAGS= `cat ../misc/PCFLAGS` ${DFLAGS} ${IFLAGS}
LINTFLAGS=${IFLAGS} ${DFLAGS} -u
LINTLIBS=${LIB}/lint/utils.ln ${LIB}/lint/textio.ln 
LIBS=`cat ../magic/LIBS`
#
# Configuration to include only specific graphics drivers
#
SRCS= ${BASE_SRCS} ${X11_SRCS}
OBJS= ${BASE_OBJS} ${X11_OBJS}
POBJS= ${BASE_POBJS} ${X11_POBJS}
HELPER_SRCS= ${X11HELPER_SRCS}
HELPER_PROGS= ${X11HELP_PROG}
#
# Sets of source files.
# One line per graphics driver.
#
BASE_SRCS= grMain.c grLock.c grDStyle.c grText.c grCMap.c grClip.c \
	grGlyphs.c grNull.c 
SUN_SRCS= grSunW1.c grSunW2.c grSunW3.c grSunW4.c grSunW5.c 
X10_SRCS= grX10a1.c grX10a2.c grX10a3.c grX10a4.c grX10a5.c 
X11_SRCS= grX11su1.c grX11su2.c grX11su3.c grX11su4.c grX11su5.c
SUNOLD_SRCS= grSun1.c grSun2.c grSun3.c grSun4.c grSun5.c
AED_SRCS= grAed1.c grAed2.c grAed3.c grAed4.c grAed5.c

# Sets of object files
BASE_OBJS= grMain.o grLock.o grDStyle.o grText.o grCMap.o grClip.o \
	grGlyphs.o grNull.o 
SUN_OBJS= grSunW1.o grSunW2.o grSunW3.o grSunW4.o grSunW5.o 
X10_OBJS= grX10a1.o grX10a2.o grX10a3.o grX10a4.o grX10a5.o 
X11_OBJS= grX11su1.o grX11su2.o grX11su3.o grX11su4.o grX11su5.o
SUNOLD_OBJS= grSun1.o grSun2.o grSun3.o grSun4.o grSun5.o
AED_OBJS= grAed1.o grAed2.o grAed3.o grAed4.o grAed5.o

# Sets of profiled object files
BASE_POBJS= grMain.po grLock.po grDStyle.po grText.po grCMap.po grClip.po \
	grGlyphs.po grNull.po 
SUN_POBJS= grSunW1.po grSunW2.po grSunW3.po grSunW4.po grSunW5.po 
X10_POBJS= grX10a1.po grX10a2.po grX10a3.po grX10a4.po grX10a5.po 
X11_POBJS= grX11su1.po grX11su2.po grX11su3.po grX11su4.po grX11su5.po
SUNOLD_POBJS= grSun1.po grSun2.po grSun3.po grSun4.po grSun5.po
AED_POBJS= grAed1.po grAed2.po grAed3.po grAed4.po grAed5.po

# Sets of sources for "helper" programs
SUNHELPER_SRCS= grSunProg.c 
SUNHELP_PROG= grSunProg 
X10HELPER_SRCS= X10helper.c 
X10HELP_PROG= X10helper 
X11HELPER_SRCS= X11Helper.c
X11HELP_PROG= X11Helper 

# all Aed sources (the masters for the lint library)
AEDSOURCES= ${AED_SRCS}

# The lint library masters, converted from "Aed" to "Gr" where necessary.
LINTLIBSOURCES= grMain.c grLock.c grCMap.c grDStyle.c grClip.c grGlyphs.c \
	grLint1.c grLint2.c grLint3.c grLint4.c grLint5.c 

# NOTE:  The grLint* files are automatically generated and may be thrown away.
# NOTE:  We don't compile the grLint*.c files.

ALLSRCS= ${SRCS} ${HELPER_SRCS} Makefile :makeforlint

.SUFFIXES: .po

all: graphics.o ${HELPER_PROGS}

# -x option on ld below strips some symbols to avoid a dbx bug that otherwise
# causes dbx to choke when loading symbols for the graphics module.
graphics.o: ${OBJS}
	/bin/rm -f graphics.o
	ld -r -x ${OBJS} -o graphics.o

graphics.po: ${POBJS}
	/bin/rm -f graphics.po
	ld -r ${POBJS} -o graphics.po

grSunProg:  grSunProg.c 
	/bin/rm -f grSunProg
	${CC} ${CFLAGS} grSunProg.c ${LIBS} -o grSunProg

X10helper:  X10helper.c 
	/bin/rm -f X10helper
	${CC} ${CFLAGS} X10helper.c ${LIBS} -o X10helper

X11Helper:  X11Helper.c 
	/bin/rm -f X11Helper
	${CC} ${CFLAGS} X11Helper.c ${LIBS} -o X11Helper

.c.po:
	/bin/rm -f $*.po
	-mv $*.o $*.so
	${CC} -c ${PCFLAGS} $*.c
	mv $*.o $*.po
	-mv $*.so $*.o

.c.o:
	/bin/rm -f $*.o
	${CC} ${CFLAGS} -c $*.c

clean:	
	rm -f ${OBJS} ${POBJS} ${MODULE}.o
	rm -f ${SUNHELP_PROG} ${X10HELP_PROG} ${X11HELP_PROG}

# all Aed sources (the masters for the lint library)

lint:   forceit
	csh :makeforlint ${AEDSOURCES}
	lint ${LINTFLAGS} ${LINTLIBSOURCES} ${LINTLIBS}

forceit:

profile: installhdrs graphics.po
	/bin/rm -f ${LIB}/graphics.po
	cp graphics.po ${LIB}/graphics.po

install: installhdrs ${LIB}/graphics.o ${HELPER_PROGS} ${LIB}/lint/graphics.ln

installcad: installhelp \
	${SYSLIB}/mos.7bit.dstyle5 \
	${SYSLIB}/mos.6bit.dstyle5 \
	${SYSLIB}/mos.4bit.dstyle5 \
	${SYSLIB}/mos.bw.dstyle5 \
	${SYSLIB}/mos.7bit.std.cmap1 \
	${SYSLIB}/mos.6bit.std.cmap1 \
	${SYSLIB}/mos.4bit.std.cmap1 \
	${SYSLIB}/bw.glyphs \
	${SYSLIB}/color.glyphs

installhelp:
	@(for i in ${HELPER_PROGS}; \
	  do \
		make ${MFLAGS} CADDIR=${CADDIR} ${BIN}/$$i; \
	  done)

installhdrs: ${H}/${MODULE}.h ${H}/graphicsInt.h ${H}/glyphs.h \
	${H}/grSunInt.h ${H}/grSunWInt.h ${H}/grAedInt.h ${H}/grX10aInt.h 

${H}/${MODULE}.h: ${MODULE}.h
	/bin/rm -f ${H}/${MODULE}.h 
	cp ${MODULE}.h ${H}

${H}/graphicsInt.h: graphicsInt.h
	/bin/rm -f ${H}/graphicsInt.h 
	cp graphicsInt.h ${H}

${H}/glyphs.h: glyphs.h
	/bin/rm -f ${H}/glyphs.h 
	cp glyphs.h ${H}

${H}/grSunInt.h: grSunInt.h
	/bin/rm -f ${H}/grSunInt.h 
	cp grSunInt.h ${H}

${H}/grSunWInt.h: grSunWInt.h
	/bin/rm -f ${H}/grSunWInt.h 
	cp grSunWInt.h ${H}

${H}/grAedInt.h: grAedInt.h
	/bin/rm -f ${H}/grAedInt.h 
	cp grAedInt.h ${H}

${H}/grX10aInt.h: grX10aInt.h
	/bin/rm -f ${H}/grX10aInt.h 
	cp grX10aInt.h ${H}

${BIN}/${X10HELP_PROG}: ${X10HELP_PROG}
	/bin/rm -f ${BIN}/${X10HELP_PROG}
	cp ${X10HELP_PROG} ${BIN}/${X10HELP_PROG}

${BIN}/${X11HELP_PROG}: ${X11HELP_PROG}
	/bin/rm -f ${BIN}/${X11HELP_PROG}
	cp ${X11HELP_PROG} ${BIN}/${X11HELP_PROG}

${BIN}/${SUNHELP_PROG}: ${SUNHELP_PROG}
	/bin/rm -f ${BIN}/${SUNHELP_PROG}
	cp ${SUNHELP_PROG} ${BIN}/${SUNHELP_PROG}

${SYSLIB}/color.glyphs: color.glyphs
	/bin/rm -f ${SYSLIB}/color.glyphs
	cp color.glyphs ${SYSLIB}/color.glyphs

${SYSLIB}/bw.glyphs: bw.glyphs
	/bin/rm -f ${SYSLIB}/bw.glyphs
	cp bw.glyphs ${SYSLIB}/bw.glyphs

${SYSLIB}/mos.7bit.dstyle5: mos.7bit.dstyle
	/bin/rm -f ${SYSLIB}/mos.7bit.dstyle5
	cp mos.7bit.dstyle ${SYSLIB}/mos.7bit.dstyle5

${SYSLIB}/mos.6bit.dstyle5: mos.6bit.dstyle
	/bin/rm -f ${SYSLIB}/mos.6bit.dstyle5
	cp mos.6bit.dstyle ${SYSLIB}/mos.6bit.dstyle5

${SYSLIB}/mos.4bit.dstyle5: mos.4bit.dstyle
	/bin/rm -f ${SYSLIB}/mos.4bit.dstyle5
	cp mos.4bit.dstyle ${SYSLIB}/mos.4bit.dstyle5

${SYSLIB}/mos.bw.dstyle5: mos.bw.dstyle
	/bin/rm -f ${SYSLIB}/mos.bw.dstyle5
	cp mos.bw.dstyle ${SYSLIB}/mos.bw.dstyle5

${SYSLIB}/mos.7bit.std.cmap1: mos.7bit.std.cmap
	/bin/rm -f ${SYSLIB}/mos.7bit.std.cmap1
	cp mos.7bit.std.cmap ${SYSLIB}/mos.7bit.std.cmap1

${SYSLIB}/mos.6bit.std.cmap1: mos.6bit.std.cmap
	/bin/rm -f ${SYSLIB}/mos.6bit.std.cmap1
	cp mos.6bit.std.cmap ${SYSLIB}/mos.6bit.std.cmap1

${SYSLIB}/mos.4bit.std.cmap1: mos.4bit.std.cmap
	/bin/rm -f ${SYSLIB}/mos.4bit.std.cmap1
	cp mos.4bit.std.cmap ${SYSLIB}/mos.4bit.std.cmap1

${LIB}/${MODULE}.o: ${MODULE}.o
	/bin/rm -f ${LIB}/${MODULE}.o
	cp ${MODULE}.o ${LIB}
	../:instclean ${MODULE} ${ALLSRCS} 

${LIB}/lint/graphics.ln: ${SRCS}
	csh :makeforlint ${AEDSOURCES}
	../:lintlib ${MODULE} ${LINTLIBSOURCES}
 
 
# Do not put anything after this line!  The rest of this file was produced
# by the program ~cad/src/magic/:depend on Wed Aug 29 13:07:38 PDT 1990.
 
# include file dependencies for "X10helper.c"
X10helper.o:	X10helper.c
 
# include file dependencies for "X11Helper.c"
X11Helper.o:	X11Helper.c
 
# include file dependencies for "grAed1.c"
grAed1.o:	grAed1.c
grAed1.o:	${H}/magic.h
grAed1.o:	${H}/geometry.h
grAed1.o:	graphics.h
grAed1.o:	${H}/windows.h
grAed1.o:	graphicsInt.h
grAed1.o:	grAedInt.h
grAed1.o:	${H}/textio.h
grAed1.o:	${H}/txcommands.h
grAed1.o:	${H}/signals.h
 
# include file dependencies for "grAed2.c"
grAed2.o:	grAed2.c
grAed2.o:	${H}/magic.h
grAed2.o:	${H}/textio.h
grAed2.o:	${H}/geometry.h
grAed2.o:	graphics.h
grAed2.o:	${H}/windows.h
grAed2.o:	graphicsInt.h
grAed2.o:	grAedInt.h
 
# include file dependencies for "grAed3.c"
grAed3.o:	grAed3.c
grAed3.o:	${H}/magic.h
grAed3.o:	${H}/geometry.h
grAed3.o:	graphics.h
grAed3.o:	${H}/windows.h
grAed3.o:	graphicsInt.h
grAed3.o:	grAedInt.h
grAed3.o:	${H}/textio.h
grAed3.o:	${H}/signals.h
grAed3.o:	${H}/malloc.h
 
# include file dependencies for "grAed4.c"
grAed4.o:	grAed4.c
grAed4.o:	${H}/magic.h
grAed4.o:	${H}/textio.h
grAed4.o:	${H}/geometry.h
grAed4.o:	graphics.h
grAed4.o:	${H}/windows.h
grAed4.o:	graphicsInt.h
grAed4.o:	grAedInt.h
grAed4.o:	${H}/txcommands.h
grAed4.o:	${H}/utils.h
 
# include file dependencies for "grAed5.c"
grAed5.o:	grAed5.c
grAed5.o:	${H}/magic.h
grAed5.o:	${H}/textio.h
grAed5.o:	${H}/geometry.h
grAed5.o:	graphics.h
grAed5.o:	glyphs.h
grAed5.o:	${H}/windows.h
grAed5.o:	graphicsInt.h
grAed5.o:	grAedInt.h
 
# include file dependencies for "grCMap.c"
grCMap.o:	grCMap.c
grCMap.o:	${H}/magic.h
grCMap.o:	${H}/geometry.h
grCMap.o:	${H}/tile.h
grCMap.o:	${H}/hash.h
grCMap.o:	${H}/database.h
grCMap.o:	${H}/windows.h
grCMap.o:	graphics.h
grCMap.o:	graphicsInt.h
grCMap.o:	${H}/utils.h
grCMap.o:	${H}/textio.h
 
# include file dependencies for "grClip.c"
grClip.o:	grClip.c
grClip.o:	${H}/magic.h
grClip.o:	${H}/textio.h
grClip.o:	${H}/geometry.h
grClip.o:	graphics.h
grClip.o:	${H}/windows.h
grClip.o:	graphicsInt.h
grClip.o:	${H}/malloc.h
 
# include file dependencies for "grDStyle.c"
grDStyle.o:	grDStyle.c
grDStyle.o:	${H}/magic.h
grDStyle.o:	${H}/utils.h
grDStyle.o:	${H}/textio.h
grDStyle.o:	${H}/geometry.h
grDStyle.o:	graphics.h
grDStyle.o:	glyphs.h
grDStyle.o:	${H}/windows.h
grDStyle.o:	graphicsInt.h
 
# include file dependencies for "grGlyphs.c"
grGlyphs.o:	grGlyphs.c
grGlyphs.o:	${H}/magic.h
grGlyphs.o:	${H}/geometry.h
grGlyphs.o:	${H}/utils.h
grGlyphs.o:	graphics.h
grGlyphs.o:	${H}/windows.h
grGlyphs.o:	graphicsInt.h
grGlyphs.o:	glyphs.h
grGlyphs.o:	${H}/textio.h
grGlyphs.o:	${H}/malloc.h
 
# include file dependencies for "grLint1.c"
grLint1.o:	grLint1.c
grLint1.o:	${H}/magic.h
grLint1.o:	${H}/geometry.h
grLint1.o:	graphics.h
grLint1.o:	${H}/windows.h
grLint1.o:	graphicsInt.h
grLint1.o:	grAedInt.h
grLint1.o:	${H}/textio.h
grLint1.o:	${H}/txcommands.h
grLint1.o:	${H}/signals.h
 
# include file dependencies for "grLint2.c"
grLint2.o:	grLint2.c
grLint2.o:	${H}/magic.h
grLint2.o:	${H}/textio.h
grLint2.o:	${H}/geometry.h
grLint2.o:	graphics.h
grLint2.o:	${H}/windows.h
grLint2.o:	graphicsInt.h
grLint2.o:	grAedInt.h
 
# include file dependencies for "grLint3.c"
grLint3.o:	grLint3.c
grLint3.o:	${H}/magic.h
grLint3.o:	${H}/geometry.h
grLint3.o:	graphics.h
grLint3.o:	${H}/windows.h
grLint3.o:	graphicsInt.h
grLint3.o:	grAedInt.h
grLint3.o:	${H}/textio.h
grLint3.o:	${H}/signals.h
grLint3.o:	${H}/malloc.h
 
# include file dependencies for "grLint4.c"
grLint4.o:	grLint4.c
grLint4.o:	${H}/magic.h
grLint4.o:	${H}/textio.h
grLint4.o:	${H}/geometry.h
grLint4.o:	graphics.h
grLint4.o:	${H}/windows.h
grLint4.o:	graphicsInt.h
grLint4.o:	grAedInt.h
grLint4.o:	${H}/txcommands.h
grLint4.o:	${H}/utils.h
 
# include file dependencies for "grLint5.c"
grLint5.o:	grLint5.c
grLint5.o:	${H}/magic.h
grLint5.o:	${H}/textio.h
grLint5.o:	${H}/geometry.h
grLint5.o:	graphics.h
grLint5.o:	glyphs.h
grLint5.o:	${H}/windows.h
grLint5.o:	graphicsInt.h
grLint5.o:	grAedInt.h
 
# include file dependencies for "grLock.c"
grLock.o:	grLock.c
grLock.o:	${H}/magic.h
grLock.o:	${H}/geometry.h
grLock.o:	graphics.h
grLock.o:	${H}/windows.h
grLock.o:	${H}/textio.h
 
# include file dependencies for "grMain.c"
grMain.o:	grMain.c
grMain.o:	${H}/magic.h
grMain.o:	${H}/textio.h
grMain.o:	${H}/geometry.h
grMain.o:	graphics.h
grMain.o:	${H}/windows.h
grMain.o:	graphicsInt.h
 
# include file dependencies for "grNull.c"
grNull.o:	grNull.c
grNull.o:	${H}/magic.h
grNull.o:	${H}/geometry.h
grNull.o:	graphics.h
grNull.o:	${H}/windows.h
grNull.o:	graphicsInt.h
grNull.o:	${H}/textio.h
grNull.o:	${H}/txcommands.h
grNull.o:	${H}/signals.h
 
# include file dependencies for "grSun1.c"
grSun1.o:	grSun1.c
grSun1.o:	${H}/magic.h
grSun1.o:	${H}/geometry.h
grSun1.o:	graphics.h
grSun1.o:	${H}/windows.h
grSun1.o:	graphicsInt.h
grSun1.o:	grSunInt.h
grSun1.o:	${H}/txcommands.h
grSun1.o:	${H}/utils.h
grSun1.o:	${H}/textio.h
grSun1.o:	${H}/signals.h
grSun1.o:	${H}/paths.h
 
# include file dependencies for "grSun2.c"
grSun2.o:	grSun2.c
grSun2.o:	${H}/magic.h
grSun2.o:	${H}/textio.h
grSun2.o:	${H}/geometry.h
grSun2.o:	graphics.h
grSun2.o:	${H}/windows.h
grSun2.o:	graphicsInt.h
grSun2.o:	grSunInt.h
 
# include file dependencies for "grSun3.c"
grSun3.o:	grSun3.c
grSun3.o:	${H}/magic.h
grSun3.o:	${H}/geometry.h
grSun3.o:	graphics.h
grSun3.o:	${H}/windows.h
grSun3.o:	graphicsInt.h
grSun3.o:	${H}/textio.h
grSun3.o:	grSunInt.h
grSun3.o:	${H}/signals.h
 
# include file dependencies for "grSun4.c"
grSun4.o:	grSun4.c
grSun4.o:	${H}/magic.h
grSun4.o:	${H}/textio.h
grSun4.o:	${H}/geometry.h
grSun4.o:	${H}/windows.h
grSun4.o:	graphics.h
grSun4.o:	graphicsInt.h
grSun4.o:	grSunInt.h
grSun4.o:	${H}/txcommands.h
 
# include file dependencies for "grSun5.c"
grSun5.o:	grSun5.c
grSun5.o:	${H}/magic.h
grSun5.o:	${H}/textio.h
grSun5.o:	${H}/geometry.h
grSun5.o:	graphics.h
grSun5.o:	glyphs.h
grSun5.o:	${H}/windows.h
grSun5.o:	graphicsInt.h
grSun5.o:	grSunInt.h
 
# include file dependencies for "grSunProg.c"
grSunProg.o:	grSunProg.c
grSunProg.o:	${H}/magic.h
grSunProg.o:	${H}/geometry.h
grSunProg.o:	graphics.h
 
# include file dependencies for "grSunProg2.c"
grSunProg2.o:	grSunProg2.c
 
# include file dependencies for "grSunW1.c"
grSunW1.o:	grSunW1.c
grSunW1.o:	${H}/magic.h
grSunW1.o:	${H}/geometry.h
grSunW1.o:	graphics.h
grSunW1.o:	${H}/windows.h
grSunW1.o:	graphicsInt.h
grSunW1.o:	grSunWInt.h
grSunW1.o:	${H}/txcommands.h
grSunW1.o:	${H}/utils.h
grSunW1.o:	${H}/textio.h
grSunW1.o:	${H}/signals.h
grSunW1.o:	${H}/malloc.h
 
# include file dependencies for "grSunW2.c"
grSunW2.o:	grSunW2.c
grSunW2.o:	${H}/magic.h
grSunW2.o:	${H}/textio.h
grSunW2.o:	${H}/geometry.h
grSunW2.o:	graphics.h
grSunW2.o:	${H}/windows.h
grSunW2.o:	graphicsInt.h
grSunW2.o:	grSunWInt.h
 
# include file dependencies for "grSunW3.c"
grSunW3.o:	grSunW3.c
grSunW3.o:	${H}/magic.h
grSunW3.o:	${H}/geometry.h
grSunW3.o:	${H}/geofast.h
grSunW3.o:	graphics.h
grSunW3.o:	${H}/windows.h
grSunW3.o:	graphicsInt.h
grSunW3.o:	${H}/textio.h
grSunW3.o:	grSunWInt.h
grSunW3.o:	${H}/signals.h
 
# include file dependencies for "grSunW4.c"
grSunW4.o:	grSunW4.c
grSunW4.o:	${H}/magic.h
grSunW4.o:	${H}/textio.h
grSunW4.o:	${H}/geometry.h
grSunW4.o:	${H}/windows.h
grSunW4.o:	graphics.h
grSunW4.o:	graphicsInt.h
grSunW4.o:	grSunWInt.h
grSunW4.o:	${H}/txcommands.h
 
# include file dependencies for "grSunW5.c"
grSunW5.o:	grSunW5.c
grSunW5.o:	${H}/magic.h
grSunW5.o:	${H}/textio.h
grSunW5.o:	${H}/geometry.h
grSunW5.o:	graphics.h
grSunW5.o:	glyphs.h
grSunW5.o:	${H}/windows.h
grSunW5.o:	graphicsInt.h
grSunW5.o:	grSunWInt.h
 
# include file dependencies for "grText.c"
grText.o:	grText.c
grText.o:	${H}/magic.h
grText.o:	${H}/textio.h
grText.o:	${H}/geometry.h
grText.o:	graphics.h
grText.o:	${H}/windows.h
grText.o:	graphicsInt.h
grText.o:	${H}/utils.h
 
# include file dependencies for "grX10a1.c"
grX10a1.o:	grX10a1.c
grX10a1.o:	${H}/magic.h
grX10a1.o:	${H}/geometry.h
grX10a1.o:	graphics.h
grX10a1.o:	${H}/windows.h
grX10a1.o:	graphicsInt.h
grX10a1.o:	${H}/textio.h
grX10a1.o:	${H}/txcommands.h
grX10a1.o:	${H}/signals.h
grX10a1.o:	${H}/utils.h
grX10a1.o:	${H}/hash.h
grX10a1.o:	grX10aInt.h
grX10a1.o:	${H}/paths.h
 
# include file dependencies for "grX10a2.c"
grX10a2.o:	grX10a2.c
grX10a2.o:	${H}/magic.h
grX10a2.o:	${H}/textio.h
grX10a2.o:	${H}/geometry.h
grX10a2.o:	graphics.h
grX10a2.o:	${H}/windows.h
grX10a2.o:	graphicsInt.h
grX10a2.o:	grX10aInt.h
grX10a2.o:	${H}/hash.h
grX10a2.o:	${H}/tile.h
grX10a2.o:	${H}/database.h
grX10a2.o:	${H}/main.h
 
# include file dependencies for "grX10a3.c"
grX10a3.o:	grX10a3.c
grX10a3.o:	${H}/magic.h
grX10a3.o:	${H}/geometry.h
grX10a3.o:	graphics.h
grX10a3.o:	${H}/windows.h
grX10a3.o:	graphicsInt.h
grX10a3.o:	${H}/textio.h
grX10a3.o:	${H}/signals.h
grX10a3.o:	grX10aInt.h
 
# include file dependencies for "grX10a4.c"
grX10a4.o:	grX10a4.c
grX10a4.o:	${H}/magic.h
grX10a4.o:	${H}/textio.h
grX10a4.o:	${H}/geometry.h
grX10a4.o:	graphics.h
grX10a4.o:	${H}/windows.h	    
grX10a4.o:	graphicsInt.h
grX10a4.o:	${H}/txcommands.h
grX10a4.o:	grX10aInt.h
 
# include file dependencies for "grX10a5.c"
grX10a5.o:	grX10a5.c
grX10a5.o:	${H}/magic.h
grX10a5.o:	${H}/hash.h
grX10a5.o:	${H}/textio.h
grX10a5.o:	${H}/geometry.h
grX10a5.o:	graphics.h
grX10a5.o:	glyphs.h
grX10a5.o:	${H}/windows.h	    
grX10a5.o:	graphicsInt.h
grX10a5.o:	grX10aInt.h
 
# include file dependencies for "grX11su1.c"
grX11su1.o:	grX11su1.c
grX11su1.o:	${H}/magic.h
grX11su1.o:	${H}/geometry.h
grX11su1.o:	graphics.h
grX11su1.o:	${H}/windows.h
grX11su1.o:	graphicsInt.h
grX11su1.o:	${H}/textio.h
grX11su1.o:	${H}/txcommands.h
grX11su1.o:	${H}/signals.h
grX11su1.o:	${H}/utils.h
grX11su1.o:	${H}/hash.h
grX11su1.o:	grX11conf.h
grX11su1.o:	grX11Int.h
grX11su1.o:	${H}/paths.h
 
# include file dependencies for "grX11su2.c"
grX11su2.o:	grX11su2.c
grX11su2.o:	${H}/magic.h
grX11su2.o:	${H}/textio.h
grX11su2.o:	${H}/geometry.h
grX11su2.o:	graphics.h
grX11su2.o:	${H}/windows.h
grX11su2.o:	graphicsInt.h
grX11su2.o:	grX11conf.h
grX11su2.o:	grX11Int.h
 
# include file dependencies for "grX11su3.c"
grX11su3.o:	grX11su3.c
grX11su3.o:	${H}/magic.h
grX11su3.o:	${H}/geometry.h
grX11su3.o:	graphics.h
grX11su3.o:	${H}/windows.h
grX11su3.o:	graphicsInt.h
grX11su3.o:	${H}/textio.h
grX11su3.o:	${H}/signals.h
grX11su3.o:	${H}/utils.h
grX11su3.o:	${H}/hash.h
grX11su3.o:	grX11conf.h
grX11su3.o:	grX11Int.h
 
# include file dependencies for "grX11su4.c"
grX11su4.o:	grX11su4.c
grX11su4.o:	${H}/magic.h
grX11su4.o:	${H}/textio.h
grX11su4.o:	${H}/geometry.h
grX11su4.o:	graphics.h
grX11su4.o:	${H}/windows.h
grX11su4.o:	graphicsInt.h
grX11su4.o:	${H}/txcommands.h
grX11su4.o:	grX11conf.h
grX11su4.o:	grX11Int.h
 
# include file dependencies for "grX11su5.c"
grX11su5.o:	grX11su5.c
grX11su5.o:	${H}/magic.h
grX11su5.o:	${H}/hash.h
grX11su5.o:	${H}/textio.h
grX11su5.o:	${H}/geometry.h
grX11su5.o:	graphics.h
grX11su5.o:	glyphs.h
grX11su5.o:	${H}/windows.h
grX11su5.o:	graphicsInt.h
grX11su5.o:	grX11conf.h
grX11su5.o:	grX11Int.h
 
# include file dependencies for "prcolors.c"
prcolors.o:	prcolors.c
 
