#!/usr/bin/env bash
#
# fetch-motif.sh -- ensure the Motif source tree is available as a sibling
#                   directory, stage it into our ignored-area, and compile
#                   UIL assets we need at build time.
#
# Clones https://github.com/dimmus/motif.git into ignored-area/motif-full/
# (Motif source is never forked, modified, or committed here).
# Then copies the relevant source files into ignored-area/third-party/motif/
# which is what CMakeLists.txt actually compiles.
#
# Prerequisites:
#   - uil (Motif UIL compiler): sudo apt install uil
#
# Idempotent: skips clone if the target .git already exists;
#             skips staging if .fetched sentinel exists.

set -euo pipefail

if [ "$(uname -s)" != "Linux" ]; then
  echo "ERROR: This project requires Linux. Run from WSL, not Git Bash or Windows."
  exit 1
fi

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MOTIF_SRC="${MOTIF_SRC:-$REPO_ROOT/ignored-area/motif-full}"
MOTIF_REPO="https://github.com/dimmus/motif.git"

log()  { printf '    %s\n' "$*"; }
warn() { printf '    WARNING: %s\n' "$*"; }
die()  { printf 'fetch-motif: %s\n' "$*" >&2; exit 1; }

# --- Clone motif (if not already present) ---

if [ -d "$MOTIF_SRC/.git" ]; then
  log "motif already cloned at $MOTIF_SRC"
else
  if ! command -v git >/dev/null 2>&1; then
    die "git not found"
  fi
  log "cloning $MOTIF_REPO -> $MOTIF_SRC"
  git clone --depth 1 "$MOTIF_REPO" "$MOTIF_SRC" 2>&1 | sed 's/^/      /'
fi

# --- Stage motif source into our ignored-area ---
# CMakeLists.txt compiles from this tree (never the upstream clone directly).
# We mirror src/lib/Xm, src/lib/Mrm, include/, and bin/utils/makestrs.
# Idempotent: skips copy if .fetched sentinel exists.

STAGED_MOTIF="$REPO_ROOT/ignored-area/third-party/motif"
STAGED_SENTINEL="$STAGED_MOTIF/.fetched"

if [ -f "$STAGED_SENTINEL" ]; then
  log "motif source already staged at $STAGED_MOTIF"
else
  log "staging motif source into ignored-area/third-party/motif/"
  rm -rf "$STAGED_MOTIF"
  mkdir -p "$STAGED_MOTIF/src/lib"
  mkdir -p "$STAGED_MOTIF/src/bin/utils"
  mkdir -p "$STAGED_MOTIF/include"
  mkdir -p "$STAGED_MOTIF/src/examples/programs"

  # No trailing slash on source — cp -a creates the leaf dir correctly
  cp -a "$MOTIF_SRC/src/lib/Xm"   "$STAGED_MOTIF/src/lib/"
  cp -a "$MOTIF_SRC/src/lib/Mrm"  "$STAGED_MOTIF/src/lib/"
  cp -a "$MOTIF_SRC/include/"     "$STAGED_MOTIF/include/"
  cp -a "$MOTIF_SRC/src/bin/utils/makestrs.c" "$STAGED_MOTIF/src/bin/utils/"

  if [ -d "$MOTIF_SRC/src/examples/programs/periodic" ]; then
    cp -a "$MOTIF_SRC/src/examples/programs/periodic" \
          "$STAGED_MOTIF/src/examples/programs/"
  fi

  touch "$STAGED_SENTINEL"
  log "motif source staged"
fi

# --- Compile periodic UIL -> UID (output stays in our project) ---

PERIODIC_SRC_DIR="$STAGED_MOTIF/src/examples/programs/periodic"
PERIODIC_OUT_DIR="$REPO_ROOT/examples/periodic"

if [ -d "$PERIODIC_SRC_DIR" ]; then
  log "compiling periodic UIL -> UID"
  if ! command -v uil >/dev/null 2>&1; then
    die "uil (Motif UIL compiler) not found. Install it: sudo apt install uil"
  fi
  mkdir -p "$PERIODIC_OUT_DIR"

  # The system uil (2.3.8) fails when periodic.uil includes periodic_l.uil
  # (two module declarations in one compilation). Merge them into a single
  # module: take the main file, inline the value section from the l10n file,
  # and append everything after the include line.
  MERGED_UIL="$PERIODIC_OUT_DIR/.periodic_merged.uil"
  cat > "$MERGED_UIL" << 'UILEOF'
module periodic
    version = 'v2.0'
    names = case_sensitive
    objects = {
	XmLabel = widget;
	XmPushButton = widget;
	XmToggleButton = widget;
	XmCascadeButton = widget;
	XmSeparator = widget;
    }
UILEOF
  sed -n '/^value$/,$ p' "$PERIODIC_SRC_DIR/periodic_l.uil" | head -n -1 >> "$MERGED_UIL"
  awk 'NR>50' "$PERIODIC_SRC_DIR/periodic.uil" >> "$MERGED_UIL"

  uil -o "$PERIODIC_OUT_DIR/periodic.uid" "$MERGED_UIL" \
    -I"$STAGED_MOTIF/src/lib/Xm" 2>&1 || warn "UIL compilation failed"
  rm -f "$MERGED_UIL"
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
# dimmus/motif ships resource string definitions as .ht/.ct templates
# that must be processed by makestrs. We compile makestrs natively and
# generate the real .h/.c files into our own source tree (never modifying
# the upstream motif repo).

MAKESTRS_SRC="$STAGED_MOTIF/src/bin/utils/makestrs.c"
STRDEFS_DIR="$STAGED_MOTIF/src/lib/Xm"
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

    # Copy template files to gen dir
    cp "$STRDEFS_DIR/XmStrDefs.ct"   "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefs.ht"   "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefs22.ht" "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefs23.ht" "$GEN_DIR/"
    cp "$STRDEFS_DIR/XmStrDefsI.ht"  "$GEN_DIR/"
    cp "$STRDEFS_DIR/xmstring.list.in" "$GEN_DIR/xmstring.list"

    # Run makestrs to generate .c and .h files
    (cd "$GEN_DIR" && "$MAKESTRS_BIN" -f xmstring.list > XmStrDefs.c 2>/dev/null) || true

    # makestrs should generate headers; if not, create minimal fallbacks
    for _stem in XmStrDefs XmStrDefs22 XmStrDefs23 XmStrDefsI; do
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
