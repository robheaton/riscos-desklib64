/* font.c - DeskLib64 test: the Font library (Font Manager veneers), Font2 and the font menu (Menu_FontMenu, Menu_FontMenuDecode).  At start it runs the checks that need no drawing and appends the results to <Wimp$ScrapDir>.fonttest; then it shows a window in which a redraw handler paints text in several fonts with Font_Paint.  A right click opens the font menu; the font chosen is reported in the log and painted in the window. */
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
#include "DeskLib:Menu.h"
#include "DeskLib:Font.h"
#include "DeskLib:Font2.h"
#include "DeskLib:ColourTran.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Error.h"

static FILE *logf;
static int fails, checks;
static window_handle win;
static char chosen[128] = "Homerton.Medium";
static menu_ptr fontmenu;

static void say (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt); vfprintf (logf, fmt, ap); va_end (ap);
  fputc ('\n', logf);
  fflush (logf);
}
static void check (int ok, const char *what) { checks++; if (!ok) fails++; say ("%s %s", ok ? "PASS" : "FAIL", what); }
static void checke (os_error *e, const char *what)
{
  checks++;
  if (e) { fails++; say ("FAIL %s: error %d %s", what, e->errnum, e->errmess); } else say ("PASS %s", what);
}

static void run_checks (void)
{
  /* font_handle is a char, but the OS result is stored as a word by the veneer (as in DeskLib32): each handle gets a word of its own */
  unsigned int hw = 0, h2w = 0, ww = 0;
#define h (*(font_handle *) &hw)
#define h2 (*(font_handle *) &h2w)
#define wimpfont (*(font_handle *) &ww)
  font_defn defn;
  font_info info, sinfo;
  font_string fs;
  font_state st;
  font2_block *f2 = NULL;
  int version = 0, used = 0, cache = 0, count = 0, x, y, x2, y2, xs = 0, ys = 0;
  os_error *e;

  checke (Font_CacheAddr (&version, &used, &cache), "Font_CacheAddr");
  say ("     font manager version %d, cache used %d of %d", version, used, cache);
  check (version >= 100 && cache > 0, "a plausible version and cache size");

  /* Font_ListFonts puts the next font name in the buffer (at least 40 characters) and the number to pass next time in count, or -1 at the end */
  {
    char listed[64];
    int nfonts = 0, seen_homerton = 0, seen_corpus = 0;
    count = 0;
    do
    {
      listed[0] = 0;
      e = Font_ListFonts (listed, &count);
      if (e) break;
      { char *q; for (q = listed; *q >= 32; q++) ; *q = 0; }		/* the name is terminated by a control character (carriage return) */
      if (listed[0]) nfonts++;
      if (strcmp (listed, "Homerton.Medium") == 0) seen_homerton = 1;
      if (strcmp (listed, "Corpus.Medium") == 0) seen_corpus = 1;
      if (nfonts <= 3) say ("     font %d: %s", nfonts, listed);
    } while (count != -1 && nfonts < 500);
    checke (e, "Font_ListFonts (all the fonts)");
    say ("     %d fonts listed", nfonts);
    check (nfonts > 0, "at least one font is listed");
    check (seen_homerton, "Homerton.Medium is in the list");
    check (seen_corpus, "Corpus.Medium is in the list");
  }

  e = Font_GetWimpFont (&wimpfont);
  checke (e, "Font_GetWimpFont");
  say ("     Wimp font handle %d", (int) wimpfont);

  e = Font_FindFont (&h, "Homerton.Medium", 12 * 16, 12 * 16, 0, 0);
  checke (e, "Font_FindFont Homerton.Medium 12pt");
  check (h != 0, "a non-zero handle");
  if (!h) return;
  memset (&defn, 0, sizeof defn);
  checke (Font_ReadDefn (h, &defn), "Font_ReadDefn");
  say ("     name '%.40s' size %u x %u res %u x %u usage %u", defn.name, defn.xsize, defn.ysize, defn.xres, defn.yres, defn.usage);
  check (strncmp (defn.name, "Homerton.Medium", 15) == 0, "the name read back");
  check (defn.xsize == 12 * 16 && defn.ysize == 12 * 16, "the size read back is 12pt (192 sixteenths)");
  checke (Font_ReadInfo (h, &info), "Font_ReadInfo");
  say ("     bounding box %d %d %d %d", info.minx, info.miny, info.maxx, info.maxy);
  check (info.maxx > info.minx && info.maxy > info.miny, "a sensible bounding box");

  checke (Font_SetFont (h), "Font_SetFont");
  memset (&st, 0, sizeof st);
  checke (Font_CurrentFont (&st), "Font_CurrentFont");
  check (st.font == h, "the current font is the one set");

  checke (Font_ReadScaleFactor (&xs, &ys), "Font_ReadScaleFactor");
  say ("     scale factor %d %d", xs, ys);

  /* the width of a string (font_string has a dl_ptr for the text) */
  memset (&fs, 0, sizeof fs);
  fs.s = DL_PTR ("Hello, world");
  fs.x = 1 << 30; fs.y = 1 << 30; fs.split = -1; fs.term = 12;
  checke (Font_StringWidth (&fs), "Font_StringWidth");
  say ("     width %d millipoints, height %d", fs.x, fs.y);
  check (fs.x > 12 * 1000 && fs.x < 200 * 1000, "a plausible width for 12 characters at 12pt (12 to 200 points)");
  x = fs.x;

  memset (&sinfo, 0, sizeof sinfo);
  checke (Font_StringBBox ("Hello, world", &sinfo), "Font_StringBBox");
  say ("     string box %d %d %d %d", sinfo.minx, sinfo.miny, sinfo.maxx, sinfo.maxy);
  check (sinfo.maxx > 0, "a positive right edge");

  checke (Font_ConvertToOS (x, 12000, &x2, &y2), "Font_ConvertToOS");
  check (x2 > 0, "a positive OS width");
  checke (Font_ConvertToPoints (x2, y2, &x, &y), "Font_ConvertToPoints");
  check (x > 0 && y > 0, "and back to millipoints");

  /* a second size of the same font: a different handle */
  e = Font_FindFont (&h2, "Homerton.Medium", 24 * 16, 24 * 16, 0, 0);
  checke (e, "Font_FindFont Homerton.Medium 24pt");
  check (h2 != 0 && h2 != h, "a different handle for another size");
  if (h2) checke (Font_LoseFont (h2), "Font_LoseFont (24pt)");
  checke (Font_LoseFont (h), "Font_LoseFont (12pt)");

  /* Font2 */
  Font2_Initialise ();
  e = Font2_ClaimFont (&f2, "Homerton.Medium", 14 * 16, 14 * 16);
  checke (e, "Font2_ClaimFont");
  check (f2 != NULL && f2->handle != 0, "a font2 block with a handle");
  if (f2) say ("     font2 handle %d, info %d %d %d %d", (int) f2->handle, f2->info.minx, f2->info.miny, f2->info.maxx, f2->info.maxy);
  if (f2) check (f2->info.maxx > f2->info.minx, "its info is filled in");
  e = Font2_ReclaimFont (&f2, "Homerton.Medium", 20 * 16, 20 * 16);
  checke (e, "Font2_ReclaimFont at another size");
  Font2_ReleaseFont (&f2);
  check (f2 == NULL, "Font2_ReleaseFont clears the pointer");

  /* the font menu */
  fontmenu = Menu_FontMenu (TRUE, "Homerton.Medium");
  check (fontmenu != NULL, "Menu_FontMenu made a menu");
}

#undef h
#undef h2
#undef wimpfont

static void paint_line (const char *font, int size, const char *text, int x, int y)
{
  font_handle h = 0;
  if (Font_FindFont (&h, font, size * 16, size * 16, 0, 0)) return;
  ColourTrans_SetFontColours (h, 0xFFFFFF00, 0x00000000, 14);
  Font_Paint (text, font_OSCOORS, x, y);
  Font_LoseFont (h);
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
    int ox = rb.rect.min.x - rb.scroll.x, oy = rb.rect.max.y - rb.scroll.y;
    paint_line ("Homerton.Medium", 12, "Homerton Medium 12pt: Hello, world", ox + 20, oy - 130);
    paint_line ("Homerton.Bold", 18, "Homerton Bold 18pt", ox + 20, oy - 190);
    paint_line ("Trinity.Medium", 14, "Trinity Medium 14pt: The quick brown fox", ox + 20, oy - 250);
    paint_line ("Corpus.Medium", 12, "Corpus Medium 12pt: jumps over the lazy dog", ox + 20, oy - 300);
    paint_line (chosen, 20, chosen, ox + 20, oy - 360);
    Wimp_GetRectangle (&rb, &more);
  }
  return TRUE;
}

static BOOL click_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  if (event->data.mouse.button.data.menu)
  {
    fontmenu = Menu_FontMenu (TRUE, chosen);
    say ("font menu %s", fontmenu ? "made" : "NOT made");
    if (fontmenu) Menu_Show (fontmenu, event->data.mouse.pos.x - 64, event->data.mouse.pos.y);
  }
  return TRUE;
}

static BOOL menu_handler (event_pollblock *event, void *reference)
{
  char *name;
  window_redrawblock rb;
  (void) reference;
  name = Menu_FontMenuDecode (event->data.selection);
  say ("font menu selection %d %d %d decodes to '%s'", event->data.selection[0], event->data.selection[1], event->data.selection[2], name ? name : "(null)");
  if (name) { strncpy (chosen, name, sizeof chosen - 1); chosen[sizeof chosen - 1] = 0; }
  rb.window = win; rb.rect.min.x = 0; rb.rect.min.y = -1000; rb.rect.max.x = 2000; rb.rect.max.y = 0;
  Wimp_ForceRedraw ((window_redrawblock *) &rb);
  return TRUE;
}

static void quit (void) { Event_CloseDown (); exit (0); }
static BOOL close_handler (event_pollblock *event, void *reference) { (void) event; (void) reference; quit (); return TRUE; }

int main (void)
{
  logf = fopen ("<Wimp$ScrapDir>.fonttest", "w");
  if (!logf) return 1;
  Event_Initialise ("DLFont");
  EventMsg_Initialise ();
  Resource_Initialise ("DLFont");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  run_checks ();
  say ("SUMMARY %d failed of %d", fails, checks);
  win = Window_Create ("main", 20);
  Event_Claim (event_REDRAW, win, event_ANY, redraw_handler, NULL);
  Event_Claim (event_CLICK, win, event_ANY, click_handler, NULL);
  Event_Claim (event_MENU, event_ANY, event_ANY, menu_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_CENTERED);
  for (;;) Event_Poll ();
  return 0;
}
