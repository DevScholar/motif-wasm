#!/usr/bin/env bash
#
# build.sh -- anti-stale build for motif-wasm
#
# Detects staleness of cmake cache and source files, reconfigures when
# needed, then builds. Designed for pnpm build:motif and build:motif:debug.
#
# Usage:
#   bash scripts/build.sh [Release|Debug]

set -euo pipefail

if [ "$(uname -s)" != "Linux" ]; then
  echo "ERROR: This project requires Linux. Run from WSL, not Git Bash or Windows."
  exit 1
fi

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
BUILD_TYPE="${1:-Release}"

log()  { printf '    %s\n' "$*"; }
warn() { printf '    WARNING: %s\n' "$*"; }
die()  { printf 'build: %s\n' "$*" >&2; exit 1; }

# --- Step 1: fetch Motif source (idempotent) ---
log "fetching Motif source..."
bash "$REPO_ROOT/scripts/fetch-motif.sh"

# --- Step 2: detect staleness ---

STALE=0
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"

# Collect all CMakeLists.txt in the project
CMAKE_FILES=()
while IFS= read -r -d '' f; do
  CMAKE_FILES+=("$f")
done < <(find "$REPO_ROOT" -maxdepth 3 -name CMakeLists.txt -print0 2>/dev/null)

if [ ! -f "$CACHE_FILE" ]; then
  log "no CMakeCache.txt — fresh configure required"
  STALE=1
else
  # Check if any CMakeLists.txt is newer than the cache
  for f in "${CMAKE_FILES[@]}"; do
    if [ "$f" -nt "$CACHE_FILE" ]; then
      log "$(realpath --relative-to="$REPO_ROOT" "$f") is newer than CMakeCache.txt"
      STALE=1
    fi
  done

  # Check if the em-x11 cmake module is newer than the cache
  EM_X11_SRC="${EM_X11_SRC:-$REPO_ROOT/../em-x11}"
  EM_X11_DEMO="$EM_X11_SRC/cmake/em_x11_demo.cmake"
  if [ -f "$EM_X11_DEMO" ] && [ "$EM_X11_DEMO" -nt "$CACHE_FILE" ]; then
    log "em_x11_demo.cmake is newer than CMakeCache.txt"
    STALE=1
  fi

  # Check for stale file(GLOB) results — if any .c file in the source tree
  # is newer than the cache, cmake might need to re-glob.
  STAGED_MOTIF="$REPO_ROOT/ignored-area/third-party/motif"
  if [ -d "$STAGED_MOTIF" ]; then
    NEWEST_C=$(find "$STAGED_MOTIF/src/lib" -name '*.c' -newer "$CACHE_FILE" 2>/dev/null | head -1)
    if [ -n "$NEWEST_C" ]; then
      log "source file $(realpath --relative-to="$REPO_ROOT" "$NEWEST_C") is newer than CMakeCache.txt"
      STALE=1
    fi
  fi
fi

# --- Step 3: build em-x11 default-host IIFE (vite) ---
# Motif-wasm links this as --pre-js.  If missing, the Host never
# auto-creates and the demo stays blank (no canvas, no windows).

EM_X11_ABS="$(cd "${EM_X11_SRC:-$REPO_ROOT/../em-x11}" && pwd)"

log "building em-x11-default-host.js..."
( cd "$EM_X11_ABS" && npx vite build -c vite.host.config.ts )

# --- Step 4: configure (if stale) ---

if [ "$STALE" -eq 1 ]; then
  log "reconfiguring cmake ($BUILD_TYPE)..."
  rm -rf "$BUILD_DIR"
  if [ "$BUILD_TYPE" = "Debug" ]; then
    emcmake cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DEM_X11_SRC="$EM_X11_ABS"
  else
    emcmake cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=Release \
      -DEM_X11_SRC="$EM_X11_ABS"
  fi
else
  log "cmake cache is up-to-date"
fi

# --- Step 5: build ---

log "building..."
EM_X11_SRC="$EM_X11_ABS" cmake --build "$BUILD_DIR" -j$(nproc)

log "build complete"
