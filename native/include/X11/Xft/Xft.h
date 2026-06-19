/* Shadow header: includes em-x11's real Xft.h but replaces the
 * XftFontMatch declaration with the correct API signature.
 * em-x11's XftFontMatch returns XftFont* (combining match+open),
 * but the real Xft API returns FcPattern*.
 *
 * em-x11's xft.c.o exports XftFontMatch with the XftFont* return type.
 * To avoid a duplicate symbol, we:
 * 1. Rename em-x11's declaration  → emx11_XftFontMatch (never called)
 * 2. Declare our wrapper           → motif_XftFontMatch (returns FcPattern*)
 * 3. Redirect all remaining calls  → motif_XftFontMatch via macro
 */

/* Step 1: rename em-x11's declaration */
#define XftFontMatch emx11_XftFontMatch
#include_next <X11/Xft/Xft.h>
#undef XftFontMatch

/* Step 2: declare our wrapper (defined in native/motif_xlib_stubs.c) */
FcPattern *motif_XftFontMatch(Display *dpy, int screen, FcPattern *pattern,
                              FcResult *result);

/* Step 3: redirect Motif call sites to our wrapper */
#define XftFontMatch motif_XftFontMatch
