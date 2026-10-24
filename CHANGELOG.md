# DeskLib64 - Changelog

## Unreleased

* Fixed: a "no validation string" value (`-1`, stored as `0xffffffff` in a `dl_ptr`) was treated as a pointer in `Template_Clone`, `Template_Free`, `Menu_FullDispose` and `Icon_FindValidationStringCommand` (MODIFICATIONS64.md C5).
* New test applications `tests/dialogs` (`Dialog2`) and `tests/save` (`Save`, file icon drag).

## 0.1 (24 October 2026)

First release: DeskLib32 (commit `957863a`, upstream DeskLib master `f7469f4`) for RISC OS 64 (AArch64), built with the AArch64 RISC OS GCC 16 tool chain 0.14. The OS structures keep their 32-bit layout (`dl_ptr`), the ARM assembler veneers are C (240 translated, the rest by hand), `Mem` is on `malloc`. Two demonstration applications. See MODIFICATIONS64.md and docs/STATUS.md.
