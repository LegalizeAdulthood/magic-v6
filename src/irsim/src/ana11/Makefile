SYSF	= 
#SYSF	= -DSYS_V

CC	= cc
MAKEDEP	= ../utils/makedep


SRCS =	analyzer.c base.c convert.c coords.c deltaT.c event.c defaults.c \
	graphics.c icon.c menu.c movetot.c movetrace.c namelen.c \
	postscript.c setsteps.c scrollbar.c textwind.c update.c window.c \
	wm.c zoom.c

OBJS =	analyzer.o base.o convert.o coords.o deltaT.o event.o defaults.o \
	graphics.o icon.o menu.o movetrace.o movetot.o namelen.o \
	postscript.o setsteps.o scrollbar.o textwind.o update.o window.o \
	wm.o zoom.o

HDRS =	../include/net.h

LIBS =

CDIRS =	-I../include

CSW =	-O

EXPORTS = ana_glob.h

ana.o : MAKEFILE $(EXPORTS) $(SRCS) anXhelper.c
	make -f MAKEFILE CSW="$(CSW)" SYSF="$(SYSF)" $@
	make -f MAKEFILE CSW="$(CSW)" SYSF="$(SYSF)" anXhelper

$(OBJS) : MAKEFILE $(EXPORTS)
	make -f MAKEFILE CSW="$(CSW)" SYSF="$(SYSF)" $@

$(EXPORTS): $(SRCS)
	../utils/export $(SRCS) $(EXPORTS)

$(SRCS) :
	co $@

MAKEFILE : Makefile
	@echo "Makig MAKEFILE in ana11"
	@echo SRCS = $(SRCS) > MAKEFILE
	@echo OBJS = $(OBJS) >> MAKEFILE
	@echo HDSR = $(HDRS) >> MAKEFILE
	@echo SYSF = >> MAKEFILE
	@echo CSW = $(CSW) >> MAKEFILE
	@echo CDIRS = $(CDIRS) >> MAKEFILE
	@echo CFLAGS = '$$(CDIRS) $$(CSW) $$(SYSF)' >> MAKEFILE
	@echo 'ana.o : $$(OBJS)' >> MAKEFILE
	@echo '	ld -r -o ana.o $$(OBJS) $(LIBS)' >> MAKEFILE
	@echo 'anXhelper : anXhelper.c' >> MAKEFILE
	@echo '	$(CC) $$(CSW) $$(SYSF) -o anXhelper anXhelper.c' >> MAKEFILE
	@echo '$$(SRCS) : ' >> MAKEFILE
	@echo '	co $$@' >> MAKEFILE
	@/bin/csh ../utils/CO $(SRCS)
	@$(MAKEDEP) -I ../include -N -o - $(SRCS) >> MAKEFILE

lint:
	lint -abhx $(CDIRS) $(SRCS) > irsim.lint
