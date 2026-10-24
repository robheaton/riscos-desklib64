/* sprite2test.c - DeskLib64 test, command line: the rest of the Sprite library that does not draw: the pointer forms (SelectP, ReadInfoP, ReadPixelP, WritePixelP, CopyP, RenameP, FlipXP, FlipYP, CreateMaskP, RemoveMaskP, DeleteP), rows and columns, Append, palettes, MemorySize, Get (a part of the screen), ScreenSave, Merge, LoadFiles and the pointer shape.  Prints one line per check and a summary; the exit status is the number of failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DeskLib:Core.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:Screen.h"
#include "DeskLib:KernelSWIs.h"
#include "DeskLib:SWI.h"

static int fails, checks;
static void check (int ok, const char *what) { checks++; if (!ok) fails++; printf ("%s %s\n", ok ? "PASS" : "FAIL", what); }
static void checke (os_error *e, const char *what)
{
  checks++;
  if (e) { fails++; printf ("FAIL %s: error %d %s\n", what, e->errnum, e->errmess); } else printf ("PASS %s\n", what);
}

extern BOOL Sprite_ReadMaskPixelP (sprite_area area, const sprite_header *sprite, const wimp_point *pos);
extern os_error *Sprite_WriteMaskPixelP (sprite_area area, const sprite_header *sprite, const wimp_point *pos, BOOL solid);

#define MODE32 0x301680B5u			/* 32 bits per pixel, 90 x 90 dpi */
#define MODE8  0x201680B5u			/* 8 bits per pixel */

static sprite_area newarea (int size)
{
  sprite_area a = (sprite_area) malloc (size);
  a->areasize = size; a->numsprites = 0; a->firstoffset = 16; a->freeoffset = 16;
  Sprite_InitArea (a);
  return a;
}

static unsigned pixel (sprite_area a, const char *name, int x, int y)
{
  wimp_point p; int c = 0, t = 0;
  p.x = x; p.y = y;
  Sprite_ReadPixel (a, name, &p, &c, &t);
  return (unsigned) c & 0xFFFFFF;
}

int main (void)
{
  sprite_area a = newarea (64 * 1024), b;
  screen_modeval m32, m8;
  sprite_header *sp = NULL, *sq = NULL;
  sprite_info info;
  wimp_point p;
  int colour = 0, tint = 0, n;
  os_error *e;
  setvbuf (stdout, NULL, _IONBF, 0);
  m32.screen_mode = MODE32; m8.screen_mode = MODE8;

  /* ---- the pointer forms ---- */
  checke (Sprite_Create (a, "pa", 0, 8, 4, m32), "Sprite_Create pa 8x4");
  checke (Sprite_Select (a, "pa", &sp), "Sprite_Select (to get the header pointer)");
  check (sp != NULL && (char *) sp >= (char *) a && (char *) sp < (char *) a + a->areasize, "the pointer is inside the area");
  checke (Sprite_SelectP (a, sp), "Sprite_SelectP");
  checke (Sprite_ReadInfoP (a, sp, &info), "Sprite_ReadInfoP");
  check (info.width == 8 && info.height == 4 && info.mode.screen_mode == MODE32, "ReadInfoP: 8 x 4, the mode word");
  p.x = 3; p.y = 2;
  checke (Sprite_WritePixelP (a, sp, &p, 0x00AA5511, 0), "Sprite_WritePixelP");
  checke (Sprite_ReadPixelP (a, sp, &p, &colour, &tint), "Sprite_ReadPixelP");
  check (((unsigned) colour & 0xFFFFFF) == 0xAA5511, "the pixel round trip");
  check (pixel (a, "pa", 3, 2) == 0xAA5511, "the named read agrees");
  checke (Sprite_CopyP (a, sp, "pb"), "Sprite_CopyP pa pb");
  check (a->numsprites == 2, "two sprites");
  checke (Sprite_Select (a, "pb", &sq), "Sprite_Select pb");
  checke (Sprite_RenameP (a, sq, "pc"), "Sprite_RenameP pb pc");
  checke (Sprite_Select (a, "pc", &sq), "Sprite_Select pc");
  checke (Sprite_FlipXP (a, sq), "Sprite_FlipXP (upside down)");
  check (pixel (a, "pc", 3, 1) == 0xAA5511, "the pixel moved from row 2 to row 1");
  checke (Sprite_FlipYP (a, sq), "Sprite_FlipYP (mirrored)");
  check (pixel (a, "pc", 4, 1) == 0xAA5511, "the pixel moved from column 3 to column 4");
  checke (Sprite_CreateMaskP (a, sq), "Sprite_CreateMaskP");
  Sprite_ReadInfoP (a, sq, &info);
  check (info.maskstatus != 0, "it has a mask");
  p.x = 4; p.y = 1;
  check (Sprite_ReadMaskPixelP (a, sq, &p) != 0, "the mask bit is set (ReadMaskPixelP)");
  checke (Sprite_WriteMaskPixelP (a, sq, &p, FALSE), "Sprite_WriteMaskPixelP clear");
  check (Sprite_ReadMaskPixelP (a, sq, &p) == 0, "the mask bit is clear");
  checke (Sprite_RemoveMaskP (a, sq), "Sprite_RemoveMaskP");
  Sprite_ReadInfoP (a, sq, &info);
  check (info.maskstatus == 0, "no mask");
  checke (Sprite_DeleteP (a, sq), "Sprite_DeleteP pc");
  check (a->numsprites == 1, "one sprite left");

  /* ---- rows, columns and append ---- */
  checke (Sprite_Select (a, "pa", &sp), "Select pa");
  checke (Sprite_InsertRow (a, "pa", 1), "Sprite_InsertRow at 1");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.width == 8 && info.height == 5, "height 5");
  checke (Sprite_Select (a, "pa", &sp), "Select pa (it may have moved)");
  checke (Sprite_InsertRowP (a, sp, 0), "Sprite_InsertRowP at 0");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.height == 6, "height 6");
  checke (Sprite_DeleteRow (a, "pa", 0), "Sprite_DeleteRow");
  Sprite_Select (a, "pa", &sp);
  checke (Sprite_DeleteRowP (a, sp, 0), "Sprite_DeleteRowP");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.height == 4, "height 4 again");
  checke (Sprite_InsertColumn (a, "pa", 2), "Sprite_InsertColumn");
  Sprite_Select (a, "pa", &sp);
  checke (Sprite_InsertColumnP (a, sp, 2), "Sprite_InsertColumnP");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.width == 10 && info.height == 4, "width 10");
  checke (Sprite_DeleteColumn (a, "pa", 2), "Sprite_DeleteColumn");
  Sprite_Select (a, "pa", &sp);
  checke (Sprite_DeleteColumnP (a, sp, 2), "Sprite_DeleteColumnP");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.width == 8 && info.height == 4, "back to 8 x 4");
  check (pixel (a, "pa", 3, 2) == 0xAA5511, "the pixel is still where it was");
  checke (Sprite_Create (a, "pd", 0, 4, 4, m32), "Create pd 4 x 4");
  checke (Sprite_Append (a, "pa", "pd", sprite_APPENDHORIZ), "Sprite_Append horizontally");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.width == 12 && info.height == 4 && a->numsprites == 1, "pa is 12 x 4 and pd is gone");
  checke (Sprite_Create (a, "pe", 0, 12, 2, m32), "Create pe 12 x 2");
  checke (Sprite_Append (a, "pa", "pe", sprite_APPENDVERT), "Sprite_Append vertically");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.width == 12 && info.height == 6 && a->numsprites == 1, "pa is 12 x 6");
  checke (Sprite_Create (a, "pf", 0, 3, 6, m32), "Create pf 3 x 6");
  Sprite_Select (a, "pa", &sp);
  checke (Sprite_AppendP (a, sp, "pf", sprite_APPENDHORIZ), "Sprite_AppendP horizontally");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.width == 15 && info.height == 6 && a->numsprites == 1, "pa is 15 x 6");
  checke (Sprite_Create (a, "pg", 0, 15, 1, m32), "Create pg 15 x 1");
  Sprite_Select (a, "pa", &sp);
  checke (Sprite_AppendP (a, sp, "pg", sprite_APPENDVERT), "Sprite_AppendP vertically");
  Sprite_ReadInfo (a, "pa", &info);
  check (info.width == 15 && info.height == 7 && a->numsprites == 1, "pa is 15 x 7");

  /* ---- palettes (8 bits per pixel) ---- */
  checke (Sprite_Create (a, "pal", 0, 8, 4, m8), "Create pal 8x4 8bpp, no palette");
  n = Sprite_ReadPaletteSize (a, "pal");
  check (n == 0, "no palette");
  e = SWI (4, 0, SWI_OS_SpriteOp | 0x20000, 256 + 37, a, "pal", 0x80000000u);	/* try it directly first, to see whether the OS allows it at this display depth */
  if (e) printf ("SKIP the palette functions: OS_SpriteOp 37 gives error %d: %s\n", e->errnum, e->errmess);
  else
  {
    Sprite_RemovePalette (a, "pal");
    n = Sprite_CreatePalette (a, "pal", TRUE);
    printf ("     CreatePalette returned %d\n", n);
    Sprite_Select (a, "pal", &sp);
    n = Sprite_ReadPaletteSize (a, "pal");
    printf ("     palette size now: %d\n", n);
    check (n > 0, "a palette");
    check (Sprite_ReadPaletteSizeP (a, sp) == n, "ReadPaletteSizeP agrees");
    checke (Sprite_RemovePalette (a, "pal"), "Sprite_RemovePalette");
    check (Sprite_ReadPaletteSize (a, "pal") == 0, "no palette again");
    Sprite_Select (a, "pal", &sp);
    Sprite_CreatePaletteP (a, sp, TRUE);
    Sprite_Select (a, "pal", &sp);
    check (Sprite_ReadPaletteSizeP (a, sp) > 0, "CreatePaletteP made one");
    checke (Sprite_RemovePaletteP (a, sp), "Sprite_RemovePaletteP");
  }

  /* ---- sizes ---- */
  n = (int) Sprite_MemorySize (8, 4, m32, (spritemem_flags) sprite_DEEPFORMAT);
  printf ("     MemorySize 8x4 32bpp: %d\n", n);
  check (n == 44 + 8 * 4 * 4, "MemorySize: 44 byte header and 8 x 4 words");
  n = (int) Sprite_MemorySizeBpp (8, 4, 8, (spritemem_flags) sprite_DEEPFORMAT);
  printf ("     MemorySizeBpp 8x4 8bpp: %d\n", n);
  check (n == 44 + 8 * 4, "MemorySizeBpp: 8 bytes per row");

  /* ---- Get a part of the screen, ScreenSave ---- */
  b = newarea (128 * 1024);
  Screen_CacheModeInfo ();
  sp = NULL;
  e = Sprite_Get (b, "grab", 0, 0, 0, 63, 63, &sp);
  checke (e, "Sprite_Get (the bottom left of the screen)");
  check (sp != NULL && (char *) sp >= (char *) b && (char *) sp < (char *) b + b->areasize, "the pointer is inside the area (no stale top half)");
  if (sp && !e) { printf ("     got %s, width word %d, height %d\n", sp->name, sp->width, sp->height); check (strncmp (sp->name, "grab", 4) == 0 && b->numsprites == 1, "named grab, one sprite"); }
  e = Sprite_Get (b, "grab", 0, 0, 0, 31, 31, &sq);
  checke (e, "Sprite_Get again (replaces grab)");
  check (b->numsprites == 1 && sq != NULL, "still one sprite");
  checke (Sprite_ScreenSave ("<Wimp$ScrapDir>.dlscreen", FALSE), "Sprite_ScreenSave");
  {
    FILE *f = fopen ("<Wimp$ScrapDir>.dlscreen", "rb");
    long len = 0;
    if (f) { fseek (f, 0, SEEK_END); len = ftell (f); fclose (f); }
    printf ("     the screen file is %ld bytes\n", len);
    check (len > 100000, "the file has the screen in it");
    remove ("<Wimp$ScrapDir>.dlscreen");
  }

  /* ---- files: Merge, LoadFiles ---- */
  {
    sprite_area s1 = newarea (4096), s2 = newarea (4096), c;
    Sprite_Create (s1, "f1", 0, 4, 4, m32); Sprite_Create (s2, "f2", 0, 6, 2, m32);
    checke (Sprite_Save (s1, "<Wimp$ScrapDir>.dlf1"), "save file 1");
    checke (Sprite_Save (s2, "<Wimp$ScrapDir>.dlf2"), "save file 2");
    c = newarea (8192);
    checke (Sprite_Merge (c, "<Wimp$ScrapDir>.dlf1"), "Sprite_Merge file 1");
    checke (Sprite_Merge (c, "<Wimp$ScrapDir>.dlf2"), "Sprite_Merge file 2");
    check (c->numsprites == 2, "two sprites in the area");
    Sprite_ReadInfo (c, "f2", &info);
    check (info.width == 6 && info.height == 2, "f2 is 6 x 2");
    c = Sprite_LoadFiles (2, "<Wimp$ScrapDir>.dlf1", "<Wimp$ScrapDir>.dlf2");
    check (c != NULL && c->numsprites == 2, "Sprite_LoadFiles loads both files into one area");
    remove ("<Wimp$ScrapDir>.dlf1"); remove ("<Wimp$ScrapDir>.dlf2");
  }

  /* ---- the pointer shape ---- */
  {
    sprite_area pa = newarea (4096);
    wimp_point act = { 0, 0 };
    int r1, r2;
    Sprite_Create (pa, "ptr", 0, 16, 16, m32);
    Sprite_Select (pa, "ptr", &sp);
    e = Sprite_SetPointerShape (pa, "ptr", 2, &act, NULL, NULL);
    if (e && e->errnum == 1814) printf ("SKIP the pointer shape functions: error %d: %s\n", e->errnum, e->errmess);
    else
    {
      checke (e, "Sprite_SetPointerShape (shape 2)");
      e = Sprite_SetPointerShapeP (pa, sp, 2, &act, NULL, NULL);
      checke (e, "Sprite_SetPointerShapeP");
    }
    OS_Byte (osbyte_SELECTPOINTER, 1, 0, &r1, &r2);		/* back to the normal pointer */
  }

  printf ("SUMMARY %d failed of %d\n", fails, checks);
  return fails;
}
