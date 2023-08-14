#
# Makefile for all of magic
#
#	MODULES are pieces of the Magic system.  The makefiles for
#		each of these will 'install' in the subdirectories
#		'lib' and 'installed'.
#	PROGRAMS are individual programs that get installed.
#		The makefiles for each of these will 'install' in
#		~cad/bin.
#	FILES are miscellaneous files 
#	DIRS are whole directories that get copied (also see disttape).
#
# Special handling: the subdirectory cadlib gets installed remotely
# as ~cad/lib/magic.  It is a symbolic link.  The module 'graphics' appears 
# in PROGRAMS because that is where X11Helper, grSunProg, and grSunProg2 reside.
#
# rcsid $Header: Makefile,v 1.16 90/09/13 10:29:32 mayo Exp $
#

SHELL=/bin/sh

#
# NOTE: tiles and tech must be before database in this list
#

BASEMODS	= tiles tech \
		  calma cif cmwind commands database dbwind debug drc \
		  extflat extract garouter gcr graphics grouter irouter \
		  macros main misc mzrouter netlist netmenu \
		  parser plot plow resis router select signals sim \
		  textio undo utils windows wiring
BASEMODS.WRL	= tiles tech \
		  calma cif.wrl cmwind commands.wrl database.wrl \
		  dbwind debug drc.wrl \
		  extflat.wrl extract.wrl garouter gcr \
		  graphics grouter irouter \
		  macros main misc mzrouter netlist netmenu \
		  parser plot plow resis.wrl router select signals sim \
		  textio undo utils windows wiring
MODULES		= ${BASEMODS} 
MODULES.WRL	= ${BASEMODS.WRL} 

BASEMAINS	= ext2dlys ext2sim ext2spice fsleeper magic magicusage \
		  net2ir extcheck
BASEMAINS.WRL	= ext2dlys ext2sim ext2spice fsleeper magic.wrl magicusage \
		  net2ir extcheck
PROGRAMS	= ${BASEMAINS} graphics
PROGRAMS.WRL	= ${BASEMAINS.WRL} graphics
FILES		= :cadtomag :countlines :makeall :makeforce :makelib \
		  :status Makefile READ_ME
DIRS		= include cadlib
CMD		= echo Need to define your command via 'make cmd CMD=command'
CADDIR		= `:findcad`
CADDIR_SUB	= `../:findcad`

everything: ${MODULES}
	/bin/csh :makeall installhdrs ${MODULES}
	/bin/csh :makeall install ${MODULES}
	/bin/csh :makemains ${PROGRAMS}

everything.wrl: ${MODULES.WRL}
	/bin/csh :makeall installhdrs ${MODULES.WRL}
	/bin/csh :makeall install ${MODULES.WRL}
	/bin/csh :makemains ${PROGRAMS}

install: 
	cd magic; make install CADDIR=${CADDIR_SUB}
	cd graphics; make installcad CADDIR=${CADDIR_SUB}
	cd windows; make installcad CADDIR=${CADDIR_SUB}
	cd tech; make installcad CADDIR=${CADDIR_SUB}
	cd doc; make install CADDIR=${CADDIR_SUB}
	cd ext2dlys; make install CADDIR=${CADDIR_SUB}
	cd ext2sim; make install CADDIR=${CADDIR_SUB}
	cd ext2spice; make install CADDIR=${CADDIR_SUB}
	cd fsleeper; make install CADDIR=${CADDIR_SUB}
	cd magicusage; make install CADDIR=${CADDIR_SUB}
	cd net2ir; make install CADDIR=${CADDIR_SUB}
	cd extcheck; make install CADDIR=${CADDIR_SUB}

# Remakes the binary in 'lib', if needed.  Used for recompiling on 
# a remote machine.
onlylib: ${MODULES}
	/bin/csh :makelib ${MODULES}

modules:
	mkdir ${MODULES}

installhdrs: ${MODULES}
	/bin/csh :makeall installhdrs ${MODULES}

cmd:	forceit
	/bin/csh :execmod "${CMD}" ${MODULES} ${BASEMAINS}

allobj:	${MODULES}
	/bin/csh :makeall  "" ${MODULES}

force:	forceit
	rm -rf include/*
	rm -rf lib/*
	cd graphics; make clean
	/bin/csh :makeall installhdrs ${MODULES}
	/bin/csh :makeforce install ${MODULES}
	/bin/csh :makemains ${PROGRAMS}

force.wrl:	forceit
	rm -rf include/*
	rm -rf lib/*
	cd graphics; make clean
	/bin/csh :makeall installhdrs ${MODULES.WRL}
	/bin/csh :makeforce install ${MODULES.WRL}
	/bin/csh :makemains ${PROGRAMS}

clean:	forceit
	/bin/csh :execmod "make clean" ${MODULES} ${BASEMAINS}

lint:	forceit
	/bin/csh :execmod "make lint" ${MODULES} ${BASEMAINS}

status:	${MODULES}
	/bin/csh :status ${MODULES}

config:	forceit
	/bin/csh :config

magic:	everything
	cd magic; make magic

tags:
	rm -f magic/tags
	cp /dev/null magic/tags
	-for i in ${MODULES}; do \
		(cd magic ; ctags -a -t ../$$i/*.[ch] ; cd ..); done
	sort magic/tags -o magic/tags

TAGS:
	rm -f magic/TAGS.build
	-for i in ${MODULES}; do \
		(cd magic ; etags ../$$i/*.[ch] ; cat TAGS >> TAGS.build ;\
		 cd ..); done
	mv magic/TAGS.build magic/TAGS

tagdirs:
	-for i in ${MODULES}; do \
		(cd $$i ; rm -f tags ; ln -s ../magic/tags tags ; cd ..); done

allp:	${MODULES}
	/bin/csh :makeall profile ${MODULES}

forcep:	${MODULES}
	/bin/csh :makeforce profile ${MODULES}

magicp: allp
	cd magic; make magicp

wc:	${MODULES}
	:countlines ${MODULES}

moduleNames:
	echo ${MODULES}

forceit:
	
