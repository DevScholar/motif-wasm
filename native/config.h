/* config.h — generated for emscripten/motif-wasm build.
 * Motif sources include <config.h> guarded by #ifdef HAVE_CONFIG_H.
 * This file lives in our project; the upstream motif tree is untouched. */

/* Standard C headers — all present in emscripten */
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_WCTYPE_H 1

/* Additional X11 headers */
#define HAVE_X11_XPOLL_H 1

/* libc functions available in emscripten */
#define HAVE_GETCWD 1
#define HAVE_PUTENV 1
#define HAVE_STRCASECMP 1
#define HAVE_STRDUP 1
#define HAVE_NANOSLEEP 1
#define HAVE_REGCOMP 1

/* Type sizes (wasm32) */
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_SHORT 2
#define SIZEOF_VOID_P 4

/* Motif feature flags */
#define XM_MSGCAT 0
#define XM_UTF8 1
#define USE_XFT 1

/* Compatibility */
#define STDC_HEADERS 1

/* Xft type aliases — em-x11 uses fontconfig FcChar* names */
#include <fontconfig/fontconfig.h>
typedef FcChar8  XftChar8;
typedef FcChar16 XftChar16;
typedef FcChar32 XftChar32;
