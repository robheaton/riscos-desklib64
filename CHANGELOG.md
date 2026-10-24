# DeskLib64 - Changelog

## Unreleased

* Fixed: a "no validation string" value (`-1`, stored as `0xffffffff` in a `dl_ptr`) was treated as a pointer in `Template_Clone`, `Template_Free`, `Menu_FullDispose` and `Icon_FindValidationStringCommand` (MODIFICATIONS64.md C5).
* Fixed (translator): `LDM` with the base register in the register list loaded the base first and used the new value for the later loads. Affected `Sprite_UnRedirect`, `Wimp_ForceRedraw`, `Wimp_SetCaretPosition`, `Wimp_BlockCopy` and `PDriver_InsertIllustration` (MODIFICATIONS64.md section 3).
* Fixed (also in DeskLib32): `Filing_OpenDir` copied only `sizeof (char *) - 1` characters of the directory name, so `Filing_ReadDir` read the wrong path (MODIFICATIONS64.md C6).
* Fixed (hand-written veneer): `Menu_FontMenuDecode` returned NULL because the buffer size was not passed to `Font_DecodeMenu` (MODIFICATIONS64.md section 3).
* New test applications `tests/dialogs` (`Dialog2`), `tests/save` (`Save`, file icon drag) and `tests/sprite` (`Sprite`) `tests/pane` (`Pane`, `Pane2`) and `tests/msgs` (`MsgTrans`, `Msgs`) `tests/filing` (`File`, `Filing`) `tests/colour` (`ColourTrans`) and `tests/font` (`Font`, `Font2`, font menu). `tests/sprite` now fills its canvas (it drew outlines).

## 0.1 (24 October 2026)

First release: DeskLib32 (commit `957863a`, upstream DeskLib master `f7469f4`) for RISC OS 64 (AArch64), built with the AArch64 RISC OS GCC 16 tool chain 0.14. The OS structures keep their 32-bit layout (`dl_ptr`), the ARM assembler veneers are C (240 translated, the rest by hand), `Mem` is on `malloc`. Two demonstration applications. See MODIFICATIONS64.md and docs/STATUS.md.
