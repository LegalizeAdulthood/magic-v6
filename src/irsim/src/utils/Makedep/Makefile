# WARNING: DO NOT EDIT.
# THIS FILE WAS AUTOMATICALLY GENERATED USING THE
# BUILDMAKE PROGRAM.

# Buildfile for makedep

OBJS = makedep.o gen.o getincl.o print.o list.o

CFLAGS = -O

#	insert for system V
#LIBS = -lBSD

.c.o:
	cc $(CFLAGS) -c $<

makedep: $(OBJS)
	cc -o makedep $(OBJS)

print.o: \
	makedep.h \
	/usr/include/stdio.h \
	/usr/include/errno.h
makedep.o: \
	makedep.h \
	/usr/include/stdio.h \
	/usr/include/errno.h
list.o: \
	makedep.h \
	/usr/include/stdio.h \
	/usr/include/errno.h
getincl.o: \
	makedep.h \
	/usr/include/stdio.h \
	/usr/include/errno.h
gen.o: \
	makedep.h \
	/usr/include/stdio.h \
	/usr/include/errno.h

install:
	install -s makedep ${DESTDIR}/usr/stanford/bin/makedep

install-man:
	install -c -m 444 makedep.1 ${DESTDIR}/usr/man/man1/makedep.1

clean:
	rm -f *BAK *CKP *.o makedep
cleanbak:
	rm -f *BAK *CKP

build:
	makedep -U *.c
	buildmake

xbuild:
	makedep -U *.c
	buildmake -DX=1
