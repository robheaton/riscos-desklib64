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

## 6. Not available

`BackTrace` (Norcroft's stack unwinding), `PCI` and `USB`. `Mem` works as before but its blocks never move (so `Mem_Compact` does nothing and an anchor only changes when its block is allocated, resized or freed).

## Building

`aarch64-riscos-gcc -munixlib -O2 -fsigned-char -I dist/include app.c dist/lib/libDeskLib64.a -o '!App/!RunImage,ff8'`. `-fsigned-char` matches DeskLib's expectations. An application is a folder with `!Run`, `!Boot` and `!RunImage` of type &FF8; the Wimp starts a 64-bit program that calls `Wimp_Initialise` as a desktop task.
