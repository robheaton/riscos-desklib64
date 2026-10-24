# DeskLib64 - every change from DeskLib32, and why

This document lists **every** difference between the source in `src/` and DeskLib32 (Andrew Youll's build of upstream DeskLib: https://github.com/adyoull/desklib32, commit `957863a`), and says what is built from the assembler instead. For each change it gives the problem, the evidence, what was changed, what it means for programs that use the library, and how it was checked. The exact source changes are also in `patches/desklib64-vs-desklib32.diff` (69 modified files and 1 added).

---

## 1. What the source is based on

`src/` is DeskLib32 at `957863a` (638 files): 569 are byte-identical to it, 69 are modified (9 headers, 60 library C files) and 1 is added (`include/dl64.h`). The 259 ARM assembler files (`Libraries/*/*.s`) and their include files are **unchanged and not built**: `gen/` and `hand/` have the same SWI veneers in C (section 3).

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

### C5. `Template/Clone.c`, `Template/Free.c`, `Menu/FullDispos.c`, `Icon/Validation.c` - "no validation string" is not a pointer

**Problem.** DeskLib uses a validation-string value of `-1` for "none". On 32 bits `(intptr_t) p > 0` is false for it, so the code does not free it. In the `dl_ptr` field the value is `0xffffffff`, which is positive as a 64-bit `intptr_t`, so `Template_Clone` copied from address `0xffffffff` and `Template_Free`, `Menu_FullDispose` and `Icon_FindValidationStringCommand` treated it as a real string.

**Evidence.** A dialogue box built from a template with indirected icons that have no validation string ended in a data abort in `malloc` after the window was freed; with the four comparisons changed the abort does not happen.

**Change.** Each of the four tests also compares the word with `0xFFFFFFFFu`.

**Verification.** `tests/dialogs` opens, fills and closes dialogue boxes made from such a template many times.

### C6. `Filing/ReadDir.c` - the directory name is copied with `sizeof` of a pointer

**Problem.** `Filing_OpenDir` copies the name with `strncpy (dirdata->dirname, dirname, sizeof (dirdata->dirname) - 1)`. `dirname` is a `char *`, so the size is that of a pointer: 4 on 32 bits (three characters copied), 8 on 64 bits (seven). The later calls read a truncated path, so `Filing_ReadDir` finds no objects (or the wrong directory) for any name longer than that. This is also in DeskLib32.

**Evidence.** `tests/filing`: reading a directory with `Filing_OpenDir` and `Filing_ReadDir` returned no objects, while `Filing_ReadDirNames` on the same path returned three.

**Change.** The name is copied whole (`strcpy`: the buffer was allocated with `strlen + 1`).

**Verification.** All three forms (names, entries, full entries) return the three objects.

### C7. `Clipboard/Clipboard.c` - the size of the data request, and an uncleared message

**Problem.** `Clipboard_Request` sets the message size to `44 + num` where `num` is the number of file types: that counts bytes where it must count 4-byte words, so the size is not a multiple of 4 and the Wimp refuses the message ("Message block is too big / not a multiple of 4"). The error is not returned to the caller, so the request is silently never sent (this is also in DeskLib32). `Clipboard_Claim` builds its message without clearing it, so the flag bits above the three that are set contain stack contents.

**Evidence.** `tests/clip` runs two tasks: the second calls `Clipboard_Claim` and `Clipboard_Request` and sends the same messages directly with `Wimp_SendMessage`. The claim arrives at the first task; the request from `Clipboard_Request` did not, while the same message with the size 52 did.

**Change.** The size is `44 + 4 * num`, and `Clipboard_Claim` clears its message.

**Verification.** Both messages arrive, with the file types and the internal handle intact.

### C8. `Event/Event.c` - `event_ANY` claims enabled pollword events

**Problem.** `Event_Claim (event_ANY, ...)` (which `Dialog_Create` uses for its window) enables every event type except null, including `event_NONZEROPOLLWORD`. Enabling that type sets bit 22 of the poll mask, which tells the Window Manager that register 3 of `Wimp_Poll` points to a pollword, but `Event_Poll` always passes 0. The Wimp is then told to read a pollword at address 0.

**Evidence.** In `tests/dialogs`, the older `Dialog` button (`Dialog_Create`, `Dialog_ShowStatic`, `Dialog_WaitForClick`) stopped the task and left the Window Manager not responding; `Dialog2`, which claims specific events only, did not. After the change the button works: the box opens, the fields are read back and the box is destroyed.

**Change.** The four loops that add or remove usage for `event_ANY` skip `event_NONZEROPOLLWORD`. A program that wants pollword events still claims that event by name.

**Verification.** `tests/dialogs`, "Old API".

### C9. `Font/ScanString.c` - an address returned through `SWI ()`

**Problem.** `Font_ScanString` passes its `char **end` parameter to `SWI ()` as an output. `SWI ()` stores each output register as a 4 byte `int`, so only the low half of the pointer that `*end` holds is replaced; the top half keeps whatever the caller's variable had.

**Evidence.** `tests/font3` set `end` to `0xDEADBEEF00000000` before the call: it came back as `0xDEADBEEF00049CA9`.

**Change.** The register is taken in an `int` and widened (`(char *) (uintptr_t) (unsigned int)`) before it is stored in `*end`. (`Environment_TaskNameFromHandle` has the same pattern, but initialises its pointer to 0 first, so it is correct.)

**Verification.** `tests/font3`: the end pointer is `string + 9`.

### C10. `Str/strnicmp.c`, `Str/strdup.c` - clash with the C library

**Problem.** DeskLib defines `strnicmp` and `strdup` unconditionally. The AArch64 C library has both, so a program that links `libDeskLib64.a` and uses either gets "multiple definition" from the linker. (`stricmp` is already guarded in the same way.)

**Evidence.** `tests/env` did not link.

**Change.** Both files are wrapped in `#ifdef __TARGET_SCL__`, so the C library's functions are used.

**Verification.** `tests/env` links and its `strnicmp` and `strdup` checks pass.

### C11. `Environment/SysVars.c` - `Environment_SysVarRead` tested a register that is not returned

**Problem.** The function finds out whether a variable exists by calling `OS_ReadVarVal` with a buffer size of -1 and testing the length returned in R2 (the call gives the "buffer overflow" error when the variable exists). The SDK's `_kernel_swi` does not return the output registers when a SWI gives an error (the Shared C Library's did), so R2 was whatever was on the stack. The value was also used as an index into the buffer.

**Evidence.** `tests/env`: the first run aborted in `Environment_SysVarRead` with a wild address. (`Resource_Initialise` calls this function.)

**Change.** Existence is the error number: no error, or `&1E4` (buffer overflow), means the variable exists; the length is only used after the real read, and only when it fits.

**Verification.** `tests/env`: a variable that does not exist reads as FALSE; a string and a number variable read back.

### C12. `Environment/SysVars.c` - `Environment_SysVarSet` with `sysvar_NUMBER`

**Problem.** For a number variable the function passed the number itself in R1, but `OS_SetVarVal` takes the address of the word. The OS read from address 42. In 32-bit RISC OS that is a read of low memory (a wrong value, or an abort); on RISC OS 64 it is a data abort inside the kernel.

**Evidence.** `tests/env`: setting a number variable aborted.

**Change.** R1 is the address of the number (length 4).

**Verification.** `tests/env`: the variable reads back as the word 42.

### C13. `Environment/Tasks.c` - the Task Manager's block, and a free in the wrong index

**Problem.** `Environment_TaskList`, `Environment_TaskIsActive` and `Environment_TaskInfo` read the `TaskManager_EnumerateTasks` block through a structure with a `char *` name. The OS writes a 32-bit pointer (16 bytes in all), so the name, size and flags were read at the wrong offsets and the name pointer was garbage. `Environment_TaskNameFromHandle` took a pointer through `SWI ()` (4 bytes, see C9) into a `char *`. The clean-up when `malloc` fails in `Environment_TaskList` freed `list[i]` (the entry that failed) instead of `list[j]` (also in DeskLib32).

**Evidence.** `tests/env`: `Environment_TaskList` aborted with a data abort on a wild address.

**Change.** The block uses `dl_ptr`, the name is converted with `DL_P`, `TaskNameFromHandle` takes the register in an `unsigned int`, and the loop frees `list[j]`.

**Verification.** `tests/env`: the task list has the Filer in it, `Environment_TaskIsActive` finds it and does not find a task that is not running.

---

## 3. Assembler sources

DeskLib's SWI veneers are 259 small ARM assembler files: they move the arguments into the right registers, make one SWI call and store the results. They cannot be assembled for AArch64. They are **not changed and not built**; their function is in C:

* **`gen/` (240 files)** is made by `tools/armv2c.py` from the `.s` files: the registers are 64-bit variables, the flags are variables, the stack is an array of words (so that `ip` and `sp` are real addresses), a SWI is `swi_f` (which also returns the flags, for the veneers that test carry or zero), and the C prototype comes from DeskLib's own header. Loads and stores of words are 32-bit, as the OS data are. Arguments after the fourth are on the stack, as in the ARM calling convention. `REGEN=1 ./build.sh` makes `gen/` again.
  **A translator bug found by `tests/sprite` (fixed):** for `LDMIA r0, {r0, r1, r2, r3}` (the base register is in the list) the first version loaded `r0` and then used the new `r0` as the base for the next loads. The ARM instruction takes all the addresses from the old value. The translator now keeps the base in a temporary when it is in the list of an `LDM`. Five veneers changed: `Sprite_UnRedirect` (its first load used a garbage address and the task aborted), `Wimp_ForceRedraw`, `Wimp_SetCaretPosition`, `Wimp_BlockCopy` and `PDriver_InsertIllustration`.
* **`hand/` (written by hand from the assembler)**: `File.c` (the File functions and `File_GetType`), `Filing.c`, `Str.c`, `Screen.c`, `DragASpr.c`, `Menu.c` (the font menu), `Misc.c` (functions that return a pointer through a pointer parameter, or read their arguments from the stack through `ip`: the ARM code stored a 32-bit word, here the whole pointer is stored) and `Mem.c`. They are the veneers that have branches or data tables, or that the translator cannot do correctly.
  **A bug in `hand/Menu.c` found by `tests/font` (fixed):** `Menu_FontMenuDecode` made the second `Font_DecodeMenu` call with the buffer size (`r4`) cleared; the assembler kept the size from the first call in `r4`. The OS then returned an error and the function returned NULL for every selection. It now passes the size.
* **`Mem`** (the relocatable memory manager) is a small version on `malloc` in `hand/Mem.c`: the blocks never move, `Mem_Compact` does nothing and an anchor only changes when its block is allocated, resized or freed. The original builds its own heap above the program's slot with a Norcroft hook and keeps pointers in integers.
* **Not built:** `BackTrace` (Norcroft's stack unwinding), `PCI` and `USB`.

**Verification.** All 506 objects build. `hello` and `demo2` run: they use `Wimp_Initialise`, `Wimp_CreateWindow`, `Wimp_CreateIcon`, `Wimp_Poll`, `Wimp_OpenWindow`, `Wimp_CloseDown`, the Template and Menu code and `Icon_Printf`. The other veneers have been compiled but not run (docs/STATUS.md).

---

## 4. Build configuration (not source changes)

`build.sh` compiles `src/Libraries`, `gen/` and `hand/` with `aarch64-riscos-gcc -munixlib -O2 -fsigned-char -w` and the includes `dist/include` (DeskLib's headers under the names `DeskLib:Wimp.h` and so on, which its sources use). `-fsigned-char` is what DeskLib expects of `char`.

## 5. Upstream problems seen but not changed

* `font_handle` is `unsigned char`, but `Font_FindFont` and `Font_GetWimpFont` store the result with a word store (`STR r0, [r6]`), so a caller's `font_handle h; Font_FindFont (&h, ...)` has three bytes after `h` overwritten. Declare handles as `unsigned int` (cast the address), or give each its own word, until this is changed upstream (present in DeskLib32).
* `Font_ListFonts (name, &count)` writes into `name` (a buffer of at least 40 characters, ended by a control character), but is declared `const char *`: a string literal passed to it is overwritten (and, with merged string constants, so is every other use of the same text).

* `Sprite.h` declares `Sprite_ReadMask` and `Sprite_WriteMask` (and the `P` forms), but the assembler defines `Sprite_ReadMaskPixel` and `Sprite_WriteMaskPixel` (`43ReadMask.s`, `44WriteMas.s`), so a program that calls the declared names does not link. `Sprite_WriteMaskPixel` takes `void *area` and `const int *pos`, not `sprite_area` and `const wimp_point *`. (Present in DeskLib32; not changed here. `tests/sprite` declares the names the library has.)

* `Wimp_Poll` is a macro in `WimpSWIs.h` (`Wimp_Poll3 (mask, event, 0)`); the assembler file also defines a function of that name, which no C code can reach. It is not built.
* A struct of two words (such as `wimp_point`) passed by value would not be passed as the ARM convention did; no DeskLib function does it, but a program should check (docs/PORTING.md).

## 6. Still to be done

* Run the veneers and libraries that the two demonstration applications do not reach (docs/STATUS.md), and fix what that finds.
* `BackTrace` for AArch64.
* Offer the changes to DeskLib's maintainers.
