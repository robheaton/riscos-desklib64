/* sprite.c - DeskLib64 test: the Sprite library on screen. Sprites made in code (a gradient, a disc with a mask, a canvas drawn into with VDU calls through Sprite_Redirect) are plotted in a window's redraw handler: unscaled, scaled, masked, greyscale and as an icon from a user sprite area.  Results of the calls are appended to <Wimp$ScrapDir>.sprtest. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "DeskLib:Core.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Wimp.h"
#include "DeskLib:WimpSWIs.h"
#include "DeskLib:Window.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:ColourTran.h"
#include "DeskLib:GFX.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Error.h"

#define MODE32 0x301680B5u			/* new-format sprite: 32 bits per pixel, 90 x 90 dpi */

/* the header declares Sprite_WriteMask, but the library (as in DeskLib32) defines Sprite_WriteMaskPixel */
extern os_error *Sprite_WriteMaskPixel (void *area, const char *name, const int *pos, int value);

static window_handle win;
static sprite_area area;
static int plots, plot_errors;
static char plot_error_text[256];
static FILE *logf;

static void note (const char *what, os_error *e)
{
  FILE *f = fopen ("<Wimp$ScrapDir>.sprtest", "a");
  if (!f) return;
  fprintf (f, "%s: %s%s\n", what, e ? "ERROR " : "ok", e ? e->errmess : "");
  fclose (f);
}

static void must (const char *what, os_error *e) { note (what, e); }

static void make_sprites (void)
{
  const int size = 64 * 1024;
  screen_modeval mode;
  int x, y, pos[2], n;
  os_error *e;
  sprite_outputstate old;
  void *save;
  wimp_point pt;

  area = (sprite_area) malloc (size);
  area->areasize = size; area->numsprites = 0; area->firstoffset = 16; area->freeoffset = 16;
  must ("InitArea", Sprite_InitArea (area));
  mode.screen_mode = MODE32;

  must ("Create grad", Sprite_Create (area, "grad", 0, 64, 64, mode));
  for (y = 0; y < 64; y++) for (x = 0; x < 64; x++)
  {
    pt.x = x; pt.y = y;
    e = Sprite_WritePixel (area, "grad", &pt, (x * 4) | ((y * 4) << 8) | ((255 - x * 2 - y * 2 > 0 ? 255 - x * 2 - y * 2 : 0) << 16), 0);
    if (e) { note ("WritePixel", e); break; }
  }

  must ("Create disc", Sprite_Create (area, "disc", 0, 48, 48, mode));
  must ("CreateMask disc", Sprite_CreateMask (area, "disc"));
  for (y = 0; y < 48; y++) for (x = 0; x < 48; x++)
  {
    int dx = x - 24, dy = y - 24, inside = dx * dx + dy * dy < 22 * 22;
    pos[0] = x; pos[1] = y;
    pt.x = x; pt.y = y;
    Sprite_WriteMaskPixel (area, "disc", pos, inside ? 1 : 0);
    Sprite_WritePixel (area, "disc", &pt, inside ? 0x000000FF : 0x00FF0000, 0);		/* red inside; blue outside (hidden by the mask) */
  }

  must ("Create canvas", Sprite_Create (area, "canvas", 0, 64, 64, mode));
  must ("ReadSaveAreaSize", Sprite_ReadSaveAreaSize (area, "canvas", &n));
  save = calloc (1, n + 16);
  must ("Redirect canvas", Sprite_Redirect (area, "canvas", save, &old));
  ColourTrans_SetGCOL (0xFFFFFF00, 0x100, 0);		/* white (0xBBGGRR00) */
  GFX_Rectangle (0, 0, 64 * 2, 64 * 2);
  ColourTrans_SetGCOL (0x00FF0000, 0x100, 0);		/* blue */
  GFX_Rectangle (8, 8, 40, 40);
  ColourTrans_SetGCOL (0x0000FF00, 0x100, 0);		/* red */
  GFX_Rectangle (56, 56, 40, 40);
  must ("UnRedirect", Sprite_UnRedirect (&old));
  free (save);
  pt.x = 20; pt.y = 20;
  {
    int colour = 0, tint = 0;
    Sprite_ReadPixel (area, "canvas", &pt, &colour, &tint);
    {
      FILE *f = fopen ("<Wimp$ScrapDir>.sprtest", "a");
      if (f) { fprintf (f, "canvas pixel (20,20) after drawing with VDU calls into it = %06x\n", (unsigned) colour & 0xFFFFFF); fclose (f); }
    }
  }
}

static void draw (int ox, int oy)
{
  wimp_point p;
  sprite_scalefactors sc;
  os_error *e;
  p.x = ox + 20; p.y = oy - 130;
  e = Sprite_Plot (area, "grad", p.x, p.y, 0);			/* OS_SpriteOp 34 */
  plots++; if (e) { if (!plot_errors++) snprintf (plot_error_text, sizeof plot_error_text, "plot %d: %.200s", plots, e->errmess); }
  p.x = ox + 120; p.y = oy - 260;
  sc.multiply.x = 2; sc.multiply.y = 2; sc.divide.x = 1; sc.divide.y = 1;
  e = Sprite_PlotScaled (area, "grad", &p, 0, &sc, NULL);	/* OS_SpriteOp 52: twice the size */
  plots++; if (e) { if (!plot_errors++) snprintf (plot_error_text, sizeof plot_error_text, "plot %d: %.200s", plots, e->errmess); }
  p.x = ox + 270; p.y = oy - 110;
  e = Sprite_PlotMask (area, "disc", &p);			/* OS_SpriteOp 49: the mask as a solid shape */
  plots++; if (e) { if (!plot_errors++) snprintf (plot_error_text, sizeof plot_error_text, "plot %d: %.200s", plots, e->errmess); }
  p.x = ox + 340; p.y = oy - 110;
  sc.multiply.x = 1; sc.multiply.y = 1; sc.divide.x = 1; sc.divide.y = 1;
  e = Sprite_PlotGrey (area, "grad", &p, &sc, NULL);		/* OS_SpriteOp 53: greyscale (not every SpriteExtend version supports it: an error here is not checked) */
  plots++; if (e) { if (!plot_errors++) snprintf (plot_error_text, sizeof plot_error_text, "plot %d: %.200s", plots, e->errmess); }
  p.x = ox + 440; p.y = oy - 130;
  e = Sprite_Plot (area, "canvas", p.x, p.y, 0);
  plots++; if (e) { if (!plot_errors++) snprintf (plot_error_text, sizeof plot_error_text, "plot %d: %.200s", plots, e->errmess); }
}

static BOOL redraw_handler (event_pollblock *event, void *reference)
{
  window_redrawblock rb;
  BOOL more;
  (void) reference;
  rb.window = event->data.openblock.window;
  Wimp_RedrawWindow (&rb, &more);
  while (more)
  {
    draw (rb.rect.min.x - rb.scroll.x, rb.rect.max.y - rb.scroll.y);
    Wimp_GetRectangle (&rb, &more);
  }
  if (logf == NULL)
  {
    FILE *f = fopen ("<Wimp$ScrapDir>.sprtest", "a");
    if (f) { fprintf (f, "first redraw done: %d plots, %d errors %s\n", plots, plot_errors, plot_error_text); fclose (f); }
    logf = (FILE *) 1;
  }
  return TRUE;
}

static void quit (void) { Event_CloseDown (); exit (0); }
static BOOL close_handler (event_pollblock *event, void *reference) { (void) event; (void) reference; quit (); return TRUE; }

int main (void)
{
  icon_createblock ic;
  icon_handle ih;
  os_error *e;
  FILE *f = fopen ("<Wimp$ScrapDir>.sprtest", "w");
  if (f) fclose (f);
  Event_Initialise ("DLSprite");
  EventMsg_Initialise ();
  Resource_Initialise ("DLSprite");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  make_sprites ();
  win = Window_Create ("main", 20);
  memset (&ic, 0, sizeof ic);					/* an icon whose sprite comes from our own sprite area */
  ic.window = win;
  ic.icondata.workarearect.min.x = 560; ic.icondata.workarearect.min.y = -140; ic.icondata.workarearect.max.x = 680; ic.icondata.workarearect.max.y = -60;
  ic.icondata.flags.value = 0x0000011Au;			/* sprite, indirected, centred */
  {
    static char name[16];
    strcpy (name, "disc");
    ic.icondata.data.indirectsprite.name = DL_PTR (name);
    ic.icondata.data.indirectsprite.spritearea = DL_PTR (area);
    ic.icondata.data.indirectsprite.nameisname = 12;
  }
  e = Wimp_CreateIcon (&ic, &ih);
  note ("Wimp_CreateIcon with a user sprite area", e);
  Event_Claim (event_REDRAW, win, event_ANY, redraw_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_CENTERED);
  for (;;) Event_Poll ();
  return 0;
}
