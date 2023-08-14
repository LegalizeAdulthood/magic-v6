/* CIFreadtech.c -
 *
 *	This module processes the portions of technology files that
 *	pertain to reading CIF files, and builds the tables used by
 *	the CIF-reading code.
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1985, 1990 Regents of the University of California. * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  The University of California        * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
 */

#ifndef lint
static char rcsid[] = "$Header: CIFrdtech.c,v 6.1 90/09/03 14:33:24 stark Exp $";
#endif  not lint

#include <stdio.h>
#include <ctype.h>
#include "magic.h"
#include "geometry.h"
#include "tile.h"
#include "hash.h"
#include "database.h"
#include "tech.h"
#include "textio.h"
#include "utils.h"
#include "CIFint.h"
#include "CIFread.h"
#include "calmaInt.h"
#include "malloc.h"
#ifdef SYSV
#include <string.h>
#endif

/* Pointer to a list of all the CIF-reading styles: */

CIFReadStyle *cifReadStyleList;

/* Names of all the CIF layer types used by any read style: */

int cifNReadLayers = 0;
char *(cifReadLayers[MAXCIFRLAYERS]);

/* Table mapping from Calma layer numbers to CIF layers */
HashTable cifCalmaToCif;

/* Variables used to keep track of progress in reading the tech file: */

CIFReadStyle *cifCurReadStyle;		/* Current style being read. */
CIFReadLayer *cifCurReadLayer;		/* Current layer being processed. */
CIFOp *cifCurReadOp;			/* Last geometric operation seen. */

/*
 * ----------------------------------------------------------------------------
 *
 * CIFReadNameToType --
 *
 * 	This procedure finds the type (integer index) of a given
 *	layer name.
 *
 * Results:
 *	The return value is the type.  If we ran out of space in
 *	the CIF layer table, or if the layer wasn't recognized and
 *	it isn't OK to make a new layer, -1 gets returned.
 *
 * Side effects:
 *	If no layer exists by the given name and newOK is TRUE, a
 *	new layer is created.
 *
 * ----------------------------------------------------------------------------
 */

int
CIFReadNameToType(name, newOK)
    char *name;		/* Name of a CIF layer. */
    bool newOK;		/* TRUE means OK to create a new layer if this
			 * name is one we haven't seen before.
			 */
{
    int i;
    static bool errorPrinted = FALSE;

    for (i=0; i < cifNReadLayers; i += 1)
    {
	/* Only accept this layer if it's in the current CIF style or
	 * it's OK to add new layers to the current style.
	 */
	
	if (!TTMaskHasType(&cifCurReadStyle->crs_cifLayers, i) && !newOK)
	    continue;
	if (strcmp(cifReadLayers[i], name) == 0)
	{
	    if (newOK) TTMaskSetType(&cifCurReadStyle->crs_cifLayers, i);
	    return i;
	}
    }

    /* This name isn't in the table.  Return an error or make a new entry. */

    if (!newOK) return -1;

    if (cifNReadLayers == MAXCIFRLAYERS)
    {
	if (!errorPrinted)
	{
	    TxError("CIF read layer table ran out of space at %d layers.\n",
		    MAXCIFRLAYERS);
	    TxError("Get your Magic maintainer to increase the table size.\n");
	    errorPrinted = TRUE;
	}
	return -1;
    }

    (void) StrDup(&(cifReadLayers[cifNReadLayers]), name);
    TTMaskSetType(&cifCurReadStyle->crs_cifLayers, cifNReadLayers);
    cifNReadLayers += 1;
    return cifNReadLayers-1;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFCalmaLayerToCifLayer --
 *
 * Find the CIF number of the layer matching the supplied Calma
 * layer number and datatype.
 *
 * Results:
 *	Returns the CIF number of the above layer, or -1 if it
 *	can't be found.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

int
CIFCalmaLayerToCifLayer(layer, datatype)
    int layer;		/* Calma layer number */
    int datatype;	/* Calma datatype */
{
    CalmaLayerType clt;
    HashEntry *he;

    clt.clt_layer = layer;
    clt.clt_type = datatype;
    if (he = HashLookOnly(&cifCalmaToCif, (char *) &clt))
	return ((int) HashGetValue(he));

    /* Try wildcarding the datatype */
    clt.clt_type = -1;
    if (he = HashLookOnly(&cifCalmaToCif, (char *) &clt))
	return ((int) HashGetValue(he));

    /* Try wildcarding the layer */
    clt.clt_layer = -1;
    clt.clt_type = datatype;
    if (he = HashLookOnly(&cifCalmaToCif, (char *) &clt))
	return ((int) HashGetValue(he));

    /* Try wildcarding them both, for a default value */
    clt.clt_layer = -1;
    clt.clt_type = -1;
    if (he = HashLookOnly(&cifCalmaToCif, (char *) &clt))
	return ((int) HashGetValue(he));

    /* No luck */
    return (-1);
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFParseReadLayers --
 *
 * 	Given a comma-separated list of CIF layer names, builds a
 *	bit mask of all those layer names.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the parameter pointed to by mask so that it contains
 *	a mask of all the CIF layers indicated.  If any of the CIF
 *	layers didn't exist, new ones are created.  If we run out
 *	of CIF layers, an error message is output.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFParseReadLayers(string, mask)
    char *string;		/* Comma-separated list of CIF layers. */
    TileTypeBitMask *mask;	/* Where to store bit mask. */
{
    int i;
    char *p;

    TTMaskZero(mask);

    /* Break the string up into the chunks between commas. */

    while (*string != 0)
    {
	p = index(string, ',');
	if (p != NULL)
	    *p = 0;
	
	i = CIFReadNameToType(string, TRUE);
	if (i >= 0)
	    TTMaskSetType(mask, i);

	if (p == NULL) break;
	*p = ',';
	for (string = p; *string == ','; string += 1) /* do nothing */;
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifNewReadStyle --
 *
 * 	This procedure creates a new CIF read style at the end of
 *	the list of styles and initializes it to completely null.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new element is added to the end of cifReadStyleList, and
 *	cifCurReadStyle is set to point to it.
 *
 * ----------------------------------------------------------------------------
 */

void
cifNewReadStyle(name)
    char *name;			/* Name of new style. */
{
    CIFReadStyle *p;
    int i;

    cifCurReadStyle = (CIFReadStyle *) mallocMagic(sizeof(CIFReadStyle));
    if (cifReadStyleList == NULL)
	cifReadStyleList = cifCurReadStyle;
    else
    {
	for (p = cifReadStyleList; p->crs_next != NULL; p = p->crs_next)
	    /* Just loop. */;
	p->crs_next = cifCurReadStyle;
    }
    cifCurReadStyle->crs_name = NULL;
    (void) StrDup(&cifCurReadStyle->crs_name, name);
    cifCurReadStyle->crs_cifLayers = DBZeroTypeBits;
    cifCurReadStyle->crs_nLayers = 0;
    cifCurReadStyle->crs_scaleFactor = 0;
    for (i=0; i<MAXCIFRLAYERS; i+=1)
    {
	cifCurReadStyle->crs_labelLayer[i] = TT_SPACE;
	cifCurReadStyle->crs_layers[i] = NULL;
    }
    cifCurReadStyle->crs_next = NULL;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFReadTechInit --
 *
 * 	Called once at the beginning of technology file read-in to
 *	initialize data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears out the layer data structure.
 *
 * ----------------------------------------------------------------------------
 */

Void
CIFReadTechInit()
{
    cifReadStyleList = NULL;
    cifNReadLayers = 0;
    cifCurReadStyle = NULL;
    cifCurReadLayer = NULL;
    cifCurReadOp = NULL;
    HashInit(&cifCalmaToCif, 64, sizeof (CalmaLayerType) / sizeof (unsigned));
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFReadTechLine --
 *
 * 	This procedure is called once by the tech module for each line
 *	in the "cifinput" section of the technology file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up information in the tables used to read CIF, and prints
 *	error messages if problems arise.
 *
 * ----------------------------------------------------------------------------
 */
	/* ARGSUSED */
bool
CIFReadTechLine(sectionName, argc, argv)
    char *sectionName;		/* Name of this section ("cifinput"). */
    int argc;			/* Number of fields on line. */
    char *argv[];		/* Values of fields. */
{
    CIFOp *newOp = NULL;
    HashEntry *he;
    CalmaLayerType clt;
    int calmaLayers[CALMA_LAYER_MAX], calmaTypes[CALMA_LAYER_MAX];
    int nCalmaLayers, nCalmaTypes, l, t;

    if (argc <= 0) return TRUE;

    /* See if we're starting a new style.  If so, create it.  If not,
     * make sure there's already a style around, and create one if
     * there isn't.
     */
    
    if (strcmp(argv[0], "style") == 0)
    {
	if (argc != 2)
	{
	    wrongNumArgs:
	    TechError("Wrong number of arguments in %s statement.\n",
		    argv[0]);
	    errorReturn:
	    if (newOp != NULL)
		freeMagic((char *) newOp);
	    return TRUE;
	}
	cifNewReadStyle(argv[1]);
	return TRUE;
    }
    
    if (cifCurReadStyle == NULL)
	cifNewReadStyle("unnamed");
    
    /* Process scalefactor lines next. */

    if (strcmp(argv[0], "scalefactor") == 0)
    {
	if (argc != 2) goto wrongNumArgs;
	cifCurReadStyle->crs_scaleFactor = atoi(argv[1]);
	if (cifCurReadStyle->crs_scaleFactor <= 0)
	{
	    cifCurReadStyle->crs_scaleFactor = 0;
	    TechError("Scalefactor must be positive.\n");
	    goto errorReturn;
	}
	return TRUE;
    }

    /* Process layer lines next. */

    if (strcmp(argv[0], "layer") == 0)
    {
	TileType type;

	cifCurReadLayer = NULL;
	cifCurReadOp = NULL;
	if (cifCurReadStyle->crs_nLayers == MAXCIFRLAYERS)
	{
	    TechError("Can't handle more than %d layers per style.\n",
		    MAXCIFRLAYERS);
	    TechError("Your local Magic wizard can increase the table size.\n");
	    goto errorReturn;
	}
        if ((argc != 2) && (argc != 3)) goto wrongNumArgs;
	type = DBTechNoisyNameType(argv[1]);
	if (type < 0) goto errorReturn;

	cifCurReadLayer = (CIFReadLayer *) mallocMagic(sizeof(CIFReadLayer));
	cifCurReadStyle->crs_layers[cifCurReadStyle->crs_nLayers]
		= cifCurReadLayer;
	cifCurReadStyle->crs_nLayers += 1;
	cifCurReadLayer->crl_magicType = type;
	cifCurReadLayer->crl_ops = NULL;
	cifCurReadLayer->crl_flags = CIFR_SIMPLE;

	/* Handle a special case of a list of layer names on the
	 * layer line.  Turn them into an OR operation.
	 */
	
	if (argc == 3)
	{
	    cifCurReadOp = (CIFOp *) mallocMagic(sizeof(CIFOp));
	    cifCurReadOp->co_opcode = CIFOP_OR;
	    CIFParseReadLayers(argv[2], &cifCurReadOp->co_cifMask);
	    TTMaskZero(&cifCurReadOp->co_paintMask);
	    cifCurReadOp->co_next = NULL;
	    cifCurReadLayer->crl_ops = cifCurReadOp;
	}
	return TRUE;
    }

    /* Process mapping between CIF layers and calma layers/types */
    if (strcmp(argv[0], "calma") == 0)
    {
	int cifnum;

	if (argc != 4) goto wrongNumArgs;
	cifnum = CIFReadNameToType(argv[1], FALSE);
	if (cifnum < 0)
	{
	    TechError("Unrecognized CIF layer: \"%s\"\n", argv[1]);
	    return TRUE;
	}
	nCalmaLayers = cifParseCalmaNums(argv[2], calmaLayers, CALMA_LAYER_MAX);
	nCalmaTypes = cifParseCalmaNums(argv[3], calmaTypes, CALMA_LAYER_MAX);
	if (nCalmaLayers <= 0 || nCalmaTypes <= 0)
	    return (TRUE);

	for (l = 0; l < nCalmaLayers; l++)
	{
	    for (t = 0; t < nCalmaTypes; t++)
	    {
		clt.clt_layer = calmaLayers[l];
		clt.clt_type = calmaTypes[t];
		he = HashFind(&cifCalmaToCif, (char *) &clt);
		HashSetValue(he, (ClientData) cifnum);
	    }
	}
	return TRUE;
    }

    /* Figure out which Magic layer should get labels from which
     * CIF layers.
     */
    
    if (strcmp(argv[0], "labels") == 0)
    {
	TileTypeBitMask mask;
	int i;

	if (cifCurReadLayer == NULL)
	{
	    TechError("Must define layer before giving labels it holds.\n");
	    goto errorReturn;
	}
	if (argc != 2) goto wrongNumArgs;
	CIFParseReadLayers(argv[1], &mask);
	for (i=0; i<MAXCIFRLAYERS; i+=1)
	{
	    if (TTMaskHasType(&mask,i))
		cifCurReadStyle->crs_labelLayer[i]
			= cifCurReadLayer->crl_magicType;
	}
	return TRUE;
    }

    /* Parse "ignore" lines:  look up the layers to enter them in
     * the table of known layers, but don't do anything else.  This
     * will cause the layers to be ignored when encountered in
     * cells.
     */
    
    if (strcmp(argv[0], "ignore") == 0)
    {
	TileTypeBitMask mask;
	int		i;

	if (argc != 2) goto wrongNumArgs;
	CIFParseReadLayers(argv[1], &mask);
	/* trash the value in crs_labelLayer so that any labels on this
	   layer get junked, also. dcs 4/11/90
        */
	for (i=0; i < cifNReadLayers; i++)
	{
	     if (TTMaskHasType(&mask,i))
	     {
	     	  if (cifCurReadStyle->crs_labelLayer[i] == TT_SPACE)
		  {
		       cifCurReadStyle->crs_labelLayer[i] = -1;
		  }
	     }
	}
	return TRUE;
    }

    /* Anything below here is a geometric operation, so we can
     * do some set-up that is common to all the operations.
     */
    
    if (cifCurReadLayer == NULL)
    {
	TechError("Must define layer before specifying operations.\n");
	goto errorReturn;
    }
    newOp = (CIFOp *) mallocMagic(sizeof(CIFOp));
    TTMaskZero(&newOp->co_paintMask);
    TTMaskZero(&newOp->co_cifMask);
    newOp->co_opcode = 0;
    newOp->co_next = NULL;

    if (strcmp(argv[0], "and") == 0)
	newOp->co_opcode = CIFOP_AND;
    else if (strcmp(argv[0], "and-not") == 0)
	newOp->co_opcode = CIFOP_ANDNOT;
    else if (strcmp(argv[0], "or") == 0)
	newOp->co_opcode = CIFOP_OR;
    else if (strcmp(argv[0], "grow") == 0)
	newOp->co_opcode = CIFOP_GROW;
    else if (strcmp(argv[0], "shrink") == 0)
	newOp->co_opcode = CIFOP_SHRINK;
    else
    {
	TechError("Unknown statement \"%s\".\n", argv[0]);
	goto errorReturn;
    }

    switch (newOp->co_opcode)
    {
	case CIFOP_AND:
	case CIFOP_ANDNOT:
	case CIFOP_OR:
	    if (argc != 2) goto wrongNumArgs;
	    CIFParseReadLayers(argv[1], &newOp->co_cifMask);
	    break;
	
	case CIFOP_GROW:
	case CIFOP_SHRINK:
	    if (argc != 2) goto wrongNumArgs;
	    newOp->co_distance = atoi(argv[1]);
	    if (newOp->co_distance <= 0)
	    {
		TechError("Grow/shrink distance must be greater than zero.\n");
		goto errorReturn;
	    }
	    break;
    }

    /* Link the new CIFOp onto the list. */

    if (cifCurReadOp == NULL)
    {
	cifCurReadLayer->crl_ops = newOp;
	if (newOp->co_opcode != CIFOP_OR)
	    cifCurReadLayer->crl_flags &= ~CIFR_SIMPLE;
    }
    else
    {
        cifCurReadOp->co_next = newOp;
	cifCurReadLayer->crl_flags &= ~CIFR_SIMPLE;
    }
    cifCurReadOp = newOp;

    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFReadTechFinal --
 *
 * 	This procedure is invoked after all the lines of a technology
 *	file have been read.  It checks to make sure that the information
 *	read in "cifinput" sections is reasonably complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Error messages may be output.
 *
 * ----------------------------------------------------------------------------
 */

Void
CIFReadTechFinal()
{
    CIFReadStyle *style;

    for (style = cifReadStyleList; style != NULL; style = style->crs_next)
    {
	/* Make sure every style has a valid scalefactor. */

	if (style->crs_scaleFactor <= 0)
	{
	    TechError("CIF input style \"%s\" bad scalefactor; using 1.\n",
		    style->crs_name);
	    style->crs_scaleFactor = 1;
	}
    }

    /* Make the first style the current one. */

    cifCurReadStyle = cifReadStyleList;
}

/*
 * ----------------------------------------------------------------------------
 *
 * CIFSetReadStyle --
 *
 * 	This procedure changes the current style used for reading
 *	CIF.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The CIF style is changed to the one specified by name.  If
 *	there is no style by that name, then a list of all valid
 *	styles is output.
 *
 * ----------------------------------------------------------------------------
 */

void
CIFSetReadStyle(name)
    char *name;			/* Name of the new style.  If NULL,
				 * just print the name of the current
				 * style.
				 */
{
    CIFReadStyle *style, *match;
    int length;

    match = NULL;
    if (name == NULL) goto badStyle;
    length = strlen(name);
    for (style = cifReadStyleList; style != NULL; style = style->crs_next)
    {
	if (strncmp(name, style->crs_name, length) == 0)
	{
	    if (match != NULL)
	    {
		TxError("CIF input style \"%s\" is ambiguous.\n", name);
		goto badStyle;
	    }
	    match = style;
	}
    }

    if (match != NULL)
    {
	cifCurReadStyle = match;
	return;
    }

    TxError("\"%s\" is not one of the CIF input styles Magic knows.\n", name);
    badStyle:
    TxPrintf("The CIF input styles are: ");
    for (style = cifReadStyleList; style != NULL; style = style->crs_next)
    {
	if (style == cifReadStyleList)
	    TxPrintf("%s", style->crs_name);
	else TxPrintf(", %s", style->crs_name);
    }
    TxPrintf(".\n");
    if (cifCurReadStyle != NULL)
        TxPrintf("The current style is \"%s\".\n", cifCurReadStyle->crs_name);
}

/*
 * ----------------------------------------------------------------------------
 *
 * cifParseCalmaNums --
 *
 * Parse a comma-separated list of Calma numbers.  Each number in
 * the list must be between 0 and CALMA_LAYER_MAX, or an asterisk
 * "*".  Store each number in the array 'numArray', which has space
 * for up to 'numNums' numbers.  An asterisk is stored as -1.
 *
 * Results:
 *	Returns the number of numbers added to the array, or
 *	-1 on error.
 *
 * Side effects:
 *	Adds numbers to the array.  If there were too many numbers,
 *	or some of the numbers were not legal Calma numbers, we
 *	print an error message.
 *
 * ----------------------------------------------------------------------------
 */

int
cifParseCalmaNums(str, numArray, numNums)
    register char *str;	/* String to parse */
    int *numArray;	/* Array to fill in */
    int numNums;	/* Maximum number of entries in numArray */
{
    int numFilled, num;

    for (numFilled = 0; numFilled < numNums; numFilled++)
    {
	/* Done if at end of string */
	if (*str == '\0')
	    return (numFilled);

	/* Is it a wild-card (*)? */
	if (*str == '*') num = -1;
	else
	{
	    num = atoi(str);
	    if (num < 0 || num > CALMA_LAYER_MAX)
	    {
		TechError("Calma layer and type numbers must be 0 to %d.\n",
		    CALMA_LAYER_MAX);
		return (-1);
	    }
	}

	/* Skip to next number */
	while (*str && *str != ',')
	{
	    if (*str != '*' && !isdigit(*str))
	    {
		TechError("Calma layer/type numbers must be numeric or '*'\n");
		return (-1);
	    }
	    str++;
	}

	while (*str && *str == ',') str++;
	numArray[numFilled] = num;
    }

    TechError("Too many layer/type numbers in line; maximum = %d\n", numNums);
    return (-1);
}
