/* Stubs for Xlib functions that Motif uses but em-x11 doesn't yet implement.
 * Many of these have macro equivalents in em-x11's Xlib.h (e.g.
 * DefaultScreenOfDisplay vs XDefaultScreenOfDisplay); Motif calls the
 * function forms that real libX11 also exports as linkable symbols. */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <string.h>
#include <stdlib.h>

/* XftFontMatch compatibility wrapper.
 * em-x11's XftFontMatch returns XftFont* (combining match+open), but the
 * real Xft API returns FcPattern* so callers can inspect it before calling
 * XftFontOpenPattern. This wrapper delegates to fontconfig's FcFontMatch
 * (which real Xft uses internally) and returns the matched pattern. */
#include <fontconfig/fontconfig.h>
FcPattern *motif_XftFontMatch(Display *dpy, int screen, FcPattern *pattern,
                              FcResult *result) {
    (void)dpy; (void)screen;
    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    return FcFontMatch(NULL, pattern, result);
}

/* ---- Screen / Display info accessors ---- */

Screen *XDefaultScreenOfDisplay(Display *dpy) {
    return ScreenOfDisplay(dpy, DefaultScreen(dpy));
}

int XScreenCount(Display *dpy) {
    return ScreenCount(dpy);
}

char *XDisplayString(Display *dpy) {
    return DisplayString(dpy);
}

unsigned long XBlackPixelOfScreen(Screen *screen) {
    return BlackPixelOfScreen(screen);
}

int XWidthOfScreen(Screen *screen) {
    return WidthOfScreen(screen);
}

int XHeightOfScreen(Screen *screen) {
    return HeightOfScreen(screen);
}

unsigned long XLastKnownRequestProcessed(Display *dpy) {
    return LastKnownRequestProcessed(dpy);
}

int XSetCloseDownMode(Display *dpy, int close_mode) {
    (void)dpy;
    return close_mode;
}

/* ---- Color allocation ---- */

Status XAllocColorCells(Display *dpy, Colormap cmap, Bool contig,
                        unsigned long *plane_masks, unsigned int nplanes,
                        unsigned long *pixels, unsigned int npixels) {
    (void)dpy; (void)cmap; (void)contig;
    if (plane_masks && nplanes > 0) *plane_masks = 0;
    for (unsigned int i = 0; i < npixels && pixels; i++) {
        pixels[i] = (unsigned long)i;
    }
    return 1;
}

int XStoreColor(Display *dpy, Colormap cmap, XColor *color) {
    (void)dpy; (void)cmap;
    if (color) {
        color->pixel = (unsigned long)(
            (color->red & 0xFF00) << 8 |
            (color->green & 0xFF00) |
            (color->blue & 0xFF00) >> 8
        );
    }
    return 1;
}

Colormap *XListInstalledColormaps(Display *dpy, Window w, int *num_return) {
    (void)w;
    Colormap *list = (Colormap *)malloc(sizeof(Colormap));
    if (list) {
        list[0] = DefaultColormap(dpy, DefaultScreen(dpy));
        *num_return = 1;
    } else {
        *num_return = 0;
    }
    return list;
}

/* ---- Event / grab helpers ---- */

int XAllowEvents(Display *dpy, int event_mode, Time time) {
    (void)dpy; (void)event_mode; (void)time;
    return 1;
}

int XChangeActivePointerGrab(Display *dpy, unsigned int event_mask,
                             Cursor cursor, Time time) {
    (void)dpy; (void)event_mask; (void)cursor; (void)time;
    return 1;
}

/* Map event type to its mask bit. libX11's _XEventTypeToMask / EvToMas.c.
 * For most types (>= MotionNotify) the mask is (1L << type); the early
 * types overlap with error codes and are special-cased. */
static unsigned long event_type_to_mask(int type) {
    switch (type) {
        case KeyPress:         return KeyPressMask;          /* 2 → 1L<<0  */
        case KeyRelease:       return KeyReleaseMask;         /* 3 → 1L<<1  */
        case ButtonPress:      return ButtonPressMask;        /* 4 → 1L<<2  */
        case ButtonRelease:    return ButtonReleaseMask;      /* 5 → 1L<<3  */
        case EnterNotify:      return EnterWindowMask;        /* 7 → 1L<<4  */
        case LeaveNotify:      return LeaveWindowMask;        /* 8 → 1L<<5  */
        case Expose:           return ExposureMask;           /* 12→ 1L<<15 */
        case GraphicsExpose:   return (1L << 16);
        case NoExpose:         return (1L << 17);
        case VisibilityNotify: return VisibilityChangeMask;   /* 15→ 1L<<18 */
        case CreateNotify:     return SubstructureNotifyMask; /* 16→ 1L<<19 */
        case DestroyNotify:    return SubstructureNotifyMask; /* 17→ 1L<<19 */
        case UnmapNotify:      return StructureNotifyMask | SubstructureNotifyMask;
        case MapNotify:        return StructureNotifyMask | SubstructureNotifyMask;
        case MapRequest:       return SubstructureRedirectMask;
        case ReparentNotify:   return StructureNotifyMask | SubstructureNotifyMask;
        case ConfigureNotify:  return StructureNotifyMask;    /* 22→ 1L<<20 */
        case GravityNotify:    return StructureNotifyMask;
        case CirculateNotify:  return StructureNotifyMask;
        case PropertyNotify:   return PropertyChangeMask;     /* 28→ 1L<<22 */
        case SelectionClear:   return (1L << 23);
        case SelectionRequest: return (1L << 23);
        case SelectionNotify:  return (1L << 23);
        case ColormapNotify:   return ColormapChangeMask;     /* 32→ 1L<<24 */
        default:
            if (type >= MotionNotify)
                return (1UL << type);
            return 0;
    }
}

int XWindowEvent(Display *dpy, Window w, long event_mask, XEvent *event_return) {
    while (1) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        if (ev.xany.window == w && (event_type_to_mask(ev.type) & event_mask)) {
            if (event_return) *event_return = ev;
            return 0;
        }
    }
}

/* ---- Cut buffers (obsolete X10 API) ---- */

char *XFetchBuffer(Display *dpy, int *nbytes_return, int buffer) {
    (void)dpy; (void)buffer;
    if (nbytes_return) *nbytes_return = 0;
    return NULL;
}

int XRotateBuffers(Display *dpy, int rotate) {
    (void)dpy; (void)rotate;
    return 1;
}

int XStoreBuffer(Display *dpy, const char *bytes, int nbytes, int buffer) {
    (void)dpy; (void)bytes; (void)nbytes; (void)buffer;
    return 1;
}

/* ---- Cursor ---- */

Status XQueryBestCursor(Display *dpy, Drawable d, unsigned int width,
                        unsigned int height, unsigned int *width_return,
                        unsigned int *height_return) {
    (void)dpy; (void)d;
    if (width_return)  *width_return  = width  > 32 ? 32 : width;
    if (height_return) *height_return = height > 32 ? 32 : height;
    return 1;
}

/* ---- Text property ---- */

void XSetTextProperty(Display *dpy, Window w, XTextProperty *text_prop,
                      Atom property) {
    if (text_prop && text_prop->value && text_prop->nitems > 0) {
        XChangeProperty(dpy, w, property, text_prop->encoding, 8,
                        PropModeReplace,
                        (unsigned char *)text_prop->value,
                        text_prop->nitems);
    }
}

/* ---- 16-bit text extents ---- */

/* Convert UCS-2 big-endian (XChar2b) to UTF-8, including surrogate pairs. */
static int ucs2_to_utf8(const XChar2b *string, int nchars,
                        unsigned char *out, int cap) {
    int w = 0, i = 0;
    while (i < nchars && w + 4 <= cap) {
        unsigned int cp = ((unsigned int)string[i].byte1 << 8) | string[i].byte2;
        i++;
        if (cp >= 0xD800 && cp <= 0xDBFF && i < nchars) {
            unsigned int lo = ((unsigned int)string[i].byte1 << 8) | string[i].byte2;
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                i++;
            }
        }
        if (cp < 0x80)
            out[w++] = (unsigned char)cp;
        else if (cp < 0x800) {
            out[w++] = (unsigned char)(0xC0 | (cp >> 6));
            out[w++] = (unsigned char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out[w++] = (unsigned char)(0xE0 | (cp >> 12));
            out[w++] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
            out[w++] = (unsigned char)(0x80 | (cp & 0x3F));
        } else {
            out[w++] = (unsigned char)(0xF0 | (cp >> 18));
            out[w++] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
            out[w++] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
            out[w++] = (unsigned char)(0x80 | (cp & 0x3F));
        }
    }
    return w;
}

int XTextExtents16(XFontStruct *font, const XChar2b *string, int nchars,
                   int *direction_return, int *font_ascent_return,
                   int *font_descent_return, XCharStruct *overall_return) {
    if (!string || nchars <= 0) return 0;
    int cap = nchars * 4 + 1;
    unsigned char stack[512];
    unsigned char *buf = (cap <= (int)sizeof(stack)) ? stack : (unsigned char *)malloc(cap);
    if (!buf) return 0;
    int used = ucs2_to_utf8(string, nchars, buf, cap - 1);
    buf[used] = '\0';
    int ret = XTextExtents(font, (const char *)buf, used, direction_return,
                           font_ascent_return, font_descent_return,
                           overall_return);
    if (buf != stack) free(buf);
    return ret;
}

/* ---- Input method ---- */

char *XmbResetIC(XIC ic) {
    (void)ic;
    return NULL;
}

/* ---- XInitImage (used by Motif Png.c / Svg.c) ---- */

Status XInitImage(XImage *image) {
    (void)image;
    return 1;
}

/* ---- Screen accessor stubs ---- */

unsigned long XWhitePixelOfScreen(Screen *screen) {
    return WhitePixelOfScreen(screen);
}

/* ---- Xutf8TextListToTextProperty ---- */

int Xutf8TextListToTextProperty(Display *dpy, char **list, int count,
                                XICCEncodingStyle style, XTextProperty *text_prop) {
    (void)dpy; (void)count; (void)style;
    if (list && list[0] && text_prop) {
        text_prop->value = (unsigned char *)strdup(list[0]);
        text_prop->nitems = strlen(list[0]);
        text_prop->encoding = XA_STRING;
        text_prop->format = 8;
        return 0;
    }
    return -1;
}

/* ---- XftDrawString32 — em-x11 doesn't implement this yet ---- */

void XftDrawString32(XftDraw *draw, _Xconst XftColor *color, XftFont *font,
                     int x, int y, const FcChar32 *string, int len) {
    (void)draw; (void)color; (void)font; (void)x; (void)y;
    (void)string; (void)len;
}
