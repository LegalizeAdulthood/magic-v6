#
# makefile for CMOS design rule description
#

PRINTER=verlag
TROFF=ditroff -P${PRINTER}
TTROFF=ditroff -Ppsc -t
PSDIT=psdit
GRN=grn -P${PRINTER}
TBL=tbl

scmos:
	${GRN} text | ${TBL} | ${TROFF} -ms

postscript:
	${GRN} text | ${TTROFF} -ms | ${PSDIT} > scmos.ps
