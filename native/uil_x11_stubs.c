/* X11 / Xft stubs for uil wasm build.
 * uil is a command-line compiler that never opens a display.
 * These satisfy the linker so we can link against libXm.
 * Minimal includes — no Xft header since it may not have all types. */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

int XAllowEvents(Display *dpy, int event_mode, Time time) { return 0; }
int XChangeActivePointerGrab(Display *dpy, unsigned int event_mask, Cursor cursor, Time time) { return 0; }
int XQueryBestCursor(Display *dpy, Drawable d, unsigned int width, unsigned int height, unsigned int *w, unsigned int *h) { return 0; }
Status XInitImage(XImage *image) { return 0; }
void XSetTextProperty(Display *dpy, Window w, XTextProperty *tp, Atom property) {}
void Xutf8SetWMProperties(Display *dpy, Window w, const char *name, const char *icon_name, char **argv, int argc, XSizeHints *hints, XWMHints *wm_hints, XClassHint *class_hints) {}

/* XTextExtents16 — em-x11 signature uses int* not XRectangle* */
int XTextExtents16(XFontStruct *fs, const XChar2b *str, int nchars, int *dir, int *ascent, int *descent, XCharStruct *overall) { return 0; }

/* Xft stubs — forward declare types from fontconfig/Xft */
typedef unsigned int FcChar32;
typedef struct _XftFont XftFont;
typedef struct _XftDraw XftDraw;
typedef struct _XftColor { unsigned long pixel; unsigned short color[4]; } XftColor;
typedef struct _XftPattern XftPattern;

Bool motif_XftFontMatch(Display *dpy, int screen, const char *pattern, XftFont **font, XftPattern **pat, void *result) { return 0; }
void XftDrawString32(XftDraw *d, const XftColor *color, XftFont *font, int x, int y, const FcChar32 *string, int len) {}
