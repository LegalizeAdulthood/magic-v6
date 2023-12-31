/*
 * dqueue.h --
 *
 * Utility module for double ended queues (dqueus).
 * DQueues are stored in a malloced array which grows when it overflows.
 * The head of the queue grows towards lower addresses, and the tail grows
 * up.  The array is treated like a cirular ring of size dq_maxSize.
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
 *
 * rcsid "$Header: dqueue.h,v 6.0 90/08/28 19:00:34 mayo Exp $"
 *
 */

#define	_DQUEUE

#ifndef	_MAGIC
int err = Need_to_include_magic_header;
#endif	_MAGIC


/* The dqueue structure, to be manipulated only by the 
 * procedures declared in this file.
 */
typedef struct {
    int dq_size;	    /* number of elements in this dqueue */
    int dq_maxSize;	    /* max capacity of this dqueue */
    int dq_front;	    /* next empty location at head of dqueue */
    int dq_rear;	    /* next empty location at tail of dqueue */
    ClientData *dq_data;    /* points to an array (filled in by DQInit) */
} DQueue;


/* Housekeeping for DQueues */
extern void DQInit();	    /* Sets up data in an already allocated DQueue */
extern void DQFree();
extern void DQChangeSize();
extern void DQCopy();
#define DQIsEmpty(q)	((q)->dq_size == 0)

/* Adding and deleting */
extern void DQPushFront();
extern void DQPushRear();
extern ClientData DQPopFront();
extern ClientData DQPopRear();

