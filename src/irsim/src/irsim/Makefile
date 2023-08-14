
# for no analyzer (systems without X) define X = NO and XLIB = <empty>
#X	= NO
#XLIB	= 
X	= 11
XLIB	= -lX11

# The following is set from ../../make_irsim (CFLAGS)
SYSF	= 

CC	= cc
MAKEDEP	= ../utils/makedep
EXPORT	= ../utils/export

SRCS =	access.c binsim.c cad_dir.c config.c conn_list.c eval.c fio.c \
	hist.c intr.c incsim.c mem.c prints.c netchange.c network.c \
	newrstep.c nsubrs.c parallel.c rsim.c sched.c sim.c sstep.c \
	stack.c tpos.c usage.c version.c

OBJS =	access.o binsim.o cad_dir.o config.o conn_list.o eval.o fio.o \
	hist.o intr.o incsim.o mem.o prints.o netchange.o network.o \
	newrstep.o nsubrs.o parallel.o rsim.o sched.o sim.o sstep.o \
	stack.o tpos.o usage.o version.o

HDRS =	../include/net.h

CDIRS =	-I../include

OPTIM = -O
DBX = -g

CSW = $(OPTIM)

ANA =	../ana$$(X)/ana.o

EXPORTS = globals.h

irsim : MAKEFILE $(EXPORTS) $(SRCS) ANALYZER
	make -f MAKEFILE CSW="$(CSW)" X="$(X)" XLIB="$(XLIB)" SYSF="$(SYSF)" $@

ANALYZER :
	( cd ../ana$(X) ; make SYSF="$(SYSF)" CSW="$(CSW)" ana.o )

$(OBJS) gentbl : MAKEFILE $(EXPORTS)
	make -f MAKEFILE CSW="$(CSW)" SYSF="$(SYSF)" $@

$(EXPORTS) : $(SRCS)
	$(EXPORT) $(SRCS) globals.h

$(SRCS) :
	co $@

MAKEFILE : Makefile
	@echo "Making MAKEFILE in irsim"
	@echo SRCS = $(SRCS) > MAKEFILE
	@echo OBJS = $(OBJS) >> MAKEFILE
	@echo HDSR = $(HDRS) >> MAKEFILE
	@echo LIBS = '$$(XLIB) -lm' >> MAKEFILE
	@echo SYSF = >> MAKEFILE
	@echo CSW = $(CSW) >> MAKEFILE
	@echo X = $(X) >> MAKEFILE
	@echo CDIRS = $(CDIRS) >> MAKEFILE
	@echo CFLAGS = '$$(CDIRS) $$(CSW) $$(SYSF)' >> MAKEFILE
	@echo 'irsim : spiketbl.c stables.c $$(OBJS) $(ANA)' >> MAKEFILE
	@echo '	$(CC) $$(CFLAGS) -o irsim $$(OBJS) $(ANA) $$(LIBS)' >> MAKEFILE
	@echo '$$(SRCS) gentbl.c genspktbl.c : ' >> MAKEFILE
	@echo '	co $$@' >> MAKEFILE
	@echo 'stables.c : gentbl'  >> MAKEFILE
	@echo '	gentbl' >> MAKEFILE
	@echo 'gentbl : gentbl.c' >> MAKEFILE
	@echo '	$(CC) $$(CFLAGS) -o gentbl gentbl.c' >> MAKEFILE
	@echo 'spiketbl.c : genspktbl' >> MAKEFILE
	@echo '	genspktbl spiketbl.c' >> MAKEFILE
	@echo 'genspktbl : genspktbl.c' >> MAKEFILE
	@echo '	$(CC) -o genspktbl genspktbl.c -lm' >> MAKEFILE
	@/bin/csh ../utils/CO $(SRCS)
	@$(MAKEDEP) -I ../include -N -o - $(SRCS) >> MAKEFILE

lint:
	lint -abhx $(CDIRS) $(SRCS) > irsim.lin
