#!/bin/bash
# build.sh [TC]: builds DeskLib64, the 64-bit DeskLib for RISC OS 64 (AArch64), from src/ (DeskLib with the changes in MODIFICATIONS64.md), gen/ (its ARM assembler veneers
# translated to C by tools/armv2c.py) and hand/ (the veneers written by hand), into dist/lib/libDeskLib64.a and dist/include/.
#   TC     the aarch64-riscos tool chain folder (the one with bin/aarch64-riscos-gcc), 0.14 or newer; or set TC in the environment
#   REGEN=1  translate the assembler again first (python3), instead of using gen/
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TC=${1:-${TC:?give the tool chain folder as the first argument or in TC}}
CC=$TC/bin/aarch64-riscos-gcc; AR=$TC/bin/aarch64-riscos-ar
[ -x "$CC" ] || { echo "no compiler at $CC"; exit 1; }
W=$HERE/work; D=$HERE/dist
rm -rf "$W" "$D"; mkdir -p "$W/shim" "$W/obj" "$D/lib" "$D/include"
[ -z "$REGEN" ] || { rm -rf "$HERE/gen"; python3 "$HERE/tools/armv2c.py" "$HERE/src" "$HERE/gen" "$HERE/hand"; rm -f "$HERE/gen/report.txt" "$HERE/gen/review.txt"; }
# DeskLib's sources include "DeskLib:Wimp.h" (a RISC OS path): one copy of each header under each spelling
for h in "$HERE"/src/include/*.h "$HERE"/src/oldinclude/*.h; do
  b=$(basename "$h")
  for p in DeskLib Desklib desklib; do [ -e "$W/shim/$p:$b" ] || cp "$h" "$W/shim/$p:$b"; done
done
FLAGS="-munixlib -O2 -fsigned-char -w -I $W/shim -I $HERE/src/include -I $HERE/src/oldinclude -I $HERE/src/Libraries"
SKIP='BackTrace|/PCI/|/USB/|Libraries/Mem/'
ok=0; bad=0; : > "$HERE/build.log"
for f in $(cd "$HERE" && find src/Libraries gen hand -name '*.c' | sort); do
  echo "$f" | grep -q -E "$SKIP" && continue
  n=$(echo "$f" | sed 's|/|_|g; s|\.c$||')
  if $CC $FLAGS -I "$(dirname "$HERE/$f")" -c "$HERE/$f" -o "$W/obj/$n.o" 2>>"$HERE/build.log"; then ok=$((ok+1)); else bad=$((bad+1)); echo "FAILED $f" >> "$HERE/build.log"; fi
done
$AR rcs "$D/lib/libDeskLib64.a" "$W"/obj/*.o
cp "$W"/shim/DeskLib:*.h "$D/include/"
echo "DeskLib64: $ok objects, $bad failed (see build.log); dist/lib/libDeskLib64.a, dist/include/"
[ "$bad" = 0 ]
