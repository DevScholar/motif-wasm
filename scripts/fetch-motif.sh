#!/usr/bin/env bash
#
# fetch-motif.sh -- download and stage Motif 2.5.2 source for the wasm build.
#
# Downloads https://github.com/thentenaar/motif/archive/refs/tags/v2.5.2.tar.gz
# into ignored-area/motif-full/ (Motif source is never forked, modified, or
# committed here).
# Then copies the relevant source files into ignored-area/third-party/motif/
# which is what CMakeLists.txt actually compiles.
#
# Prerequisites:
#   - curl, tar, gcc (for makestrs), node (for wasm uil at runtime)
#
# Idempotent: skips download if the target source already exists;
#             skips staging if .fetched sentinel exists.

set -euo pipefail

if [ "$(uname -s)" != "Linux" ]; then
  echo "ERROR: This project requires Linux. Run from WSL, not Git Bash or Windows."
  exit 1
fi

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MOTIF_SRC="${MOTIF_SRC:-$REPO_ROOT/ignored-area/motif-full}"
MOTIF_TARBALL="https://github.com/thentenaar/motif/archive/refs/tags/v2.5.2.tar.gz"
MOTIF_VERSION="2.5.2"

log()  { printf '    %s\n' "$*"; }
warn() { printf '    WARNING: %s\n' "$*"; }
die()  { printf 'fetch-motif: %s\n' "$*" >&2; exit 1; }

# --- Download motif tarball (if not already present) ---

MOTIF_SENTINEL="$MOTIF_SRC/.fetched"

if [ -f "$MOTIF_SENTINEL" ]; then
  log "motif ${MOTIF_VERSION} already downloaded at $MOTIF_SRC"
else
  if ! command -v curl >/dev/null 2>&1; then
    die "curl not found"
  fi
  log "downloading motif ${MOTIF_VERSION} -> $MOTIF_SRC"
  rm -rf "$MOTIF_SRC"
  mkdir -p "$MOTIF_SRC"

  curl -L --silent --show-error "$MOTIF_TARBALL" \
    | tar -xz --strip-components=1 -C "$MOTIF_SRC" 2>&1 | sed 's/^/      /'

  touch "$MOTIF_SENTINEL"
  log "motif ${MOTIF_VERSION} downloaded"
fi

# --- Stage motif source into our ignored-area ---
# CMakeLists.txt compiles from this tree (never the upstream clone directly).
# We mirror lib/Xm, lib/Mrm, include/, and config/makestrs.
# Idempotent: skips copy if .fetched sentinel exists.

STAGED_MOTIF="$REPO_ROOT/ignored-area/third-party/motif"
STAGED_SENTINEL="$STAGED_MOTIF/.fetched"

if [ -f "$STAGED_SENTINEL" ]; then
  log "motif source already staged at $STAGED_MOTIF"
else
  log "staging motif source into ignored-area/third-party/motif/"
  rm -rf "$STAGED_MOTIF"
  mkdir -p "$STAGED_MOTIF/lib"
  mkdir -p "$STAGED_MOTIF/config"
  mkdir -p "$STAGED_MOTIF/include"
  mkdir -p "$STAGED_MOTIF/demos/programs"

  cp -a "$MOTIF_SRC/lib/Xm"   "$STAGED_MOTIF/lib/"
  cp -a "$MOTIF_SRC/lib/Mrm"  "$STAGED_MOTIF/lib/"
  cp -a "$MOTIF_SRC/include/" "$STAGED_MOTIF/include/"
  cp -a "$MOTIF_SRC/config/makestrs.c" "$STAGED_MOTIF/config/"

  if [ -d "$MOTIF_SRC/demos/programs/periodic" ]; then
    cp -a "$MOTIF_SRC/demos/programs/periodic" \
          "$STAGED_MOTIF/demos/programs/"
  fi

  # Fix _XmCreateImage macro in XmI.h: the upstream version uses bare
  # IMAGE->member which breaks when IMAGE is *ptr (operator precedence).
  # Parenthesize (IMAGE) and wrap in do-while, matching the XmP.h style.
  log "patching XmI.h _XmCreateImage macro"
  python3 -c "
import re
path = '$STAGED_MOTIF/lib/Xm/XmI.h'
with open(path) as f:
    content = f.read()
old = '''#define _XmCreateImage(IMAGE, DISPLAY, DATA, WIDTH, HEIGHT, BYTE_ORDER) {\\\\
    IMAGE = XCreateImage(DISPLAY,\\\\
			 DefaultVisual(DISPLAY, DefaultScreen(DISPLAY)),\\\\
			 1,\\\\
			 XYBitmap,\\\\
			 0,\\\\
			 DATA,\\\\
			 WIDTH, HEIGHT,\\\\
			 8,\\\\
			 (WIDTH+7) >> 3);\\\\
    IMAGE->byte_order = BYTE_ORDER;\\\\
    IMAGE->bitmap_unit = 8;\\\\
    IMAGE->bitmap_bit_order = LSBFirst;\\\\
}'''
new = '''#define _XmCreateImage(IMAGE, DISPLAY, DATA, WIDTH, HEIGHT, BYTE_ORDER) \\\\
do { \\\\
    (IMAGE) = XCreateImage(DISPLAY,\\\\
			 DefaultVisual(DISPLAY, DefaultScreen(DISPLAY)),\\\\
			 1, XYBitmap, 0, (DATA), (WIDTH), (HEIGHT),\\\\
			 8, (((WIDTH)+7) >> 3));\\\\
    (IMAGE)->byte_order = (BYTE_ORDER);\\\\
    (IMAGE)->bitmap_unit = 8;\\\\
    (IMAGE)->bitmap_bit_order = LSBFirst;\\\\
} while (0)'''
if old in content:
    content = content.replace(old, new)
    with open(path, 'w') as f:
        f.write(content)
    print('    patched')
else:
    print('    already patched (or macro not found)')
" 2>&1 | sed 's/^/      /'
  touch "$STAGED_SENTINEL"
  log "motif source staged"
fi

# --- Compile periodic UIL -> UID (output stays in our project) ---
# Uses our own wasm uil (built via cmake) instead of the system (64-bit) uil,
# so that sizeof(long)=4 and the UID file is byte-compatible with the
# wasm MRM reader.

PERIODIC_SRC_DIR="$STAGED_MOTIF/demos/programs/periodic"
PERIODIC_OUT_DIR="$REPO_ROOT/examples/twm-periodic"
WASM_UIL="$REPO_ROOT/build/artifacts/uil.cjs"

if [ -d "$PERIODIC_SRC_DIR" ]; then
  log "compiling periodic UIL -> UID (via wasm uil)"
  if [ ! -f "$WASM_UIL" ]; then
    warn "wasm uil not found at $WASM_UIL -- skipping UID generation (CMakeLists.txt will build uil and generate periodic.uid)"
  else
    if ! command -v node >/dev/null 2>&1; then
      die "node not found. Node.js is required to run the wasm uil compiler."
    fi
    mkdir -p "$PERIODIC_OUT_DIR"

    node "$WASM_UIL" \
      -o "$PERIODIC_OUT_DIR/periodic.uid" \
      "$PERIODIC_SRC_DIR/periodic.uil" \
      "-I$PERIODIC_SRC_DIR" \
      "-I$STAGED_MOTIF/lib/Xm" 2>&1 || warn "UIL compilation failed"
  fi
else
  warn "periodic demo not found at $PERIODIC_SRC_DIR"
fi

# --- Stage X11 bitmaps from em-x11 ---
#
# Motif IconButton.c / I18List.c include <X11/bitmaps/gray> etc.
# These live in em-x11's third-party area; symlink into our tree.

log "staging X11 bitmaps"
BITMAP_DST="$REPO_ROOT/ignored-area/include/X11/bitmaps"
mkdir -p "$BITMAP_DST"
EM_X11_ROOT="${EM_X11_SRC:-$REPO_ROOT/../em-x11}"
BITMAP_SRC="$EM_X11_ROOT/ignored-area/third-party/xbitmaps"
if [ -d "$BITMAP_SRC" ]; then
  for bm in gray stipple gray1 gray3 light_gray dimple1 dimple3 dot \
            cross_weave root_weave wide_weave flipped_gray; do
    [ -f "$BITMAP_SRC/$bm" ] && ln -sf "$BITMAP_SRC/$bm" "$BITMAP_DST/$bm"
  done
  log "X11 bitmaps staged"
else
  warn "xbitmaps not found at $BITMAP_SRC"
fi

# --- Compile makestrs and generate XmStrDefs files ---
# thentenaar/motif ships resource string definitions as .ht/.ct templates
# that must be processed by makestrs. We compile makestrs natively and
# generate the real .h/.c files into our own source tree (never modifying
# the upstream motif repo).

MAKESTRS_SRC="$STAGED_MOTIF/config/makestrs.c"
STRDEFS_DIR="$STAGED_MOTIF/lib/Xm"
GEN_DIR="$REPO_ROOT/native/generated/Xm"

if [ -f "$MAKESTRS_SRC" ]; then
  log "compiling makestrs (native)"
  MAKESTRS_BIN="$REPO_ROOT/build/makestrs"
  mkdir -p "$REPO_ROOT/build"
  gcc -o "$MAKESTRS_BIN" "$MAKESTRS_SRC" 2>&1 || warn "makestrs compilation failed"

  if [ -x "$MAKESTRS_BIN" ]; then
    log "generating XmStrDefs files"
    rm -rf "$GEN_DIR"
    mkdir -p "$GEN_DIR"

    cp "$STRDEFS_DIR/XmStrDefs.ct"   "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefs.ht"   "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefs22.ht" "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefs23.ht" "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefs25.ht" "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefsI.ht"  "$GEN_DIR/"
    cp "$STRDEFS_DIR/xmstring.list.in" "$GEN_DIR/xmstring.list"

    (cd "$GEN_DIR" && "$MAKESTRS_BIN" -f xmstring.list > XmStrDefs.c 2>/dev/null) || true

    for _stem in XmStrDefs XmStrDefs22 XmStrDefs23 XmStrDefs25 XmStrDefsI; do
      if [ ! -f "$GEN_DIR/${_stem}.h" ] || [ ! -s "$GEN_DIR/${_stem}.h" ]; then
        warn "${_stem}.h not generated by makestrs -- using fallback"
        sed "s/<<<STRING_TABLE_GOES_HERE>>>/extern char *XmStrings[];/" \
          "$GEN_DIR/${_stem}.ht" > "$GEN_DIR/${_stem}.h"
      fi
    done

    log "XmStrDefs generated in native/generated/Xm/"
  fi
else
  warn "makestrs source not found at $MAKESTRS_SRC"
fi

log "done"
