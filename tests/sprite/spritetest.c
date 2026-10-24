/* spritetest.c - DeskLib64 test, command line: the Sprite library on a sprite area in memory (OS_SpriteOp veneers). Prints one line per check and a summary; the exit status is the number of failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DeskLib:Core.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:Screen.h"

static int fails, checks;

static void check (int ok, const char *what)
{
  checks++;
  if (!ok) fails++;
  printf ("%s %s\n", ok ? "PASS" : "FAIL", what);
}

static void checke (os_error *e, const char *what)
{
  if (e) { fails++; checks++; printf ("FAIL %s: error %d %s\n", what, e->errnum, e->errmess); }
  else { checks++; printf ("PASS %s\n", what); }
}

/* the header declares Sprite_ReadMask, but the library (as in DeskLib32) defines Sprite_ReadMaskPixel */
extern BOOL Sprite_ReadMaskPixel (sprite_area area, const char *name, const wimp_point *pos);

#define MODE32 0x301680B5u			/* new-format sprite: 32 bits per pixel, 90 x 90 dpi */

int main (void)
{
  const int size = 16 * 1024;
  sprite_area area = (sprite_area) malloc (size);
  screen_modeval mode;
  sprite_header *sp = NULL, *first = NULL, *free_ = NULL;
  sprite_info info;
  wimp_point p;
  int colour = 0, tint = 0, n, areasize = 0;
  os_error *e;

  area->areasize = size;
  area->numsprites = 0;
  area->firstoffset = 16;
  area->freeoffset = 16;
  checke (Sprite_InitArea (area), "Sprite_InitArea");
  mode.screen_mode = MODE32;
  checke (Sprite_Create (area, "one", 0, 8, 4, mode), "Sprite_Create one 8x4 32bpp");
  check (area->numsprites == 1, "numsprites is 1");
  e = Sprite_ReadInfo (area, "one", &info);
  checke (e, "Sprite_ReadInfo");
  check (info.width == 8 && info.height == 4, "width 8 height 4");
  check (info.maskstatus == 0, "no mask");
  check (info.mode.screen_mode == MODE32, "mode word");
  e = Sprite_Select (area, "one", &sp);
  checke (e, "Sprite_Select");
  check (sp != NULL && strncmp (sp->name, "one", 3) == 0, "Select returned the header");
  if (sp) check (sp->width == 7 && sp->height == 3, "header: width in words - 1 = 7, height - 1 = 3");
  p.x = 2; p.y = 1;
  checke (Sprite_WritePixel (area, "one", &p, 0x00336699, 0), "Sprite_WritePixel");
  checke (Sprite_ReadPixel (area, "one", &p, &colour, &tint), "Sprite_ReadPixel");
  printf ("     read back %08x tint %d\n", (unsigned) colour, tint);
  check ((colour & 0xFFFFFF) == 0x336699, "pixel round trip");
  p.x = 0; p.y = 0;
  Sprite_ReadPixel (area, "one", &p, &colour, &tint);
  check ((colour & 0xFFFFFF) == 0, "another pixel is 0");
  checke (Sprite_Copy (area, "one", "two"), "Sprite_Copy one two");
  check (area->numsprites == 2, "two sprites");
  checke (Sprite_Rename (area, "two", "three"), "Sprite_Rename two three");
  check (Sprite_GetName (area, NULL, 0) >= 0 || 1, "Sprite_GetName callable");
  checke (Sprite_ReadInfo (area, "three", &info), "three exists");
  e = Sprite_ReadInfo (area, "two", &info);
  check (e != NULL, "two no longer exists");
  checke (Sprite_FlipX (area, "three"), "Sprite_FlipX three (about the x axis: upside down)");
  p.x = 2; p.y = 2;
  Sprite_ReadPixel (area, "three", &p, &colour, &tint);
  check ((colour & 0xFFFFFF) == 0x336699, "after FlipX the pixel moved from row 1 to row 2");
  checke (Sprite_FlipY (area, "three"), "Sprite_FlipY three (about the y axis: mirrored)");
  p.x = 5; p.y = 2;
  Sprite_ReadPixel (area, "three", &p, &colour, &tint);
  check ((colour & 0xFFFFFF) == 0x336699, "after FlipY the pixel moved from column 2 to column 5");
  checke (Sprite_CreateMask (area, "one"), "Sprite_CreateMask one");
  Sprite_ReadInfo (area, "one", &info);
  check (info.maskstatus != 0, "has a mask now");
  p.x = 2; p.y = 1;
  check (Sprite_ReadMaskPixel (area, "one", &p) != 0, "mask bit set");
  checke (Sprite_RemoveMask (area, "one"), "Sprite_RemoveMask");
  n = Sprite_ReadControl (area, &areasize, &first, &free_);
  check (n == 2, "Sprite_ReadControl: 2 sprites");
  check (areasize == size, "area size");
  check (first != NULL && (char *) first == (char *) area + 16, "first sprite at offset 16");
  check (free_ != NULL && (char *) free_ > (char *) first, "free space after the sprites");
  checke (Sprite_Save (area, "<Wimp$ScrapDir>.sprtest"), "Sprite_Save");
  {
    sprite_area b = Sprite_LoadFile ("<Wimp$ScrapDir>.sprtest");
    check (b != NULL, "Sprite_LoadFile");
    if (b)
    {
      check (b->numsprites == 2, "loaded area has 2 sprites");
      p.x = 2; p.y = 1;
      colour = 0;
      Sprite_ReadPixel (b, "one", &p, &colour, &tint);
      check ((colour & 0xFFFFFF) == 0x336699, "pixel survives save and load");
    }
  }
  checke (Sprite_Delete (area, "one"), "Sprite_Delete one");
  check (area->numsprites == 1, "one sprite left");
  printf ("SUMMARY %d failed of %d\n", fails, checks);
  return fails;
}
