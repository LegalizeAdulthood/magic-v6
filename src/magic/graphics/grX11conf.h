/* Redefine data types so that Magic's names don't conflict with X's names.
 */
typedef Window MagicWindow; /* Name for Magic's def of a window. */
#define Window XWindow	    /* Name for X's def or a window. */
			    /* From now on, referring to "Window" will
			     * use X's definitioin.
			     */
