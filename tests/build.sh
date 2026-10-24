#!/bin/bash
# tests/build.sh [TC]: builds the demonstration and test applications into dist/apps/ (run ../build.sh first).  Copy an application folder to RISC OS 64 and double-click it.
set -e
HERE=$(cd "$(dirname "$0")" && pwd); R=$HERE/..
TC=${1:-${TC:?give the tool chain folder as the first argument or in TC}}
CC=$TC/bin/aarch64-riscos-gcc
A=$R/dist/apps; mkdir -p "$A/!DLHello" "$A/!DLDemo2" "$A/!DLDialogs" "$A/!DLSave" "$A/!DLSprite" "$A/!DLPane" "$A/!DLFont"
FLAGS="-munixlib -O2 -fsigned-char -I $R/dist/include"
$CC $FLAGS "$HERE/hello/hello.c" "$R/dist/lib/libDeskLib64.a" -o "$A/!DLHello/!RunImage,ff8"
cp "$HERE/hello/!Run,feb" "$HERE/hello/!Boot,feb" "$A/!DLHello/"
$CC $FLAGS "$HERE/demo2/demo2.c" "$R/dist/lib/libDeskLib64.a" -o "$A/!DLDemo2/!RunImage,ff8"
cp "$HERE/demo2/!Run,feb" "$HERE/demo2/!Boot,feb" "$A/!DLDemo2/"
python3 "$HERE/demo2/mktemplate.py" "$A/!DLDemo2/Templates,fec"
for t in dialogs:DLDialogs save:DLSave sprite:DLSprite pane:DLPane font:DLFont; do
  d=${t%%:*}; n=${t##*:}
  $CC $FLAGS "$HERE/$d/$d.c" "$R/dist/lib/libDeskLib64.a" -o "$A/!$n/!RunImage,ff8"
  cp "$HERE/$d/!Run,feb" "$HERE/$d/!Boot,feb" "$A/!$n/"
  python3 "$HERE/$d/mktemplate.py" "$A/!$n/Templates,fec"
done
$CC $FLAGS "$HERE/sprite/spritetest.c" "$R/dist/lib/libDeskLib64.a" -o "$A/spritetest,ff8"
$CC $FLAGS "$HERE/filing/filetest.c" "$R/dist/lib/libDeskLib64.a" -o "$A/filetest,ff8"
$CC $FLAGS "$HERE/colour/colourtest.c" "$R/dist/lib/libDeskLib64.a" -o "$A/colourtest,ff8"
mkdir -p "$A/msgtest"; $CC $FLAGS "$HERE/msgs/msgtest.c" "$R/dist/lib/libDeskLib64.a" -o "$A/msgtest/msgtest,ff8"; cp "$HERE/msgs/Messages" "$A/msgtest/Messages,fff"; cp "$HERE/msgs/Msgs" "$A/msgtest/Msgs,fff"
echo "built $A/!DLHello, !DLDemo2, !DLDialogs, !DLSave, !DLSprite, !DLPane, !DLFont, spritetest,ff8, filetest,ff8, colourtest,ff8 and msgtest/"
