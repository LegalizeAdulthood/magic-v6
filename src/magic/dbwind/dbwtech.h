/*
 * dbwtech.h --
 *
 * Style information for display.
 * MAXTILESTYLES is the maximum number of styles usable for display
 * of tiles.
 *
 * rcsid $Header: dbwtech.h,v 6.0 90/08/28 18:11:34 mayo Exp $
 */

#define	_DISTECH

/*
 * Alas, we need to be able to handle more than 32
 * different display styles, so we can't map back
 * from types into styles with a bitmask.
 */
#define	MAXTILESTYLES	64

extern TileTypeBitMask	DBWStyleToTypesTbl[MAXTILESTYLES];

#define	DBWStyleToTypes(s)	(&DBWStyleToTypesTbl[s])
