# DeskLib64: every change from DeskLib32

DeskLib32 is Andrew Youll's build of upstream DeskLib (https://github.com/adyoull/desklib32, commit `957863a`). `patches/desklib64.patch` has the source changes (63 files: 10 headers, 53 library C files); the assembler is translated by `tools/armv2c.py` and the rest is in `hand/`.

## Headers
* `Core.h`: `dl_ptr`, `DL_PTR`, `DL_P` (a 32-bit word that holds a pointer in an OS structure) and `#include <stdint.h>`.
* Pointer fields of the structures that the OS reads or writes became `dl_ptr`: see the table in docs/PORTING.md (`Wimp.h`, `Screen.h`, `Font.h`, `PDriver.h`, `Sprite.h`). They were found by comparing `sizeof` of every structure type compiled for a 32-bit and a 64-bit target: after the change only DeskLib's own bookkeeping structures (`linklist_header`, `dialog2_block`, `filing_dirdata`, `print_block`, `save_saveblock`, `template_record`, `clear_picture`) differ, which is as it should be.
* `Menu2.h`: `menu2_handle` is `intptr_t` (it holds a pointer). `Print.h`: `print_result` is `intptr_t` (it holds an `os_error *` or a code).
* `PCI.h`: the second member `flags` of `pci_address` is `oflags` (two members with one name are an error in GCC).
* `dl64.h` (new): the SWI call and `DL_ARG` for the generated veneers.

## Library C
* 53 files: uses of the `dl_ptr` fields (reads `((char *) DL_P (x))`, assignments `x = DL_PTR (v)`), done with a script and then by hand where the compiler still complained; `(int)` casts of pointers became `(intptr_t)` (a pointer through `int` loses its upper half, and for an address of 2 GB or more its sign).
* `Template/Delete.c`: `free (t->windowdef)` instead of `free (&t->windowdef)` (the original freed the address of a field).
* `Icon/DisposeInd.c`: compares the validation string with `0xffffffff` as a word.

## Assembler
The 259 files of ARM SWI veneers (`Libraries/*/*.s`) are not used. `tools/armv2c.py` translates 240 of them to C (the registers are variables, the stack is a word array, a SWI is `swi_f`; the prototype comes from DeskLib's header); `hand/` has the rest, written by hand from the assembler: `File.c` (the File functions and `File_GetType`), `Filing.c`, `Str.c`, `Screen.c`, `DragASpr.c`, `Menu.c` (the font menu), `Misc.c` (functions that return a pointer through a pointer parameter or read their arguments from the stack through `ip`: the whole 64-bit pointer is stored) and `Mem.c`.

## Memory manager
`Mem` is on `malloc` (`hand/Mem.c`): the original builds its own heap above the program's slot with a Norcroft hook and keeps pointers in integers.

## Not built
`BackTrace`, `PCI`, `USB`.
