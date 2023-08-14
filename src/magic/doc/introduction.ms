.if t .tr ~\(ap
.rm CM
.nr PS 12
.ps 12
.nr VS 14
.vs 14
.\" rcsid $Header: introduction.ms,v 1.16 90/09/12 16:32:36 mayo Exp $
.DS C
.LG
.LG
.sp |2c
\fBOverview of the DECWRL/Livermore Magic Release\fR
.rm CH
.ds LH Overview of DECWRL/Livermore Magic
.ds RH \*(DY
.ds CF - % -
.sp 1c
.NL
.sp 1c
.SM
This document corresponds to Magic version 6.
.NL
.DE
.sp .5c
.NH 1
Introduction
.PP
This version of Magic, version 6, 
gathers together work done by numerous people at
several institutions since Magic version 4 was released from Berkeley
on the 1986 VLSI tools tape.  
This is a release of Magic and IRSIM only.  You'll probably want to obtain 
other tools by ordering the 1986 VLSI Tools Tape from Berkeley.
.PP
This release has been prepared with the assistance of several groups.
Much of the new software came from Walter Scott's group at
the Lawrence Livermore National Labs (LLNL).
LLNL also provided partial funding to help prepare the release.  
Digital Equipment Corporation's Western Research Lab (DECWRL) 
helped out by providing computer equipment, a place to work, and
the services of one of us (Robert Mayo).  Don Stark, Michael Arnold, and
Gordon Hamachi also worked on the release at DECWRL.
Stanford donated significant pieces of new code, including a simulation
system called IRSIM.
Other individuals and institutions have also contributed code and assistance
in ways too numerous to detail here.
.LP
New features in Magic Version 6 include:
.RS
.IP "New and Improved Routing - \fIMichael Arnold and Walter Scott of LLNL"
.RS
.PP
Three major routing improvements have been made in this version of Magic.
There is a new, improved, global router courtesy of Walter Scott (of LLNL).
Walter Scott has also added a gate array router.  See the "garoute" command
in the manual page for details.
Michael Arnold (of LLNL) has written an interactive maze router that allows
the user to specify hints to control the routing.  See the documentation
for the "iroute" command.
.RE
.IP "Extractor Enhancements - \fIDon Stark of Stanford and Walter Scott of LLNL"
.RS
.PP
The new "extresis" command, developed by Don Stark, provides
substantially better resistance extraction.  Magic's
normal extraction ("extract") lumps resistances on a 
node into a single value.  
In branching networks, this approximation is often not acceptable.
Resis was written to solve this problem.  Walter Scott added accurate path
length extraction, an important feature when dealing with high speed
circuits, such as ECL.
.RE
.IP "New contact structure - \fI Walter Scott and Michael Arnold of LLNL and Don Stark of Stanford"
.RS
.PP
Multilayer contacts are handled better.
In the previous version of Magic, there needed to be a separate contact type
for each possible combination of contact layers over a given point.  This
caused a combinatorial explosion of tile types for multi-layer technologies
with stacked contacts.  Under the new scheme, there are only a couple of
tile types for each layer:  one that connects up, one that connects down,
and one that connects in both directions.
.RE
.IP "Simulator Interface to IRSIM - \fIStanford\fR"
.RS
.PP
A simulator interface is provided courtesy of Stanford.  
See the commands "startrsim", "simcmd", and "rsim".
The irsim simulator, Stanford's much improved rewrite of esim, is included 
in this distribution.
Credit goes to Mike Chow, Arturo Salz, and Mark Horowitz.
.RE
.IP "New device/machine Support - \fIVarious\fR"
.RS
.PP
X11 is fully supported in this release, and is the preferred interface.  Older
drivers for graphics terminals and X10 are also included, but X11 is the
preferred interface (meaning it is better supported and you'll have lots of
company).  Magic's X11 driver has a long history, starting with an X10 driver
by Doug Pan at Stanford.  Brown University, the University of 
Southern California, the University of Washington, and Lawrence Livermore
National Labs all prepared improved versions, some of them for X11.  Don
Stark of Stanford took on the task of pulling these together and producing
the X11 driver in this release.
.PP
Magic runs on a number of workstations, such as the DECstation 3100 and Sun's
SPARC processors.  Partial Unix System V support is provided, via the 
compilation flags mentioned below.  The system also runs on the MacII.
Don Stark gets credit for the System V mods and support for HP machines, while
Mike Chow helped get it running on the MacII.
.PP
To assist people with small machines (such as the Mac II), Magic can now be
compiled without some of its fancy features.  Compilation flags are provided,
as indicated below, to eliminate things like routing, plotting, or calma output.
This is courtesy of Don Stark.
.RE
.IP "Reorganization of Magic Source Directory"
.RS
.PP
Magic, as previously distributed, was set up with the assumption that lots of
people would be changing the code at the same time.  As a result, the makefiles
did all sorts of paranoid things like making extra copies of the source code
whenever a module was re-installed.  
.PP
Since Magic is more stable now, this copying is no longer needed.  Instead,
each makefile invokes the script \fB../:instclean\fR after installing a module.
This script,
by default, doesn't copy the source code but does leave the .o files around.
This cuts down on the disk space needed by a factor of two.
You can change the script if you want the copying, 
or if you want to delete unused .o files to save even more disk space.
.RE
.IP "Lots of bug fixes - \fIVarious"
.RS
.PP
Lots of bugs have been fixed in this release.  We'd like to thank everybody
that has reported bugs in the past.  If you find a new bug, please report
it as mentioned below.
.RE
.RE
.NH 1
Distribution Information
.LP
This version of Magic is available via FTP.  Contact "magic@decwrl.dec.com"
for information.
.LP
For a handling fee, this version of Magic may be obtained on magnetic tape from:
.DS C
EECS/ERL Industrial Liaison Program
479 Cory Hall
University of California at Berkeley
Berkeley, CA  94720
.DE
.DE
.NH 1 
Bug Reports
.PP
Maintenance of Magic is a volunteer effort.  Please send descriptions
of bugs via InterNet e-mail to "magic@decwrl.dec.com" or via Uucp e-mail
to "decwrl!magic".  If you develop a fix for the problem, 
please send that too!  
.NH 1
Changes for Magic maintainers
.PP
Previous releases of Magic expected to find their
system files in the home directory of the user \fBcad\fR.
The default behavior of version 6 is no different, but it
is possible to put the files in another directory by setting the \fBCAD_HOME\fR
shell environment variable.  If this variable is set, magic will use that
location instead of the ~cad it finds in the password file.
.NH 2
INSTALLING MAGIC
.PP
The distribution tape contains a version of Magic ready to run on Digital's
line of Ultrix RISC workstations, such as the DECstation 3100.  For other 
machines, read ahead.
In any event, all users should set their shell environment variable 
CAD_HOME to point to the place where the tape is loaded, unless that place
is ~cad, in which case things will default correctly.
.PP
Before installing Magic, you should set your shell environment variable 
CAD_HOME to point to the place where you loaded the tape.
If you "cd" to the magic source directory (${CAD_HOME}/src/magic) you will find
a makefile.  A "\fBmake config\fR" will run a configuration script that asks
questions about your configuration and sets up magic to be compiled for your
local environment.
.PP
After running a "make config", you can run a "\fBmake force\fR" to force 
a complete
recompilation of magic.  A "\fBmake install\fR" will then copy the binaries to
the ${CAD_HOME}/bin area, as well as install things in ${CAD_HOME}/lib 
and ${CAD_HOME}/man.  
.PP
Included in this documentation
is a set of Magic maintainer's manuals.  These should be read by anybody
interested in modifying Magic or by anybody that is having difficulty 
installing it on their system.
.NH 2
Technology file changes
.PP
Users of Magic 4 should have little trouble switching to Magic 6.  
.PP
A new section, the \fBmzrouter\fR section needs to be added to your technology
files.  See the mzrouter section of the 
tutorial \fIMagic Maintainer's Manual #2: The Technology File\fR 
for details.
.PP
Display styles must be defined in the \fI.tech\fR file
for the mzrouter hint layers magnet, fence and rotate.  We
suggest copying this information from the styles section of the
scmos technology file on the distribution tape.  You'll also need to include
these display styles in your \fI.dstyle\fR file.
.NH 1
Beta-test Sites
.PP
We'd like to thank the beta-test sites that tried out this version of Magic,
reported bugs and fixes in a timely manner, and ported the code to new 
machines:  
.DS I
Mike Chow, Apple Computer
Arun Rao, Arizona State University
Richard Hughey, Brown University
Rick Carley, Carnegie-Mellon University
Hank Walker, Carnegie-Mellon University
Christos Zoulas, Cornell University
Andreas Andreou, John Hopkins University
George Entenman, The Microelectronics Center of North Carolina 
Shih-Lien Lu, The MOSIS Service
Jen-I Pi, The MOSIS Service
Guntram Wolski,	Silicon Engineering, Inc.
Don Stark, Stanford University
Gregory Frazier, University of California at Los Angeles
Yuval Tamir, University of California at Los Angeles
Steven Parkes, University of Illinois
Larry McMurchie, University of Washington
Tim Heldt, Washington State University
David Lee, Xerox Palo Alto Research Center
.DE
Martin Harriman of Silicon Engineering wrote a "select less" command for 
Magic during the beta-test phase.  "Select less" has been a 
much-requested feature.
.PP
In addition to the persons named above, there were many other beta-test
users of Magic at these and other sites -- too many to list here.  We 
appreciate their help.  We also acknowledge the help of the pre-release
sites, who tested a version that included most of the fixes from the beta-test
phase.
