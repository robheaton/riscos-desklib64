# DeskLib64

**DeskLib for RISC OS 64 (AArch64).** A port of [DeskLib](https://github.com/riscos-dot-info/desklib), "the FreeWare C library for RISC OS machines", to 64-bit RISC OS, built with the [AArch64 RISC OS GCC 16 tool chain](https://github.com/robheaton/riscos-gcc16#risc-os-64-aarch64-early-preview) (`aarch64-riscos-gcc`, with `-munixlib`).

It starts from [Andrew Youll's DeskLib32](https://github.com/adyoull/desklib32) (upstream DeskLib master with a few fixes) and changes only what 64 bits need. **This repository does not contain DeskLib itself**: `build.sh` fetches DeskLib32 at a fixed commit, applies the patch in `patches/`, translates DeskLib's ARM assembler SWI veneers to C, and builds `libDeskLib64.a` (see [LICENSES.md](LICENSES.md) for why).

**Status: early.** The library builds in full (506 objects) and two demonstration applications, `tests/hello` and `tests/demo2`, exercise the event system, windows, template files, icons and menus. A good deal of DeskLib (dialogue boxes, save and drag, sprites, panes, printing, fonts menus ...) has not been run yet: see [docs/STATUS.md](docs/STATUS.md). Please report problems as issues.

## Build

```bash
./build.sh /path/to/aarch64-riscos-gcc-16.2.0-0.14-linux-x86_64      # needs git, python3, patch; tool chain 0.14 or newer
tests/build.sh /path/to/the/same/tool/chain                           # the demo applications, in dist/apps/
```

`dist/lib/libDeskLib64.a` and `dist/include/` (the headers, as `DeskLib:Wimp.h` and so on: DeskLib's sources include `"DeskLib:Wimp.h"`) are what you need to build your own application:

```bash
aarch64-riscos-gcc -munixlib -O2 -fsigned-char -I dist/include myapp.c dist/lib/libDeskLib64.a -o '!MyApp/!RunImage,ff8'
```

Copy an application folder to a RISC OS 64 system and double-click it: it runs as a normal desktop task.

## What is different from the 32-bit DeskLib

* **Pointers inside operating system structures are 32 bits** (`dl_ptr`): a window or icon block, a menu, a template block ... keep exactly the layout that the OS reads and writes, so a field that held a pointer (the buffer of an indirected icon, the sprite area of a window, a submenu ...) is now a `dl_ptr`. Set one with `DL_PTR (pointer)`, read it with `DL_P (value)`. The data they point to must be below 4 GB. [docs/PORTING.md](docs/PORTING.md) lists the fields and the other differences an application has to know about.
* `menu2_handle` and `print_result` are `intptr_t` (they carry pointers).
* The ARM assembler veneers are C: `tools/armv2c.py` translates 240 of them, and `hand/` has the rest, written from the assembler. The SWI call is `swi_x`/`swi_f` of the SDK.
* `Mem` (the relocatable memory manager) is a small version on `malloc`: blocks never move.
* Not built: `BackTrace` (it needs Norcroft's stack unwinding), `PCI` and `USB` (hardware specific).

Every change from DeskLib32, with the reason, is in [MODIFICATIONS64.md](MODIFICATIONS64.md).

## Licence

DeskLib has its own licence (reproduced in `LICENCE`); the patch and the files derived from DeskLib's sources carry it. The tools, the build scripts and the demonstration applications are GPL-3.0-or-later. Applications built with DeskLib must acknowledge that DeskLib was used. See [LICENSES.md](LICENSES.md).

## Credits

DeskLib is the work of many authors (see `LICENCE`); DeskLib32 is Andrew Youll's build and fixes; DeskLib64 is by Rob Heaton, with Claude Code (Anthropic).
