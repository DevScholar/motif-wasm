#!/usr/bin/env bash
# Temporary script to build wml natively for WMLTARGETS generation.
# Run from WSL: bash scripts/_build_wml.sh
set -euo pipefail

WMLDIR="$(cd "$(dirname "$0")/../ignored-area/motif-full/tools/wml" && pwd)"
BUILDDIR="$WMLDIR/_build"
mkdir -p "$BUILDDIR"

CFLAGS="-I$WMLDIR -I/tmp -I$WMLDIR/../../lib -I$WMLDIR/../../include -w"

echo "=== Building libwml ==="
gcc -c $CFLAGS -o "$BUILDDIR/wmlparse.o"  "$WMLDIR/wmlparse.c"
gcc -c $CFLAGS -o "$BUILDDIR/wmloutkey.o" "$WMLDIR/wmloutkey.c"
gcc -c $CFLAGS -o "$BUILDDIR/wmlouth.o"   "$WMLDIR/wmlouth.c"
gcc -c $CFLAGS -o "$BUILDDIR/wmloutmm.o"  "$WMLDIR/wmloutmm.c"
gcc -c $CFLAGS -o "$BUILDDIR/wmloutp1.o"  "$WMLDIR/wmloutp1.c"
gcc -c $CFLAGS -o "$BUILDDIR/wmlresolve.o" "$WMLDIR/wmlresolve.c"
gcc -c $CFLAGS -o "$BUILDDIR/wmlsynbld.o" "$WMLDIR/wmlsynbld.c"
gcc -c $CFLAGS -o "$BUILDDIR/wmlutils.o"  "$WMLDIR/wmlutils.c"
echo "libwml OK"

echo "=== Building wml ==="
gcc -c $CFLAGS -o "$BUILDDIR/wml.o" "$WMLDIR/wml.c"
gcc -o "$BUILDDIR/wml" "$BUILDDIR/wml.o" "$BUILDDIR/wmlparse.o" "$BUILDDIR/wmloutkey.o" "$BUILDDIR/wmlouth.o" "$BUILDDIR/wmloutmm.o" "$BUILDDIR/wmloutp1.o" "$BUILDDIR/wmlresolve.o" "$BUILDDIR/wmlsynbld.o" "$BUILDDIR/wmlutils.o" -w
echo "wml OK"

echo "=== Building wmluiltok ==="
gcc -I/tmp -w -o "$BUILDDIR/wmluiltok" "$WMLDIR/wmluiltok.c"
echo "wmluiltok OK"

echo "=== Generating WMLTARGETS ==="
cd "$WMLDIR"
# wmluiltok reads Uil.y from stdin, writes tokens.dat to stdout
"$BUILDDIR/wmluiltok" < "$WMLDIR/Uil.y" > "$BUILDDIR/tokens.dat"
# wml reads tokens.dat from CWD — must be in the wml run directory
cp "$BUILDDIR/tokens.dat" "$WMLDIR/tokens.dat"
# wml reads motif.wml and generates WMLTARGETS in current dir
"$BUILDDIR/wml" "$WMLDIR/motif.wml"
echo "WMLTARGETS generated"

# Copy generated headers to clients/uil include path
# Exclude Uil.h and UilLexPars.h — those are bison outputs for the parser,
# not WMLTARGETS. The public API Uil.h in clients/uil/ must be preserved.
echo "=== Copying WMLTARGETS ==="
for f in "$WMLDIR"/Uil*.h; do
    basename="$(basename "$f")"
    case "$basename" in
        Uil.h|UilLexPars.h) ;;
        *) cp "$f" "$WMLDIR/../../clients/uil/" ;;
    esac
done
echo "done"
