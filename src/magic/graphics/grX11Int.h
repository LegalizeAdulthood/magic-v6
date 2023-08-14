/*
 * grX11Int.h --
 *
 * Internal definitions for grX11su[1..5].c.
 *
 * NOTE:  In order for the these defs to work correctly, this file
 * (grXInt.h) must be included after all the Magic .h files and before
 * the X .h files.
 */

/* Constants
 */
#define M_WIDTH		1023
#define M_HEIGHT	750

#define MAX_CURSORS	32	/* Maximum number of programmable cursors */

#define GR_DEFAULT_FONT "9x15"

#define true		1
#define false		0

#undef TRUE

#define grMagicToX(y) ( grCurrent.mw->w_allArea.r_ytop - (y))
#define grXToMagic(y) ( grCurrent.mw->w_allArea.r_ytop - (y))

#ifdef	OLD_R2_FONTS
/*
 * Some machines still run release 2 of X.
 */
# define       X_FONT_SMALL    "vg-13"
# define       X_FONT_MEDIUM   "fg-18"
# define       X_FONT_LARGE    "vrb-25"
# define       X_FONT_XLARGE   "vrb-37"
#else
/*
 * Our default fonts for X11.  (Release 3 fonts.)
 */

# define	X_FONT_SMALL	"-*-helvetica-medium-r-normal--10-*-75-75-p-*-iso8859-*"
# define	X_FONT_MEDIUM	"-*-helvetica-medium-r-normal--14-*-75-75-p-*-iso8859-*"
# define	X_FONT_LARGE	"-*-helvetica-medium-r-normal--18-*-75-75-p-*-iso8859-*"
# define	X_FONT_XLARGE 	"-*-helvetica-medium-r-normal--24-*-75-75-p-*-iso8859-*"
#endif

/* Macro for conversion between X and Magic coordinates
 */

/* Current settings for X function parameters */
typedef struct {
    XFontStruct 	*font;
    Cursor		cursor;
    int			fontSize;
    int			depth;
    int			maskmod;
    Window 		window;
    MagicWindow		*mw;
} GR_CURRENT;

extern Display *grXdpy;
extern Colormap grXcmap;
extern int	grXscrn;
extern unsigned long grPixels[];
extern unsigned long grPlanes[];
extern GR_CURRENT grCurrent;
extern GC grGCFill, grGCText, grGCDraw, grGCCopy, grGCGlyph;

extern bool grx11GetCursorPos();
extern bool grx11DrawGrid();
extern Void GrX11EnableTablet();
extern Void GrX11DisableTablet();
extern Void GrX11SetCMap();
extern Void grx11PutText();
extern Void grx11DefineCursor();
extern Void GrX11SetCursor();
extern Void GrX11TextSize();
extern Void GrX11DrawGlyph();
extern Void GrX11BitBlt();
extern Void NullBitBlt();
extern int  GrX11ReadPixel();
extern Void grx11DrawLine();
extern Void grx11SetLineStyle();
extern Void grx11SetCharSize();
extern Void grx11SetWMandC();
extern Void grx11FillRect();


