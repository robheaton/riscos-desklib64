# NOTICE - DeskLib and this 64-bit port

## DeskLib's licence

DeskLib is **"The FreeWare C library for RISC OS machines."** Many authors have written it over many years, and copyright remains with them. Each source file carries its authors' copyright notice, and `LICENCE` lists the contributors.

DeskLib is under its own licence, which upstream keeps in `!DeskLib/Docs/TextHelp`. It is **reproduced verbatim in `LICENCE`** in this repository, because its first condition is that the copyright messages and conditions are distributed intact with every copy. In summary (the `LICENCE` text is authoritative):

- DeskLib may be copied and distributed, provided that:
  - the copyright messages and conditions stay intact;
  - only original, unaltered copies are distributed (alterations are to be sent to the moderator so they can become official updates);
  - no profit is made from the distribution.
- Software built with DeskLib must acknowledge, in its distribution, that DeskLib (or parts of it) was used.
- There is no warranty.

DeskLib is **not** under the GPL.

## Modifications in this repository

DeskLib64 is [DeskLib32](https://github.com/adyoull/desklib32) (Andrew Youll's build, commit `957863a`, which is upstream DeskLib master `f7469f4` with a few fixes), with 63 files modified and one added (`include/dl64.h`) so that it builds for 64-bit RISC OS and its structures keep the layout that the Window Manager uses. The 259 ARM assembler files are kept as they are and are not built: `gen/` and `hand/` have the same SWI veneers in C. Every change and its reason is in `MODIFICATIONS64.md`, and the exact edits are in `patches/desklib64-vs-desklib32.diff`.

DeskLib's licence asks that altered copies are not passed on, but sent back so they can become official updates. In line with that, these changes are documented file by file and are being offered to DeskLib's maintainers.

The translator (`tools/armv2c.py`), the build scripts, the demonstration applications (`tests/`) and the documents are new and are not part of DeskLib; they are Copyright (C) 2026 Rob Heaton, GNU General Public License, version 3 or (at your option) any later version. The files in `gen/` and `hand/` are DeskLib's veneers in another language, so DeskLib's licence applies to them.
