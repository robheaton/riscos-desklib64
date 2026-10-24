# DeskLib64 - every change from DeskLib32, and why

This document lists **every** difference between the source in `src/` and DeskLib32 (Andrew Youll's build of upstream DeskLib: https://github.com/adyoull/desklib32, commit `957863a`), and says what is built from the assembler instead. For each change it gives the problem, the evidence, what was changed, what it means for programs that use the library, and how it was checked. The exact source changes are also in `patches/desklib64-vs-desklib32.diff` (62 modified files and 1 added).

---

## 1. What the source is based on

`src/` is DeskLib32 at `957863a` (638 files): 576 are byte-identical to it, 62 are modified (9 headers, 53 library C files) and 1 is added (`include/dl64.h`). The 259 ARM assembler files (`Libraries/*/*.s`) and their include files are **unchanged and not built**: `gen/` and `hand/` have the same SWI veneers in C (section 3).

The target is AArch64 RISC OS 64, with the AArch64 RISC OS GCC 16 tool chain (0.14 or newer) and `-munixlib -O2 -fsigned-char`. On this target `int` and `long` are 32 and 64 bits and a pointer is 64 bits; the Window Manager's data structures are made of 32-bit words.

---

## 2. Changes to C source and headers

### H1. `include/Core.h` - `dl_ptr`

**Problem.** DeskLib keeps pointers in operating system structures (the buffer of an indirected icon, the sprite area of a window, the submenu of a menu item ...). A pointer is 8 bytes here, so the structure is bigger than the OS expects and every later field is in the wrong place.

**Change.** `typedef uint32_t dl_ptr;` and two macros: `DL_PTR (p)` makes one from a pointer, `DL_P (v)` gives the pointer back (as `void *`). `<stdint.h>` is included.

**Impact.** A program writes `DL_PTR` and `DL_P` where it used to assign a pointer to such a field. The data must be below 4 GB, as the heaps and stacks of RISC OS 64 programs are.

### H2. The pointer fields of the OS structures are `dl_ptr`

**Problem.** After compiling DeskLib's headers unchanged, the sizes of the structures differ from the 32-bit ones: `window_block` 112 bytes (88), `icon_block` 48 (32), `icon_data` 24 (12), `menu_item` 48 (24), `template_block` 48 (24), `message_block` 264 (256), `event_pollblock` 272 (260), `sprite_header` 48 (44) ... A window created from such a block, or a template loaded into it, is wrong.

**Evidence.** All the structure types of the headers were compiled for a 32-bit and a 64-bit target (`gcc -m32` and `-m64`, with stand-ins for the C library headers) and their `sizeof` compared: 29 types differed. `Template/Clone.c` has a compile-time check that `window_block` is 88 bytes (M3 of DeskLib32): it failed.

**Change.** The pointer members of the structures that the OS reads or writes became `dl_ptr`: `icon_indtext` (`buffer`, `validstring`), `icon_indsprite` (`name`, `spritearea`), `window_block` (`spritearea`), `menu_submenu` (`menu`), `template_block` (`buffer`, `workfree`, `workend`, `font`, `name`), `message_ramfetch` and `message_ramtransmit` (`buffer`), `pointer_shapeblock` (`shapedata`), `screen_modeval` (`spec_mode`), `font_string` (`s`), `printer_info` (`description`) and `sprite_outputstate` (`spritearea`, `sprite`, `savearea`). Files: `Wimp.h`, `Screen.h`, `Font.h`, `PDriver.h`, `Sprite.h`.

DeskLib's own bookkeeping structures (`linklist_header`, `dialog2_block`, `filing_dirdata`, `print_block`, `save_saveblock`, `template_record`, `clear_picture`) keep ordinary pointers: the OS never sees them.

**Impact.** Source that assigns a pointer to one of these fields, or reads one as a pointer, stops compiling (an error in GCC 16), which is the point: the places are found by the compiler. docs/PORTING.md has the list and examples.

**Verification.** The same size comparison after the change: only the bookkeeping structures differ. `window_block` is 88 bytes again and `Template/Clone.c`'s check passes. A window made from a template file and one made in code open correctly in the demonstration applications.

### H3. `include/Menu2.h`, `include/Print.h` - handles that carry pointers

**Problem.** `menu2_handle` was an `int` holding a pointer to the menu block, and `print_result` an `int` that held either a result code or an `os_error *`. A 64-bit pointer does not fit.

**Change.** Both are `intptr_t`.

**Impact.** None for a program that only passes them on; a program that stored one in an `int` has to widen it.

### H4. `include/PCI.h` - a duplicate member name

**Problem.** `pci_address` has two members called `flags` (an input and an output). Norcroft accepted that; GCC reports an error.

**Change.** The output member is `oflags`.

### H5. `include/dl64.h` (new)

What the generated veneers include: the declarations of `swi_x` and `swi_f` (the SDK's SWI calls) and `DL_ARG`, which turns a C argument of any one-word type, a pointer or a small structure into the 64-bit value of a register.

### C1. 30 files in `Libraries/` - uses of the `dl_ptr` fields

**Problem.** After H2 the code that read or wrote the fields as pointers no longer compiled (79 errors in about 30 files).

**Change.** A read is `((char *) DL_P (x))` (or `DL_P (x)` for a sprite area or a menu), an assignment `x = v;` is `x = DL_PTR (v);`. `tools/fixptr.py` made the bulk of it; the rest was done by hand (`icon.indirecttext.validstring = (dl_ptr) -1` for "no validation", the template block fields, `free (DL_P (...))`).

**Verification.** The library compiles without errors or pointer warnings.

### C2. 32 files in `Libraries/` - `(int)` casts of pointers

**Problem.** `(menu_item *) (((int) menu) + sizeof (menu_block))` and similar. A pointer through `int` loses its top half, and for an address of 2 GB or more `int` is negative and the result is sign extended into a wrong address. The heaps of RISC OS 64 programs are above 2 GB.

**Change.** 56 lines: `(int)` of a pointer became `(intptr_t)`. A window handle passed as a menu pointer (the dialogue code opens a window as a menu) is `(menu_block *)(intptr_t) dbox->window`.

**Verification.** No `-Wpointer-to-int-cast` or `-Wint-to-pointer-cast` warnings remain.

### C3. `Template/Delete.c`, `Str/LeafName.c`

* `Template_Delete` freed `&t->windowdef`, the address of a field of the record, not the block that the field points to (GCC 16 says so). It is `free (t->windowdef)`.
* `Str_LeafName` had `(char *)(int) path`, which is `(char *) path`.

### C4. `Icon/DisposeInd.c`

The validation string is compared with `0` and `0xffffffffu` as words (it is a `dl_ptr`).

---

## 3. Assembler sources

DeskLib's SWI veneers are 259 small ARM assembler files: they move the arguments into the right registers, make one SWI call and store the results. They cannot be assembled for AArch64. They are **not changed and not built**; their function is in C:

* **`gen/` (240 files)** is made by `tools/armv2c.py` from the `.s` files: the registers are 64-bit variables, the flags are variables, the stack is an array of words (so that `ip` and `sp` are real addresses), a SWI is `swi_f` (which also returns the flags, for the veneers that test carry or zero), and the C prototype comes from DeskLib's own header. Loads and stores of words are 32-bit, as the OS data are. Arguments after the fourth are on the stack, as in the ARM calling convention. `REGEN=1 ./build.sh` makes `gen/` again.
* **`hand/` (written by hand from the assembler)**: `File.c` (the File functions and `File_GetType`), `Filing.c`, `Str.c`, `Screen.c`, `DragASpr.c`, `Menu.c` (the font menu), `Misc.c` (functions that return a pointer through a pointer parameter, or read their arguments from the stack through `ip`: the ARM code stored a 32-bit word, here the whole pointer is stored) and `Mem.c`. They are the veneers that have branches or data tables, or that the translator cannot do correctly.
* **`Mem`** (the relocatable memory manager) is a small version on `malloc` in `hand/Mem.c`: the blocks never move, `Mem_Compact` does nothing and an anchor only changes when its block is allocated, resized or freed. The original builds its own heap above the program's slot with a Norcroft hook and keeps pointers in integers.
* **Not built:** `BackTrace` (Norcroft's stack unwinding), `PCI` and `USB`.

**Verification.** All 506 objects build. `hello` and `demo2` run: they use `Wimp_Initialise`, `Wimp_CreateWindow`, `Wimp_CreateIcon`, `Wimp_Poll`, `Wimp_OpenWindow`, `Wimp_CloseDown`, the Template and Menu code and `Icon_Printf`. The other veneers have been compiled but not run (docs/STATUS.md).

---

## 4. Build configuration (not source changes)

`build.sh` compiles `src/Libraries`, `gen/` and `hand/` with `aarch64-riscos-gcc -munixlib -O2 -fsigned-char -w` and the includes `dist/include` (DeskLib's headers under the names `DeskLib:Wimp.h` and so on, which its sources use). `-fsigned-char` is what DeskLib expects of `char`.

## 5. Upstream problems seen but not changed

* `Wimp_Poll` is a macro in `WimpSWIs.h` (`Wimp_Poll3 (mask, event, 0)`); the assembler file also defines a function of that name, which no C code can reach. It is not built.
* A struct of two words (such as `wimp_point`) passed by value would not be passed as the ARM convention did; no DeskLib function does it, but a program should check (docs/PORTING.md).

## 6. Still to be done

* Run the veneers and libraries that the two demonstration applications do not reach (docs/STATUS.md), and fix what that finds.
* `BackTrace` for AArch64.
* Offer the changes to DeskLib's maintainers.
