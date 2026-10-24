# Porting an application from DeskLib to DeskLib64

DeskLib64 keeps DeskLib's API. What changes is the size of a pointer: 8 bytes on AArch64, while the structures that the Window Manager reads and writes (window blocks, icon blocks, menus, messages ...) are made of 32-bit words. The rules:

## 1. Pointer fields of OS structures are `dl_ptr`

`dl_ptr` is `uint32_t`. `DL_PTR (p)` makes one from a pointer and `DL_P (v)` gives the pointer back (as `void *`; cast it to what you need). The data must be below 4 GB: the heaps and stacks of RISC OS 64 programs are.

| Structure | Field(s) |
|---|---|
| `icon_indtext` | `buffer`, `validstring` (the "no validation" value is `(dl_ptr) -1`) |
| `icon_indsprite` | `name`, `spritearea` (1 = the Wimp sprite area) |
| `window_block` | `spritearea` |
| `menu_submenu` | `menu` |
| `template_block` | `buffer`, `workfree`, `workend`, `font`, `name` |
| `message_ramfetch`, `message_ramtransmit` | `buffer` |
| `pointer_shapeblock` | `shapedata` |
| `screen_modeval` | `spec_mode` |
| `font_string` | `s` |
| `printer_info` | `description` |
| `sprite_outputstate` | `spritearea`, `sprite`, `savearea` |

```c
/* 32-bit DeskLib */                             /* DeskLib64 */
ic.icondata.data.indirecttext.buffer = text;     ic.icondata.data.indirecttext.buffer = DL_PTR (text);
puts (icon.data.indirecttext.buffer);            puts ((char *) DL_P (icon.data.indirecttext.buffer));
```

The compiler finds every place: an assignment of a pointer to a `dl_ptr` is an error.

## 2. Handles that carry pointers

`menu2_handle` and `print_result` are `intptr_t` (an `int` was a pointer on 32 bits).

## 3. Casts between pointers and integers

`(int) pointer` truncates, and for an address of 2 GB or more it also changes the sign: `(menu_item *) ((int) menu + 28)` is a wrong address. Cast through `intptr_t` or `uintptr_t`, or better do the arithmetic on pointers.

## 4. Pointers returned through a pointer

`void **`, `char **` and `sprite_header **` out parameters receive the whole pointer. (The ARM veneers stored a 32-bit word.)

## 5. Small structures passed by value

A few DeskLib functions take a structure of one word by value. DeskLib64's veneers pass it as a word; a structure of two words (such as `wimp_point`) would not be passed the same way, so such a function has to be checked.

## 6. `SWI ()` outputs are 4 bytes

The `SWI (in, out, number, ...)` function stores each output register as a 4-byte `int` through the pointer you give it. Passing the address of a pointer (`char **end`) fills only the low half: set the pointer to 0 first, or take the register in an `unsigned int` and convert (`(char *) DL_P (v)`). If the SWI returns an address, it is below 4 GB, so that conversion is exact.

## 7. What an OS call returns when it gives an error

`_kernel_swi` in the AArch64 library (and so `SWI ()`) does **not** return the output registers when the SWI gives an error, as the Shared C Library's did. Code that relies on a register after an error ("`OS_ReadVarVal` with a size of -1 gives the length of a variable that exists") must test the error number instead. Also pass the right thing in each register: a SWI that wants the address of a word must not be given the value (`OS_SetVarVal` with a number).

## 8. Bytes are unsigned

Norcroft's plain `char` is unsigned. DeskLib64 and its tests are built with `-fsigned-char`, so plain `char` is signed there. Code that builds a number from bytes (`b[1] + (b[2] << 8)`) must use `unsigned char` (or mask with `& 255`), or a byte of 128 or more makes the result negative: `Pointer_GetPosition` read x = 400 as 144 until it was changed (MODIFICATIONS64.md C14).

## 9. The language level

Old Norcroft code uses implicit function declarations, `int` for pointers and similar. A current GCC refuses these in its default mode: use `-std=gnu89 -fpermissive` for old sources (`ports/build-examples.sh` does), and fix them properly when you can (the errors say where an `int` receives a pointer).

## 10. Reads at address 0

RISC OS 32 lets a program read address 0 (low memory) without a fault, so a NULL pointer dereference on a read can go unnoticed for years. On RISC OS 64 it is a data abort ("abort on data transfer at ..."). The Wimp error box names the address; `-Wl,-Map` and `nm` on the object give the function. Example: `!FontDemo` of upstream DeskLib reads a field of a NULL pointer in its redraw code (`ports/patches/FontDemo.patch`).

## 11. Names that the C library has

`strnicmp`, `strdup` and `stricmp` are in the AArch64 C library: DeskLib64's copies are compiled out (`__TARGET_SCL__`), so a program that defines its own gets a link error "multiple definition" and should drop its copy.

## 12. Resources and the application folder

* `!Run` must not set a small `WimpSlot` (64 KB to 320 KB): a 64-bit program is bigger. Leave the line out, or ask for several MB.
* Sprites that `Icon_BarIcon` uses must be in the Wimp's pool (`*IconSprites <path>`); `Icon_BarIconUser` takes a user sprite area.
* A task started with `Wimp_StartTask` that is a visible command (`Echo`) opens a text window that waits for a key. Use `TaskWindow_Start` or a command that does not print.
* `TaskWindow_Start` returns a handle of 0: the real one is in the `message_TASKWINDOW_EGO` message.
* Window help (`Window_AutoHelp`) needs `message_HELPREQUEST` claimed with `EventMsg_Claim` as well.

## 13. Not available

`BackTrace` (Norcroft's stack unwinding), `PCI` and `USB`. `Mem` works as before but its blocks never move (so `Mem_Compact` does nothing and an anchor only changes when its block is allocated, resized or freed).

## Finding what went wrong

* The Wimp's error box gives the address of an abort. Link with `-Wl,-Map=app.map`: the map has the start of every object and function; `aarch64-riscos-nm` on the objects names the function.
* Log to a file from the application (`<Wimp$ScrapDir>`) and read it with the agent or `*Type`: every step that was reached is there.
* A call that gives an `os_error *` should be tested (`Error_Check`) in test code: several DeskLib functions ignore the error.
* `docs/COVERAGE.md` says which DeskLib64 functions have been run, and `docs/STATUS.md` what each test checks.

## Building

`aarch64-riscos-gcc -munixlib -O2 -fsigned-char -I dist/include app.c dist/lib/libDeskLib64.a -o '!App/!RunImage,ff8'`. `-fsigned-char` matches DeskLib's expectations. An application is a folder with `!Run`, `!Boot` and `!RunImage` of type &FF8; the Wimp starts a 64-bit program that calls `Wimp_Initialise` as a desktop task.
