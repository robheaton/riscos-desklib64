/* cleartest.c - DeskLib64 test, command line: the Clear library (Clear picture files) and the Mem library it uses.  It writes a file in the directory given as the first argument (default <Wimp$ScrapDir>) and deletes it.  Prints one line per check and a summary; the exit status is the number of failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DeskLib:Core.h"
#include "DeskLib:Mem.h"
#include "DeskLib:Clear.h"
#include "DeskLib:File.h"

static int fails, checks;
static void check (int ok, const char *what) { checks++; if (!ok) fails++; printf ("%s %s\n", ok ? "PASS" : "FAIL", what); }

int main (int argc, char **argv)
{
  char path[300];
  clear_picture *p, *q;
  unsigned i;
  mem_anchor a = NULL;

  snprintf (path, sizeof path, "%s.cleartst", argc > 1 ? argv[1] : "<Wimp$ScrapDir>");

  /* Mem: the Clear library allocates with it */
  check (Mem_Initialise () != 0, "Mem_Initialise");
  check (Mem_Alloc (&a, 100) != 0 && a != NULL, "Mem_Alloc 100 bytes");
  if (a) { memset (a, 0x5A, 100); check (((unsigned char *) a)[99] == 0x5A, "the block is writable"); }
  check (Mem_MidExtend (&a, 50, 50) != 0, "Mem_MidExtend by 50 bytes");
  Mem_Free (&a);
  check (a == NULL, "Mem_Free clears the anchor");

  Clear_Init ("cleartest", 123);
  p = Clear_Create (8, 4, 8);
  check (p != NULL, "Clear_Create 8 x 4 at 8 bits per pixel");
  if (!p) { printf ("SUMMARY %d failed of %d\n", fails + 1, checks); return 1; }
  check (p->width == 8 && p->height == 4 && p->bpp == 8, "width, height and bpp are set");
  check (p->palette != NULL && p->bitmap != NULL, "a palette and a bitmap exist");
  for (i = 0; i < 256; i++) { p->palette[i].data.red = (char) i; p->palette[i].data.green = (char) (255 - i); p->palette[i].data.blue = (char) (i ^ 0x55); }
  for (i = 0; i < 32; i++) p->bitmap[i] = (unsigned char) (i * 7);

  Clear_Save (p, path);
  check (File_Exists (path), "Clear_Save made the file");
  printf ("     file type after Clear_Save: %03x (&690 is Clear)\n", File_GetType (path));
  /* Clear_Save sets the type while the file is open; whether that survives the close depends on the file system, so it is shown but not checked */
  check (File_Size (path) == (int) (strlen ("cleartest") + 1 + 16 + 3 * 256 + 32), "its size is right");
  Clear_Free (p);

  q = Clear_Load (path);
  check (q != NULL, "Clear_Load");
  if (q)
  {
    check (q->width == 8 && q->height == 4 && q->bpp == 8, "the loaded picture has the same size");
    check (q->creator != NULL && strcmp (q->creator, "cleartest") == 0 && q->creatorversion == 123, "the creator and version were read");
    for (i = 0; i < 256; i++)
      if ((unsigned char) q->palette[i].data.red != i || (unsigned char) q->palette[i].data.green != 255 - i || (unsigned char) q->palette[i].data.blue != (i ^ 0x55)) break;
    check (i == 256, "all 256 palette entries match");
    for (i = 0; i < 32; i++) if (q->bitmap[i] != (unsigned char) (i * 7)) break;
    check (i == 32, "all 32 bitmap bytes match");
    Clear_Free (q);
  }
  check (Clear_Load ("<Wimp$ScrapDir>.nosuchfile") == NULL, "loading a missing file gives NULL");

  p = Clear_Create (4, 2, 24);
  check (p != NULL, "Clear_Create at 24 bits per pixel");
  if (p)
  {
    for (i = 0; i < 24; i++) p->bitmap[i] = (unsigned char) (i + 1);
    Clear_Save (p, path);
    Clear_Free (p);
    q = Clear_Load (path);
    check (q != NULL && q->bpp == 24 && q->width == 4 && q->height == 2, "a 24 bit picture loads");
    if (q) { for (i = 0; i < 24; i++) if (q->bitmap[i] != (unsigned char) (i + 1)) break; check (i == 24, "its 24 bytes match"); Clear_Free (q); }
  }
  File_Delete (path);
  check (!File_Exists (path), "the file is deleted");

  printf ("SUMMARY %d failed of %d\n", fails, checks);
  return fails;
}
