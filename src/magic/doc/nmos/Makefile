#
# makefile for nMOS design rule description
#

PRINTER=verlag
TROFF=ditroff -P${PRINTER}
TTROFF=ditroff -Ppsc -t
PSDIT=psdit
GRN=grn -P${PRINTER}
TBL=tbl

nmos:
	${GRN} text | ${TBL} | ${TROFF} -ms

postscript:
	${GRN} text | ${TBL} | ${TTROFF} -ms | ${PSDIT} > nmos.ps

