/*	$Header: vdmpc.c,v 6.0 90/08/28 18:52:09 mayo Exp $	*/

#ifndef lint
static char sccsid[] = "@(#)vdmp.c	4.4 (Berkeley) 7/16/83";
#endif

/*
 *  reads raster file created by cifplot and dumps it onto the
 *  Varian or Versatec plotter.
 *  Assumptions:
 *	Input is from device 0.
 *	plotter is already opened as device 1.
 *	error output file is device 2.
 */
#include <stdio.h>
#include <sys/vcmd.h>

#define IN	0
#define OUT	1

#define MAGIC_WORD	0xA5CF4DFB

#define BUFSIZE		1024*128
#define BLOCK		1024
#define MAX(a,b)        (((a) < (b)) ? (b) : (a))
#define MIN(a,b)        (((a) > (b)) ? (b) : (a))
#define ABS(x)          (((x) >= 0)  ? (x) : -(x))


static char *Sid = "@(#)vdmp.c	4.4\t7/16/83";

int	plotmd[] = { VPLOT };
int	prtmd[]	= { VPRINT };

int	inbuf[BLOCK/sizeof(int)];
char	buf[BUFSIZE];
int	lines;

int	varian;			/* 0 for versatec, 1 for varian. */
int	BYTES_PER_LINE;		/* number of bytes per raster line. */
int	PAGE_LINES;		/* number of raster lines per page. */
int	in_width;		/* Width in pixels of one input line. */
int	in_bytes;		/* Width in bytes of one input line. */

char	*name, *host, *acctfile;

main(argc, argv)
	int argc;
	char *argv[];
{
	register int n;

	while (--argc) {
		if (**++argv == '-') {
			switch (argv[0][1]) {
			case 'x':
				BYTES_PER_LINE = atoi(&argv[0][2]) / 8;
				varian = BYTES_PER_LINE == 264;
				break;

			case 'y':
				PAGE_LINES = atoi(&argv[0][2]);
				break;

			case 'n':
				argc--;
				name = *++argv;
				break;

			case 'h':
				argc--;
				host = *++argv;
			}
		} else
			acctfile = *argv;
	}

	n = read(IN, inbuf, BLOCK);
	if (inbuf[0] == MAGIC_WORD && n == BLOCK) {
		/* we have a formatted dump file 
		 *    word 1:  MAGIC_WORD
		 *    word 2:  number of scan lines
		 *    word 3:  width in pixels
		 *    rest of block:  not used, set to zero
		 *
		 *    Bytes of data follow.  Images are packed back-to-back
		 *    in the order black, cyan, magenta, and yellow.
		 */
		int in_lines, dpi, inches;
		char pass_cntl[8], rewind_cntl[4];
		char *out_buf;
#define		PASS1	0x04
#define		PASS2	0x09
#define		PASS3	0x0A
#define		PASS4	0x0B

		in_lines = inbuf[1];
		in_width = inbuf[2];
		in_bytes = in_width / 8;
		dpi = PAGE_LINES/12;
		inches = ((in_lines + dpi-1)/dpi);
		out_buf = (char *) malloc(MAX(BYTES_PER_LINE, in_bytes) + 4);

		/*
		fprintf(stderr, "in_lines = %d\n", in_lines);
		fprintf(stderr, "in_width = %d\n", in_width);
		fprintf(stderr, "in_bytes = %d\n", in_bytes);
		fprintf(stderr, "dpi = %d\n", dpi);
		fprintf(stderr, "inches = %d\n", inches);
		*/

/* The control codes needed by the color versatec model ECP42 follow.
 *
 * Each pass (black, magenta, cyan, yellow) is preceded by a color preamble.
 * The first bytes are 0x9b(meta-escape) 0x50('P') 0x00 0x04(byte count).
 * This is followed by the plot length in inches (rounded up to the nearest
 * inch) as 2 bytes, MSB first.  This length is ignored after the first pass.
 * Next comes a "plotter control byte", which is encoded thus:
 *
 *     bits 3,2                         bits 1,0
 *        00   single pass            00      black
 *        01   first pass (print      01      cyan
 *             tickmarks)             10      magenta
 *        10   intermediate pass      11      yellow
 *        11   unused
 *
 *       The last byte of the preamble is a null (0x00) which Versatec
 * "reserves for future use".
 *       The first pass must be black, and must orint tickmarks.  The
 * remainder of the passes must be "intermediate passes".
 * Each intermediate pass must be preceded by a "rewind" command to scroll
 * the paper back to the beginning.  The rewind command is the sequence of 
 * bytes:
 *      0x9b 0x57 0x00 0x00
 */

		/* Color preamble */
		pass_cntl[0] = 0x9b;	/* ESC P */
		pass_cntl[1] = 0x50;
		pass_cntl[2] = 0x00;	/* 4 bytes to follow */
		pass_cntl[3] = 0x04;
		pass_cntl[4] = (inches >> 8) & 0xFF;  /* Plot length in inches */
		pass_cntl[5] = inches & 0xFF;
		pass_cntl[6] = PASS1;	/* "plotter control byte" */
		pass_cntl[7] = 0x00;

		/* Rewind command. */
		rewind_cntl[0] = 0x9B;
		rewind_cntl[1] = 0x57;
		rewind_cntl[2] = 0x00;
		rewind_cntl[3] = 0x00;
		
		/* Black pass */
		ioctl(OUT, VSETSTATE, prtmd);
		pass_cntl[6] = PASS1;	
		write(OUT, pass_cntl, 8);
		ioctl(OUT, VSETSTATE, plotmd);
		copylines(out_buf, in_lines);
		
		/* Cyan pass */
		ioctl(OUT, VSETSTATE, prtmd);
		write(OUT, rewind_cntl, 4);
		pass_cntl[6] = PASS2;	
		write(OUT, pass_cntl, 8);
		ioctl(OUT, VSETSTATE, plotmd);
		copylines(out_buf, in_lines);
		
		/* Magenta pass */
		ioctl(OUT, VSETSTATE, prtmd);
		write(OUT, rewind_cntl, 4);
		pass_cntl[6] = PASS3;	
		write(OUT, pass_cntl, 8);
		ioctl(OUT, VSETSTATE, plotmd);
		copylines(out_buf, in_lines);
		
		/* Yellow pass */
		ioctl(OUT, VSETSTATE, prtmd);
		write(OUT, rewind_cntl, 4);
		pass_cntl[6] = PASS4;	
		write(OUT, pass_cntl, 8);
		ioctl(OUT, VSETSTATE, plotmd);
		copylines(out_buf, in_lines);
		
		/* Advance to viewing area */
		ioctl(OUT, VSETSTATE, prtmd);
		write(OUT, "\f", 2);
	} else {			/* dump file not formatted */
		lseek(IN, 0L, 0);	/* reset in's seek pointer and plot */
		n = putplot();
		/* page feed */
		ioctl(OUT, VSETSTATE, prtmd);
		if (varian)
			write(OUT, "\f", 2);
		else
			write(OUT, "\n\n\n\n\n", 6);
	}

	account(name, host, acctfile);
	exit(n);
}

copylines(data_buf, datalines)
	char data_buf[];
	int datalines;
{
	register int lines, n;
	static int msg = 0;
	int num_read, i;

	/* Pad end of scan line, if needed. */
	for (i = in_bytes; i < BYTES_PER_LINE; i++) data_buf[i] = 0x00;  

	/* Process 1 scan line during each iteration. */
	for (lines = 0; lines < datalines; lines++)
	{
	    /* When this loop terminates, 1 scan line will have been read. */
	    for (num_read = 0; num_read < in_bytes; )
	    {
		if ((n = read(IN, &data_buf[num_read], in_bytes - num_read)) <= 0)
		{
		    if (!msg) 
		    {
			fprintf(stderr, "vdmpc:  ran out of scan line data, using zeros.\n");
			if (n == 0)
			    fprintf(stderr, "vdmpc:  (End of File)\n");
			else
			    perror("vdmpc");
			msg = 1;
		    }
		    for (i = num_read; i < in_bytes; i++) data_buf[i] = 0x00;  
		    num_read = in_bytes;
		}
		else
		    num_read += n;
	    }

	    /* Write 1 scan line.  We write the width of the physical device,
	     * which may be wider or narrower than the input scan line. 
	     */
	    if (write(OUT, data_buf, BYTES_PER_LINE) != BYTES_PER_LINE)
	    {
		    fprintf(stderr, "vdmpc:  couldn't write line of data.\n");
		    perror("vdmpc");
		    return(2);
	    }
	}

	return(0);
}

putplot()
{
	register char *cp;
	register int bytes, n;

	cp = buf;
	bytes = 0;
	ioctl(OUT, VSETSTATE, plotmd);
	while ((n = read(IN, cp, sizeof(buf))) > 0) {
		if (write(OUT, cp, n) != n)
			return(1);
		bytes += n;
	}
	/*
	 * Make sure we send complete raster lines.
	 */
	if ((n = bytes % BYTES_PER_LINE) > 0) {
		n = BYTES_PER_LINE - n;
		for (cp = &buf[n]; cp > buf; )
			*--cp = 0;
		if (write(OUT, cp, n) != n)
			return(1);
		bytes += n;
	}
	lines += bytes / BYTES_PER_LINE;
	return(0);
}

account(who, from, acctfile)
	char *who, *from, *acctfile;
{
	register FILE *a;

	if (who == NULL || acctfile == NULL)
		return;
	if (access(acctfile, 02) || (a = fopen(acctfile, "a")) == NULL)
		return;
	/*
	 * Varian accounting is done by 8.5 inch pages;
	 * Versatec accounting is by the (12 inch) foot.
	 */
	fprintf(a, "t%6.2f\t", (lines / 200.0) / PAGE_LINES);
	if (from != NULL)
		fprintf(a, "%s:", from);
	fprintf(a, "%s\n", who);
	fclose(a);
}
