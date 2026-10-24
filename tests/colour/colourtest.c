/* colourtest.c - DeskLib64 test, a desktop task that runs once and quits (VDU output into a sprite does not work as expected in a Terminal window): ColourTrans (SetGCOL, ReturnColourNumberForMode, SelectTable, InvalidateCache, the font colour calls) and GFX drawing into a sprite with Sprite_Redirect.  Appends one line per check and a summary to <Wimp$ScrapDir>.colourtest, and shows an error box with the summary. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "DeskLib:Core.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:ColourTran.h"
#include "DeskLib:GFX.h"
#include "DeskLib:Event.h"
#include "DeskLib:Error.h"

static int fails, checks;
static FILE *logf;
#define printf logprintf
static void logprintf (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt); vfprintf (logf, fmt, ap); va_end (ap);
  fflush (logf);
}
static void check (int ok, const char *what) { checks++; if (!ok) fails++; printf ("%s %s\n", ok ? "PASS" : "FAIL", what); }
static void checke (os_error *e, const char *what)
{
  checks++;
  if (e) { fails++; printf ("FAIL %s: error %d %s\n", what, e->errnum, e->errmess); } else printf ("PASS %s\n", what);
}

#define MODE32 0x301680B5u			/* new-format sprite: 32 bits per pixel, 90 x 90 dpi (2 OS units per pixel) */

/* the colour in a 32 bpp sprite pixel, as 0xBBGGRR (the order of a RISC OS palette entry without its low byte) */
static unsigned pixel (sprite_area area, const char *name, int x, int y)
{
  wimp_point p;
  int colour = 0, tint = 0;
  p.x = x; p.y = y;
  Sprite_ReadPixel (area, name, &p, &colour, &tint);
  return (unsigned) colour & 0xFFFFFF;
}

int main (void)
{
  logf = fopen ("<Wimp$ScrapDir>.colourtest", "w");
  if (!logf) return 1;
  Event_Initialise ("DLColour");
  const int size = 64 * 1024;
  sprite_area area = (sprite_area) malloc (size);
  screen_modeval mode;
  sprite_outputstate old;
  void *save;
  int n, c1, c2, c3;
  os_error *e;
  colourtrans_fontblock fb;
  static char table[1024];

  area->areasize = size; area->numsprites = 0; area->firstoffset = 16; area->freeoffset = 16;
  checke (Sprite_InitArea (area), "Sprite_InitArea");
  mode.screen_mode = MODE32;
  checke (Sprite_Create (area, "canvas", 0, 32, 32, mode), "a 32 x 32 sprite, 32 bits per pixel");

  checke (ColourTrans_InvalidateCache (), "ColourTrans_InvalidateCache");

  /* the colour numbers in the current mode */
  c1 = ColourTrans_ReturnColourNumberForMode (0xFFFFFF00, -1, (int *) -1);
  c2 = ColourTrans_ReturnColourNumberForMode (0x00000000, -1, (int *) -1);
  c3 = ColourTrans_ReturnColourNumberForMode (0x0000FF00, -1, (int *) -1);
  printf ("     white %08x black %08x red %08x\n", (unsigned) c1, (unsigned) c2, (unsigned) c3);
  check (c1 != -1 && c2 != -1 && c3 != -1, "colour numbers for white, black and red (no error)");
  check (c1 != c2 && c1 != c3 && c2 != c3, "they are all different");
  c1 = ColourTrans_ReturnColourNumberForMode (0xFFFFFF00, MODE32, (int *) -1);
  printf ("     white in the 32 bpp sprite mode %08x\n", (unsigned) c1);
  check (c1 != -1, "a colour number for the sprite mode");

  /* draw into the sprite: set the colour with SetGCOL, fill rectangles, read the pixels */
  n = 0;
  checke (Sprite_ReadSaveAreaSize (area, "canvas", &n), "Sprite_ReadSaveAreaSize");
  save = calloc (1, n + 16);
  checke (Sprite_Redirect (area, "canvas", save, &old), "Sprite_Redirect");
  checke (ColourTrans_SetGCOL (0xFFFFFF00, 0x100, 0), "SetGCOL white (&BBGGRR00)");
  GFX_RectangleFill (0, 0, 64, 64);
  checke (ColourTrans_SetGCOL (0x0000FF00, 0x100, 0), "SetGCOL red");
  GFX_RectangleFill (4, 4, 20, 20);
  checke (ColourTrans_SetGCOL (0xFF000000, 0x100, 0), "SetGCOL blue");
  GFX_RectangleFill (36, 36, 20, 20);
  checke (ColourTrans_SetGCOL (0x00FF0000, 0x100, 0), "SetGCOL green");
  GFX_RectangleFill (36, 4, 20, 20);
  checke (Sprite_UnRedirect (&old), "Sprite_UnRedirect");
  free (save);
  printf ("     (a sprite pixel reads as 0xBBGGRR) pixels: background %06x red %06x blue %06x green %06x\n", pixel (area, "canvas", 15, 15), pixel (area, "canvas", 5, 5), pixel (area, "canvas", 24, 24), pixel (area, "canvas", 24, 5));
  check (pixel (area, "canvas", 15, 15) == 0xFFFFFF, "the background is white");
  check (pixel (area, "canvas", 5, 5) == 0x0000FF, "the first square is red (0xBBGGRR: 0000ff)");
  check (pixel (area, "canvas", 24, 24) == 0xFF0000, "the second square is blue (ff0000)");
  check (pixel (area, "canvas", 24, 5) == 0x00FF00, "the third square is green");

  /* a translation table between two modes */
  memset (table, 0xEE, sizeof table);
  e = ColourTrans_SelectTable (-1, (int *) -1, -1, (int *) -1, table);
  checke (e, "ColourTrans_SelectTable, current mode to current mode");

  /* the font colours (font 0 = the current font) */
  memset (&fb, 0, sizeof fb);
  fb.font = 0; fb.back = (int) 0xFFFFFF00; fb.fore = 0; fb.max = 14;
  e = ColourTrans_ReturnFontColours (&fb);
  checke (e, "ColourTrans_ReturnFontColours");
  printf ("     back %08x fore %08x max %d\n", (unsigned) fb.back, (unsigned) fb.fore, fb.max);
  check (fb.max >= 0 && fb.max <= 14, "the offset limit is 0 to 14");
  e = ColourTrans_SetFontColours (0, 0xFFFFFF00, 0x00000000, 14);
  checke (e, "ColourTrans_SetFontColours (the current font)");

  printf ("SUMMARY %d failed of %d\n", fails, checks);
  fclose (logf);
  Event_CloseDown ();
  return fails;
}
