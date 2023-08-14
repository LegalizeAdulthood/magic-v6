.\" ------ IN level section string pagenum
.\"     A table of contents (INdex) macro, where level is 0 for chapters,
.\"     1 for subsections.
.\"
.\"     Note:  This macro resets tab stops and 'fill' mode.
.de IN
.nf
.if !\\$1 \{\
.ta \n(.lu-5mR
\fBCHAPTER \\$2\ \ \ \\$3\fR\ \
\}
.if \\$1 \{\
.ta \n(.iu+3m \n(.iu+6m \n(.lu-5mR
	\\$2    \\$3\ \
	\}          
..        
.de AN
.nf
.if !\\$1 \{\
.ta \n(.lu-5mR
\fBAPPENDIX \\$2\ \ \ \\$3\fR\ \
\}
.if \\$1 \{\
.ta \n(.iu+3m \n(.iu+6m \n(.lu-5mR
	\\$2    \\$3\ \ \
	\}          
..        
.nr PS 14
.nr VS 17
.LP
.DS C
.sp 0.5i
.LG
.LG
\fBTable of Contents\fR
.NL
.DE
.sp 0.4i
.LP
.SM
.IN 0 "1" "Overview" 1   
.IN 0 "2" "Manual Pages - Section 1 (Programs)" 1   
.IN 0 "3" "Manual Pages - Section 3 (Libraries)" 1   
.IN 0 "4" "Manual Pages - Section 5 (File Formats)" 1   
.IN 0 "5" "Manual Pages - Section 8 (System Maintenance)" 1   
.IN 0 "6" "Tutorials" 1
.IN 1 "6.1" "Magic Tutorial #1:  Getting Started" 1
.IN 1 "6.2" "Magic Tutorial #2:  Basic Painting and Selection" 1
.IN 1 "6.3" "Magic Tutorial #3:  Advanced Painting (Wiring and Plowing)" 1
.IN 1 "6.4" "Magic Tutorial #4:  Cell Heirarchies" 1
.IN 1 "6.5" "Magic Tutorial #5:  Multiple Windows" 1
.IN 1 "6.6" "Magic Tutorial #6:  Design-Rule Checking" 1
.IN 1 "6.7" "Magic Tutorial #7:  Netlists and Routing" 1
.IN 1 "6.8" "Magic Tutorial #8:  Circuit Extraction" 1
.IN 1 "6.9" "Magic Tutorial #9:  Format Conversion for CIF and Calma" 1
.IN 1 "6.10" "Magic Tutorial #10:  The Interactive Router" 1
.IN 1 "6.11" "Magic Tutorial #11:  Using RSIM with Magic" 1
.IN 0 "7" "Maintainer's Manuals" 1
.IN 1 "7.1" "Magic Maintainer's Manual #1:  Hints for System Maintainers" 1
.IN 1 "7.1" "Magic Maintainer's Manual #2:  The Technology File" 1
.IN 1 "7.1" "Magic Maintainer's Manual #3:  The Display Style and Glyph Files" 1
.IN 1 "7.1" "Magic Maintainer's Manual #4:  Using Magic under X Windows" 1
.IN 0 "8" "Technology Manuals" 1
.IN 1 "8.1" "Magic Technology Manual #1:  NMOS" 1
.IN 1 "8.1" "Magic Technology Manual #2:  SCMOS" 1
.AN 0 "A" "Other Reports In This Series" 1
