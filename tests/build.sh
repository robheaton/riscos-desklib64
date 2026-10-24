#!/bin/bash
# tests/build.sh [TC]: builds the demonstration and test applications into dist/apps/ (run ../build.sh first).  Copy an application folder to RISC OS 64 and double-click it.
set -e
HERE=$(cd "$(dirname "$0")" && pwd); R=$HERE/..
TC=${1:-${TC:?give the tool chain folder as the first argument or in TC}}
CC=$TC/bin/aarch64-riscos-gcc
A=$R/dist/apps; mkdir -p "$A/!DLHello" "$A/!DLDemo2" "$A/!DLDialogs" "$A/!DLSave" "$A/!DLSprite"
FLAGS="-munixlib -O2 -fsigned-char -I $R/dist/include"
$CC $FLAGS "$HERE/hello/hello.c" "$R/dist/lib/libDeskLib64.a" -o "$A/!DLHello/!RunImage,ff8"
cp "$HERE/hello/!Run,feb" "$HERE/hello/!Boot,feb" "$A/!DLHello/"
$CC $FLAGS "$HERE/demo2/demo2.c" "$R/dist/lib/libDeskLib64.a" -o "$A/!DLDemo2/!RunImage,ff8"
cp "$HERE/demo2/!Run,feb" "$HERE/demo2/!Boot,feb" "$A/!DLDemo2/"
python3 "$HERE/demo2/mktemplate.py" "$A/!DLDemo2/Templates,fec"
for t in dialogs:DLDialogs save:DLSave sprite:DLSprite; do
  d=${t%%:*}; n=${t##*:}
  $CC $FLAGS "$HERE/$d/$d.c" "$R/dist/lib/libDeskLib64.a" -o "$A/!$n/!RunImage,ff8"
  cp "$HERE/$d/!Run,feb" "$HERE/$d/!Boot,feb" "$A/!$n/"
  python3 "$HERE/$d/mktemplate.py" "$A/!$n/Templates,fec"
done
$CC $FLAGS "$HERE/sprite/spritetest.c" "$R/dist/lib/libDeskLib64.a" -o "$A/spritetest,ff8"
echo "built $A/!DLHello, !DLDemo2, !DLDialogs, !DLSave, !DLSprite and spritetest,ff8"
