# Porting examples: upstream DeskLib's example applications on RISC OS 64

`build-examples.sh` builds the example applications of upstream DeskLib ([riscos-dot-info/desklib](https://github.com/riscos-dot-info/desklib), folder `Examples`) for RISC OS 64 **without changing their C source**, with one exception (below). Their sources are not copied into this repository.

```
./build.sh /path/to/toolchain
ports/build-examples.sh /path/to/toolchain          # dist/examples/!TestApp ... (copy a folder to RISC OS 64)
```

What the script does, and what an application needs in general:

* **`-std=gnu89 -fpermissive`.** These sources were written for Norcroft C and use implicit declarations and integer/pointer mixing that a current GCC refuses in its default mode. `-fsigned-char` is needed too (see `docs/PORTING.md`).
* **Header names.** `#include "Desklib:Colourtran.h"` differs in case from the header name; RISC OS does not care, so the script links the odd spellings.
* **`WimpSlot` lines** are removed from `!Run` (a 64-bit program needs more than the 64 KB to 320 KB that the examples ask for).
* **One source change**: `patches/FontDemo.patch` (a test for a NULL pointer). `!FontDemo` redraws its window before a font has been chosen and reads a field through a NULL `font2`. RISC OS 32 allows a read at address 0, RISC OS 64 gives an abort. This is a bug in the example, not in DeskLib.

All eleven examples build. What each one does when run is in `docs/STATUS.md`.
