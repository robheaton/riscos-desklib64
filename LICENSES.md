# Licences

## DeskLib

DeskLib is "The FreeWare C library for RISC OS machines", copyright its many authors, under its own licence (`LICENCE`, reproduced verbatim from upstream). It allows copying and distribution if the copyright messages and conditions stay intact, **only original, unaltered copies are distributed** (alterations should be sent to the moderator so that they become official updates), and no profit is made; programs built with it must acknowledge DeskLib.

Because of the second condition **this repository does not contain a modified copy of DeskLib**. It contains:

* `patches/desklib64.patch`: the 64-bit changes as a patch against DeskLib32 (Andrew Youll's build of upstream DeskLib, commit `957863a`). A patch contains lines of the files it changes, so it carries those files' licence: DeskLib's.
* `hand/*.c`: SWI veneers written from DeskLib's ARM assembler files, and `Mem.c`, which replaces DeskLib's `Mem`: DeskLib's licence applies to them.
* `build.sh` fetches the unaltered DeskLib32 and builds a copy in `work/`; `dist/` is the build output. The changes are being offered to DeskLib's maintainers, as the licence asks.

## The rest

The scripts, `tools/armv2c.py`, the demonstration applications (`tests/`) and the documents are Copyright (C) 2026 Rob Heaton, GNU General Public License, version 3 or (at your option) any later version.
