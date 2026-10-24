#!/bin/bash
# ports/build-examples.sh [TC] [EXAMPLES]: builds the example applications of upstream DeskLib (https://github.com/riscos-dot-info/desklib, folder Examples) for RISC OS 64, without changing their source, into dist/examples/.
# TC = the tool chain folder; EXAMPLES = a checkout of that folder (default: a shallow clone into work/desklib-examples).  Run ../build.sh first.
# The sources are DeskLib's own and are not copied into this repository: only this script and the notes are.  Changes needed (none to the C source):
#   * the compiler is told to be an old C: -std=gnu89 (the examples use implicit declarations and a pointer where an int is wanted, which a current GCC refuses as errors in its default language level)
#   * the include names that differ in case from the DeskLib headers (RISC OS file names are case insensitive) get a link in a temporary include folder
#   * ports/patches/NAME.patch is applied to a copy of that example first (only FontDemo has one: it dereferences a NULL pointer, which RISC OS 32 allows)
#   * the !Run files lose their WimpSlot line (a 64-bit program needs more memory than the 64 KB to 320 KB that they ask for)
set -e
HERE=$(cd "$(dirname "$0")" && pwd); R=$HERE/..
TC=${1:-${TC:?give the tool chain folder as the first argument or in TC}}
EX=${2:-$R/work/desklib-examples}
CC=$TC/bin/aarch64-riscos-gcc
LIB=$R/dist/lib/libDeskLib64.a; INC=$R/dist/include
[ -d "$EX" ] || { mkdir -p "$R/work"; git clone -q --depth 1 https://github.com/riscos-dot-info/desklib "$R/work/desklib-src"; EX=$R/work/desklib-src/Examples; }
OUT=$R/dist/examples; mkdir -p "$OUT"
ALIAS=$(mktemp -d)
FLAGS="-std=gnu89 -munixlib -O2 -fsigned-char -fpermissive -w -I $INC -I $ALIAS"
for d in "$EX"/\!*; do
  n=$(basename "$d"); app=$OUT/$n; mkdir -p "$app"
  # links for the include names that differ only in case
  grep -hoiE '#include *"desklib:[^"]*"' "$d"/*.c "$d"/*.h 2>/dev/null | sed 's/#include *"//I; s/"$//' | sort -u | while read -r inc; do
    [ -e "$INC/$inc" ] && continue
    want=${inc#*:}
    real=$(ls "$INC" | grep -i -x -F "DeskLib:$want" | head -1)
    [ -n "$real" ] && ln -sf "$INC/$real" "$ALIAS/$inc"
  done
  src=$d
  if [ -f "$HERE/patches/${n#!}.patch" ]; then			# a patch that an example needs (see ports/README.md)
    src=$(mktemp -d)/$n; cp -r "$d" "$src"; (cd "$src" && patch -s -p0 < "$HERE/patches/${n#!}.patch")
  fi
  srcs=$(ls "$src"/*.c)
  if $CC $FLAGS -I "$src" $srcs "$LIB" -o "$app/!RunImage,ff8" 2> "$app/build.log"; then echo "built $n"; else echo "FAILED $n (see $app/build.log)"; continue; fi
  for f in "$d"/*; do
    case "$f" in *.c|*.h|*/Makefile*|*/Main.c) ;; *) cp -r "$f" "$app/";; esac
  done
  for f in "$app"/\!Run,feb; do [ -f "$f" ] && sed -i '/^[Ww][Ii][Mm][Pp][Ss][Ll][Oo][Tt]/d' "$f"; done
done
rm -rf "$ALIAS"
