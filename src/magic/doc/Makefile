#
# makefile for Magic documentation
#

CADDIR = /udir/mayo/cadhome
DOCDIR=${CADDIR}/doc/magic

PRINTER=verlag
TROFF=ditroff -P${PRINTER}
TTROFF=ditroff -Ppsc -t
# PSDIT=psdit -F/jove_u2/mayo/transcript
PSDIT=psdit
GRN=grn -Ppsc
TBL=tbl -Ppsc
EQN=eqn -Ppsc


print_only:  force
	lpr cover.ps
	lpr -h copyright.ps
	lpr -h tofc.ps
	lpr -h introduction.ps
	lpr -h tut1.ps
	lpr -h tut2.ps
	lpr -h tut3.ps
	lpr -h tut4.ps
	lpr -h tut5.ps
	lpr -h tut6.ps
	lpr -h tut7.ps
	lpr -h tut8.ps
	lpr -h tut9.ps
	lpr -h tut10.ps
	lpr -h tut11.ps
	lpr -h maint1.ps
	lpr -h maint2.ps
	lpr -h maint3.ps
	lpr -h maint4.ps
	lpr -h otherreports.ps

print:  postscript print_only

postscript: copyright.ps introduction.ps tofc.ps tut1.ps tut2.ps tut3.ps \
	tut4.ps tut5.ps tut6.ps tut7.ps tut8.ps tut9.ps tut10.ps tut11.ps \
	maint1.ps maint2.ps maint3.ps maint4.ps
	cd nmos; make postscript PRINTER=psc "PSDIT=${PSDIT}"
	cd scmos; make postscript PRINTER=psc "PSDIT=${PSDIT}"

copyright.ps: copyright.ms
	${TTROFF} -ms copyright.ms | ${PSDIT} > copyright.ps

introduction.ps: introduction.ms
	${TTROFF} -ms introduction.ms | ${PSDIT} > introduction.ps

tofc.ps: tofc.ms
	${TTROFF} -ms tofc.ms | ${PSDIT} > tofc.ps

tut1.ps: tut1
	${GRN} tut1 | ${TBL} | ${TTROFF} -ms | ${PSDIT} > tut1.ps

tut2.ps: tut2
	${GRN} tut2 | ${TBL} | ${TTROFF} -ms | ${PSDIT} > tut2.ps

tut3.ps: tut3
	${TTROFF} -ms tut3 | ${PSDIT} > tut3.ps

tut4.ps: tut4
	${TTROFF} -ms tut4 | ${PSDIT} > tut4.ps

tut5.ps: tut5
	${GRN} tut5 | ${TTROFF} -ms | ${PSDIT} > tut5.ps

tut6.ps: tut6
	${TTROFF} -ms tut6 | ${PSDIT} > tut6.ps

tut7.ps: tut7
	${GRN} tut7 | ${TBL} | ${TTROFF} -ms | ${PSDIT} > tut7.ps

tut8.ps: tut8
	${GRN} tut8 | ${EQN} | ${TBL} | ${TTROFF} -ms | ${PSDIT} > tut8.ps

tut9.ps: tut9
	${GRN} tut9 | ${TTROFF} -ms | ${PSDIT} > tut9.ps

tut10.ps: tut10
	${GRN} tut10 | ${TTROFF} -ms | ${PSDIT} > tut10.ps

tut11.ps: tut11
	${TTROFF} -ms tut11 | ${PSDIT} > tut11.ps

maint1.ps: maint1
	${TTROFF} -ms maint1 | ${PSDIT} > maint1.ps

maint2.ps: maint2
	${TBL} maint2 | ${GRN} | ${TTROFF} -ms | ${PSDIT} > maint2.ps

maint3.ps: maint3
	${TTROFF} -ms maint3 | ${PSDIT} > maint3.ps

maint4.ps: maint4
	${TTROFF} -ms maint4 | ${PSDIT} > maint4.ps

install:  force
	cd man; make install CADDIR=${CADDIR}
	cd tutcells; make install CADDIR=${CADDIR}

force:
	
