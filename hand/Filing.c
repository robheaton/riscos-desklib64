/* Filing.c - DeskLib's Filing_MakePath and the single directory entry functions (Filing/MakePath.s, Single2.s, SingleFul2.s), by hand for AArch64 */
#include <stdint.h>
#include <string.h>
#include "dl64.h"
#define X 0x20000

char *Filing_MakePath (char *dest, const char *a, const char *b)
{
  char *p = dest;
  if (!dest || !a || !b) return 0;
  while (*a) *p++ = *a++;
  *p++ = '.';
  do *p++ = *b; while (*b++);
  return dest;
}

static os_error *single (int reason, const char *dirname, void *entry, int size, const char *match)
{
  uint64_t r[10];
  uint32_t next = 0;
  for (;;)
  {
    memset (r, 0, sizeof r);
    r[0] = (unsigned) reason; r[1] = (uintptr_t) dirname; r[2] = (uintptr_t) entry; r[3] = 256; r[4] = next; r[5] = (uint32_t) size; r[6] = (uintptr_t) match;
    if (swi_x (0x0C | X, r)) return (os_error *) (uintptr_t) r[0];		/* OS_GBPB 10 or 11 */
    if ((uint32_t) r[3] == 1) return 0;					/* an object was read */
    next = (uint32_t) r[4];
    if ((int32_t) next >= 0) continue;					/* more to look at */
    *(uint32_t *) ((char *) entry + 16) = 0;				/* objtype = 0: nothing found */
    return 0;
  }
}

os_error *Filing_SingleDirEntry2 (const char *dirname, filing_direntry *direntry, int size, char *match) { return single (10, dirname, direntry, size, match); }
os_error *Filing_SingleFullDirEntry2 (const char *dirname, filing_fulldirentry *direntry, int size, const char *match) { return single (11, dirname, direntry, size, match); }
