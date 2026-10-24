# DeskLib64 - Changelog

## Unreleased

* `docs/PORTING.md` rewritten from what the tests and the upstream examples showed: `SWI ()` outputs, what a SWI returns on an error, unsigned bytes, the language level, reads at address 0, names the C library has, resources and the application folder, and how to find a crash.
* New `ports/build-examples.sh`: builds the example applications of upstream DeskLib for RISC OS 64 (one small patch for `!FontDemo`); see `ports/README.md`.

## 0.3 (24 October 2026)

Four more test programs (`sprite2test`, `gaps`, `gaps2`, `gaps3`), two more fixes (MODIFICATIONS64.md C16 and C17) and a coverage report: the tests now reach 479 of the 619 library functions (`docs/COVERAGE.md`, made by `tools/coverage.py`). `src/` is still 71 modified files and 1 added; `gen/` is 239 files because `Sprite_AppendP` is now written by hand.

* New test `tests/gaps3` (a desktop task, 27 checks): window help (`Window_AutoHelp` answering a help request that the test sends to its own task), `Menu_PopUp`, `PopUpAuto` and `Warn`, `Icon_BarIconUser`, `Icon_ButtonIsHeld`, `Icon_FindValidationStringCommand`, `Wimp_Enumerate`, `Wimp_ReadPixTrans` and `Font_FindCaret`, `FindCaretJ` and `Caret`. No library change was needed.
* New test `tests/gaps2` (a desktop task, 50 checks): Pane2 (handles, numbers from 1, variables, flags, open, hide, delete), Pane (Link, SetFlags, Show, Hide, Delete), the Handler functions (called with events made by the test), Resource (automatic path, path, sprites, choices) and the rest of the Font functions. No library change was needed.
* Fixed (also in DeskLib32): `Icon_SetText`, `Icon_SetTextRJ`, `Icon_GetInteger` and `Icon_GetDouble` treated an icon with direct text as an indirected one and used its text as an address (MODIFICATIONS64.md C17).
* New test `tests/gaps` (a desktop task, 78 checks): Wimp window information calls, the Window functions, Template, the Icon colour, shade, selection and number functions, validation strings, `Icon_BarIcon`, and the Menu builders.
* Fixed (also in DeskLib32): `Sprite_AppendP` passed the second sprite by name to the pointer form of `OS_SpriteOp 35`, which refuses it ("Can't append sprite"); the second sprite is now looked up first (MODIFICATIONS64.md C16).
* New test `tests/sprite/sprite2test.c` (command line, 80 checks): the pointer forms of the Sprite functions, rows and columns, `Append`, palettes, `MemorySize`, `Get` from the screen, `ScreenSave`, `Merge`, `LoadFiles`.
* New `tools/coverage.py` and `docs/COVERAGE.md`: which library functions the test programs reach.

## 0.2 (24 October 2026)

Testing of most of the library: seventeen new test programs and applications (`tests/dialogs` to `tests/taskwindow`) and 13 fixes (MODIFICATIONS64.md C5 to C15, the translator fix and the font menu fix). `src/` is now 71 modified files and 1 added.

* Fixed: a "no validation string" value (`-1`, stored as `0xffffffff` in a `dl_ptr`) was treated as a pointer in `Template_Clone`, `Template_Free`, `Menu_FullDispose` and `Icon_FindValidationStringCommand` (MODIFICATIONS64.md C5).
* Fixed (translator): `LDM` with the base register in the register list loaded the base first and used the new value for the later loads. Affected `Sprite_UnRedirect`, `Wimp_ForceRedraw`, `Wimp_SetCaretPosition`, `Wimp_BlockCopy` and `PDriver_InsertIllustration` (MODIFICATIONS64.md section 3).
* Fixed (also in DeskLib32): `Filing_OpenDir` copied only `sizeof (char *) - 1` characters of the directory name, so `Filing_ReadDir` read the wrong path (MODIFICATIONS64.md C6).
* Fixed (hand-written veneer): `Menu_FontMenuDecode` returned NULL because the buffer size was not passed to `Font_DecodeMenu` (MODIFICATIONS64.md section 3).
* Fixed (also in DeskLib32): `Clipboard_Request` set a message size that the Wimp refuses, so the request was never sent, and `Clipboard_Claim` did not clear its message (MODIFICATIONS64.md C7).
* Fixed: `Event_Claim (event_ANY, ...)` enabled pollword events although `Event_Poll` passes no pollword, which stopped the older `Dialog` library working (MODIFICATIONS64.md C8).
* Fixed: `Font_ScanString` left the top half of the returned end pointer unchanged (MODIFICATIONS64.md C9).
* Fixed: `strnicmp` and `strdup` clashed with the C library at link time (MODIFICATIONS64.md C10).
* Fixed (also in DeskLib32): `Environment_SysVarRead` used a register that the SDK does not return on an error (C11), `Environment_SysVarSet` passed a number where the OS wants its address, which faulted in the kernel (C12), and the Task Manager functions read a 16 byte block through a 24 byte structure (C13).
* Fixed: `Pointer_GetPosition` read its position bytes as signed characters (MODIFICATIONS64.md C14).
* Fixed (also in DeskLib32): `TaskWindow_SendInput` told the task window task that 24 more bytes of input followed than were sent (MODIFICATIONS64.md C15).
* New test `tests/taskwindow` (`TaskWindow_Start` with output and "died" functions, `SendInput`, `Suspend`, `Resume`, `Kill`, `TaskWindow_Check`), with a helper program that runs in the task window.
* New test `tests/controls` (radios, options, `IncDec`, both slider libraries).
* New test `tests/menu2` (`Menu2`, `Error`), a desktop application that logs each step to a file.
* New test `tests/misc` (`Coord`, `LinkList`, `TextFile`, `Screen`, `OS_Byte`, `OS_Word`, `Module`, `Pointer`, `Kbd`, `Hourglass`; 61 checks).
* New test `tests/env` (`Str`, `Time`, `Environment`), a command-line program (51 checks).
* New test applications `tests/dialogs` (`Dialog2`), `tests/save` (`Save`, file icon drag) and `tests/sprite` (`Sprite`) `tests/pane` (`Pane`, `Pane2`) and `tests/msgs` (`MsgTrans`, `Msgs`) `tests/filing` (`File`, `Filing`) `tests/colour` (`ColourTrans`) and `tests/font` (`Font`, `Font2`, font menu) `tests/clear` (`Clear`, `Mem`), `tests/drag` (`DragASpr`) `tests/clip` (`Clipboard`) and `tests/font3` (`Font_Paint3`, `Font_ScanString`). `tests/sprite` now fills its canvas (it drew outlines).

## 0.1 (24 October 2026)

First release: DeskLib32 (commit `957863a`, upstream DeskLib master `f7469f4`) for RISC OS 64 (AArch64), built with the AArch64 RISC OS GCC 16 tool chain 0.14. The OS structures keep their 32-bit layout (`dl_ptr`), the ARM assembler veneers are C (240 translated, the rest by hand), `Mem` is on `malloc`. Two demonstration applications. See MODIFICATIONS64.md and docs/STATUS.md.
