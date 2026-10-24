# DeskLib64 - Changelog

## 0.1 (24 October 2026)

First release: DeskLib32 (commit `957863a`, upstream DeskLib master `f7469f4`) for RISC OS 64 (AArch64), built with the AArch64 RISC OS GCC 16 tool chain 0.14. The OS structures keep their 32-bit layout (`dl_ptr`), the ARM assembler veneers are C (240 translated, the rest by hand), `Mem` is on `malloc`. Two demonstration applications. See MODIFICATIONS64.md and docs/STATUS.md.
