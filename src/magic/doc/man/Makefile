#
# makefile for Magic documentation
#

CADDIR = /nc/cadhome
DOCDIR=${CADDIR}/doc/magic
MANDIR=${CADDIR}/man
MANMACS= tmac.anc

PRINTER=verlag
TROFF=ditroff -h -P${PRINTER}
TTROFF=ditroff -Ppsc -t
PSDIT=psdit
GRN=grn -Ppsc
TBL=tbl -Ppsc
EQN=eqn -Ppsc

mans:	force
	:printmans "${TROFF} ${MANMACS} -" *.1 *.3 *.5 *.8

install:  directories \
	${MANDIR}/man1/ext2dlys.1 ${MANDIR}/man1/ext2spice.1 \
	${MANDIR}/man1/extcheck.1 \
	${MANDIR}/man1/magic.1 ${MANDIR}/man1/magicusage.1 \
	${MANDIR}/man1/grsunprog.1 ${MANDIR}/man1/ext2sim.1 \
	${MANDIR}/man1/net2ir.1 ${MANDIR}/man1/rsleeper.1 \
	${MANDIR}/man1/sim2spice.1 ${MANDIR}/man1/sleeper.1 \
	${MANDIR}/man1/fsleeper.1 ${MANDIR}/man3/mpack.3 \
	${MANDIR}/man3/dqueue.3 ${MANDIR}/man3/extflat.3 \
	${MANDIR}/man3/geometry.3 ${MANDIR}/man3/hash.3 \
	${MANDIR}/man3/heap.3 ${MANDIR}/man3/libmalloc.3 \
	${MANDIR}/man3/list.3 ${MANDIR}/man3/magicutils.3 \
	${MANDIR}/man3/malloc.3 ${MANDIR}/man3/path.3 \
	${MANDIR}/man3/runstats.3 ${MANDIR}/man3/set.3 \
	${MANDIR}/man3/show.3 ${MANDIR}/man3/stack.3 \
	${MANDIR}/man3/string.3 \
	${MANDIR}/man5/cmap.5 ${MANDIR}/man5/displays.5 \
	${MANDIR}/man5/dlys.5 ${MANDIR}/man5/sim.5 \
	${MANDIR}/man5/dstyle.5 ${MANDIR}/man5/ext.5 \
	${MANDIR}/man5/glyphs.5 ${MANDIR}/man5/magic.5 \
	${MANDIR}/man5/net.5 ${MANDIR}/man5/sim.5 \
	${MANDIR}/man8/prleak.8

directories:
	-mkdir ${MANDIR}
	-mkdir ${MANDIR}/man1 ${MANDIR}/cat1 ${MANDIR}/man3 ${MANDIR}/cat3
	-mkdir ${MANDIR}/man5 ${MANDIR}/cat5 ${MANDIR}/man8 ${MANDIR}/cat8


${MANDIR}/man1/ext2dlys.1: ext2dlys.1
	rm -f ${MANDIR}/man1/ext2dlys.1
	cp ext2dlys.1 ${MANDIR}/man1

${MANDIR}/man1/ext2spice.1: ext2spice.1
	rm -f ${MANDIR}/man1/ext2spice.1
	cp ext2spice.1 ${MANDIR}/man1

${MANDIR}/man1/extcheck.1: extcheck.1
	rm -f ${MANDIR}/man1/extcheck.1
	cp extcheck.1 ${MANDIR}/man1

${MANDIR}/man1/magic.1: magic.1
	rm -f ${MANDIR}/man1/magic.1
	cp magic.1 ${MANDIR}/man1

${MANDIR}/man1/magicusage.1: magicusage.1
	rm -f ${MANDIR}/man1/magicusage.1
	cp magicusage.1 ${MANDIR}/man1

${MANDIR}/man1/grsunprog.1: grsunprog.1
	rm -f ${MANDIR}/man1/grsunprog.1
	cp grsunprog.1 ${MANDIR}/man1

${MANDIR}/man1/net2ir.1: net2ir.1
	rm -f ${MANDIR}/man1/net2ir.1
	cp net2ir.1 ${MANDIR}/man1

${MANDIR}/man1/rsleeper.1: rsleeper.1
	rm -f ${MANDIR}/man1/rsleeper.1
	cp rsleeper.1 ${MANDIR}/man1

${MANDIR}/man1/sim2spice.1: sim2spice.1
	rm -f ${MANDIR}/man1/sim2spice.1
	cp sim2spice.1 ${MANDIR}/man1

${MANDIR}/man1/sleeper.1: sleeper.1
	rm -f ${MANDIR}/man1/sleeper.1
	cp sleeper.1 ${MANDIR}/man1

${MANDIR}/man1/ext2sim.1: ext2sim.1
	rm -f ${MANDIR}/man1/ext2sim.1
	cp ext2sim.1 ${MANDIR}/man1

${MANDIR}/man1/fsleeper.1: fsleeper.1
	rm -f ${MANDIR}/man1/fsleeper.1
	cp fsleeper.1 ${MANDIR}/man1

${MANDIR}/man3/mpack.3: mpack.3
	rm -f ${MANDIR}/man3/mpack.3
	cp mpack.3 ${MANDIR}/man3

${MANDIR}/man3/dqueue.3: dqueue.3
	rm -f ${MANDIR}/man3/dqueue.3
	cp dqueue.3 ${MANDIR}/man3

${MANDIR}/man3/extflat.3: extflat.3
	rm -f ${MANDIR}/man3/extflat.3
	cp extflat.3 ${MANDIR}/man3

${MANDIR}/man3/geometry.3: geometry.3
	rm -f ${MANDIR}/man3/geometry.3
	cp geometry.3 ${MANDIR}/man3

${MANDIR}/man3/hash.3: hash.3
	rm -f ${MANDIR}/man3/hash.3
	cp hash.3 ${MANDIR}/man3

${MANDIR}/man3/heap.3: heap.3
	rm -f ${MANDIR}/man3/heap.3
	cp heap.3 ${MANDIR}/man3

${MANDIR}/man3/libmalloc.3: libmalloc.3
	rm -f ${MANDIR}/man3/libmalloc.3
	cp libmalloc.3 ${MANDIR}/man3

${MANDIR}/man3/list.3: list.3
	rm -f ${MANDIR}/man3/list.3
	cp list.3 ${MANDIR}/man3

${MANDIR}/man3/magicutils.3: magicutils.3
	rm -f ${MANDIR}/man3/magicutils.3
	cp magicutils.3 ${MANDIR}/man3

${MANDIR}/man3/malloc.3: malloc.3
	rm -f ${MANDIR}/man3/malloc.3
	cp malloc.3 ${MANDIR}/man3

${MANDIR}/man3/path.3: path.3
	rm -f ${MANDIR}/man3/path.3
	cp path.3 ${MANDIR}/man3

${MANDIR}/man3/runstats.3: runstats.3
	rm -f ${MANDIR}/man3/runstats.3
	cp runstats.3 ${MANDIR}/man3

${MANDIR}/man3/set.3: set.3
	rm -f ${MANDIR}/man3/set.3
	cp set.3 ${MANDIR}/man3

${MANDIR}/man3/show.3: show.3
	rm -f ${MANDIR}/man3/show.3
	cp show.3 ${MANDIR}/man3

${MANDIR}/man3/stack.3: stack.3
	rm -f ${MANDIR}/man3/stack.3
	cp stack.3 ${MANDIR}/man3

${MANDIR}/man3/string.3: string.3
	rm -f ${MANDIR}/man3/string.3
	cp string.3 ${MANDIR}/man3

${MANDIR}/man5/cmap.5: cmap.5
	rm -f ${MANDIR}/man5/cmap.5
	cp cmap.5 ${MANDIR}/man5

${MANDIR}/man5/displays.5: displays.5
	rm -f ${MANDIR}/man5/displays.5
	cp displays.5 ${MANDIR}/man5

${MANDIR}/man5/dlys.5: dlys.5
	rm -f ${MANDIR}/man5/dlys.5
	cp dlys.5 ${MANDIR}/man5

${MANDIR}/man5/dstyle.5: dstyle.5
	rm -f ${MANDIR}/man5/dstyle.5
	cp dstyle.5 ${MANDIR}/man5

${MANDIR}/man5/ext.5: ext.5
	rm -f ${MANDIR}/man5/ext.5
	cp ext.5 ${MANDIR}/man5

${MANDIR}/man5/glyphs.5: glyphs.5
	rm -f ${MANDIR}/man5/glyphs.5
	cp glyphs.5 ${MANDIR}/man5

${MANDIR}/man5/magic.5: magic.5
	rm -f ${MANDIR}/man5/magic.5
	cp magic.5 ${MANDIR}/man5

${MANDIR}/man5/net.5: net.5
	rm -f ${MANDIR}/man5/net.5
	cp net.5 ${MANDIR}/man5

${MANDIR}/man5/sim.5: sim.5
	rm -f ${MANDIR}/man5/sim.5
	cp sim.5 ${MANDIR}/man5

${MANDIR}/man8/prleak.8: prleak.8
	rm -f ${MANDIR}/man8/prleak.8
	cp prleak.8 ${MANDIR}/man8

force:
	
