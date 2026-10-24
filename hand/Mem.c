/* Mem.c - DeskLib's relocatable memory manager, replaced for RISC OS 64 by a version on malloc: the blocks never move (so an anchor is only changed when its block is
   allocated, resized or freed, and Mem_Compact has nothing to do).  The original (src/Libraries/Mem/Mem.c) builds its own heap above the program's slot with a Norcroft
   hook and keeps pointers in integers; the interface is the same. */
#include <stdlib.h>
#include <string.h>
#include "dl64.h"
#include "DeskLib:Mem.h"

int mem_autocompact = mem_PARTCOMPACT;

typedef struct { size_t size; size_t pad; } mem_hdr;				/* 16 bytes: the data stays 16 byte aligned */

static mem_hdr *hdr (mem_anchor *a) { return a && *a ? (mem_hdr *) *a - 1 : 0; }

BOOL Mem_Initialise (void) { return TRUE; }

BOOL Mem_Alloc (mem_anchor *anchor, int numbytes)
{
  mem_hdr *h = malloc (sizeof (mem_hdr) + (numbytes > 0 ? (size_t) numbytes : 0));
  if (!h) { *anchor = NULL; return FALSE; }
  h->size = numbytes > 0 ? (size_t) numbytes : 0;
  *anchor = h + 1;
  return TRUE;
}

BOOL Mem_MidExtend (mem_anchor *anchor, int at, int by)
{
  mem_hdr *h = hdr (anchor), *n;
  size_t size, newsize;
  char *p;
  if (!h) return FALSE;
  size = h->size;
  if (at < 0 || (size_t) at > size) return FALSE;
  if (by >= 0)
  {
    newsize = size + (size_t) by;
    n = realloc (h, sizeof (mem_hdr) + newsize);
    if (!n) return FALSE;
    p = (char *) (n + 1);
    memmove (p + at + by, p + at, size - (size_t) at);			/* open the gap */
  }
  else
  {
    size_t cut = (size_t) -by;
    if (cut > (size_t) at) return FALSE;
    newsize = size - cut;
    p = (char *) (h + 1);
    memmove (p + at - cut, p + at, size - (size_t) at);			/* delete the bytes below 'at' */
    n = realloc (h, sizeof (mem_hdr) + newsize);
    if (!n) return FALSE;
  }
  n->size = newsize;
  *anchor = n + 1;
  return TRUE;
}

void Mem_MoveAnchor (mem_anchor *from, mem_anchor *to) { *to = *from; *from = NULL; }

void Mem_Free (mem_anchor *anchor)
{
  mem_hdr *h = hdr (anchor);
  if (h) free (h);
  *anchor = NULL;
}

void Mem_Compact (void) { }

int Mem_Size (mem_anchor *anchor) { mem_hdr *h = hdr (anchor); return h ? (int) h->size : 0; }

BOOL Mem_CheckHeap (void) { return TRUE; }
