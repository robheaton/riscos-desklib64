#!/bin/bash
# build.sh [TC]: builds DeskLib64, the 64-bit DeskLib for RISC OS 64 (AArch64), into dist/ (lib/libDeskLib64.a and include/).
#   TC       the aarch64-riscos tool chain folder (the one with bin/aarch64-riscos-gcc); or set TC in the environment
#   DESKLIB32  a checkout of https://github.com/adyoull/desklib32 at the commit below (default: cloned into work/)
# The DeskLib source is never copied into this repository: it is fetched unaltered and the patch in patches/ is applied to a copy in work/.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TC=${1:-${TC:?give the tool chain folder as the first argument or in TC}}
PIN=957863a4fdb4461d56b593fbd8dc3175209848b9
W=$HERE/work; D=$HERE/dist
CC=$TC/bin/aarch64-riscos-gcc; AR=$TC/bin/aarch64-riscos-ar
[ -x "$CC" ] || { echo "no compiler at $CC"; exit 1; }
if [ -n "$DESKLIB32" ]; then SRC=$DESKLIB32/src
else
  [ -d "$W/desklib32" ] || { mkdir -p "$W"; git clone -q https://github.com/adyoull/desklib32 "$W/desklib32"; }
  git -C "$W/desklib32" checkout -q "$PIN"
  SRC=$W/desklib32/src
fi
rm -rf "$W/src" "$W/gen" "$W/shim" "$W/obj" "$D"
mkdir -p "$W" "$W/shim" "$W/obj" "$D/lib" "$D/include"
cp -r "$SRC" "$W/src"
patch -s -p1 -d "$W/src" < "$HERE/patches/desklib64.patch"
# DeskLib's sources include "DeskLib:Wimp.h" (a RISC OS path): one copy of each header under each spelling
for h in "$W"/src/include/*.h "$W"/src/oldinclude/*.h; do
  b=$(basename "$h")
  for p in DeskLib Desklib desklib; do [ -e "$W/shim/$p:$b" ] || cp "$h" "$W/shim/$p:$b"; done
done
python3 "$HERE/tools/armv2c.py" "$W/src" "$W/gen" "$HERE/hand"
FLAGS="-munixlib -O2 -fsigned-char -w -I $W/shim -I $W/src/include -I $W/src/oldinclude -I $W/src/Libraries"
SKIP='BackTrace|/PCI/|/USB/|Libraries/Mem/'
ok=0; bad=0; : > "$HERE/build.log"
for f in $(cd "$W" && find src/Libraries gen -name '*.c'; cd "$HERE" && find hand -name '*.c' | sed "s|^|../|"); do
  case "$f" in ../*) full="$HERE/${f#../}";; *) full="$W/$f";; esac
  echo "$f" | grep -q -E "$SKIP" && continue
  n=$(echo "$f" | sed 's|^\.\./||; s|/|_|g; s|\.c$||')
  if $CC $FLAGS -I "$(dirname "$full")" -c "$full" -o "$W/obj/$n.o" 2>>"$HERE/build.log"; then ok=$((ok+1)); else bad=$((bad+1)); echo "FAILED $f" >> "$HERE/build.log"; fi
done
$AR rcs "$D/lib/libDeskLib64.a" "$W"/obj/*.o
cp "$W"/shim/DeskLib:*.h "$D/include/"
echo "DeskLib64: $ok objects, $bad failed (see build.log); dist/lib/libDeskLib64.a, dist/include/"
[ "$bad" = 0 ]
