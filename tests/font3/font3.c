/* font3.c - DeskLib64 test: Font_Paint3 with a transformation matrix, a justification block and Font_ScanString, which go through the SWI () function (varargs).  At start the checks that need no drawing run and are appended to <Wimp$ScrapDir>.font3test; then a window paints the same string normally, scaled, rotated and mirrored. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "DeskLib:Core.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Wimp.h"
#include "DeskLib:WimpSWIs.h"
#include "DeskLib:Window.h"
#include "DeskLib:Font.h"
#include "DeskLib:ColourTran.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Error.h"

static FILE *logf;
static int fails, checks;
static window_handle win;
static const char text[] = "Rotate me";

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

static font_transformation matrix (int xx, int xy, int yx, int yy)
{
  font_transformation t;
  memset (&t, 0, sizeof t);
  t.scale.xx = xx; t.scale.xy = xy; t.scale.yx = yx; t.scale.yy = yy;
  return t;
}

/* the bounding box of the text with a transformation, in millipoints, through Font_ScanString */
static os_error *bbox (font_handle h, font_transformation *t, wimp_rect *out, char **endp)
{
  font_scanstringblock blk;
  int x = 1 << 30, y = 1 << 30, len = (int) strlen (text);		/* x and y: how far to scan (millipoints), as for Font_StringWidth */
  unsigned flags = font_scan_BBOX | (t ? (1u << 6) : 0);
  os_error *e;
  memset (&blk, 0, sizeof blk);
  blk.split_ch = -1;
  e = Font_ScanString (h, text, endp, flags, &x, &y, &blk, t, &len);
  *out = blk.bbox;
  return e;
}

static void run_checks (void)
{
  unsigned int hw = 0;
  font_handle *hp = (font_handle *) &hw;		/* the veneer stores a word: see the font test */
  wimp_rect b1, b2, bm, br;
  font_transformation t;
  char *end;
  os_error *e;
  int w1, h1, w2, h2;

  checke (Font_FindFont (hp, "Homerton.Medium", 24 * 16, 24 * 16, 0, 0), "Font_FindFont Homerton.Medium 24pt");
  if (!*hp) return;

  /* ScanString: the plain bounding box and where the scan ended (the output through a char ** parameter) */
  end = (char *) (uintptr_t) 0xDEADBEEF00000000ull;	/* the top half must be replaced, not left as it was */
  e = bbox (*hp, NULL, &b1, &end);
  checke (e, "Font_ScanString with font_scan_BBOX");
  say ("     box %d %d %d %d, end pointer %p (string at %p)", b1.min.x, b1.min.y, b1.max.x, b1.max.y, (void *) end, (const void *) text);
  w1 = b1.max.x - b1.min.x; h1 = b1.max.y - b1.min.y;
  check (w1 > 0 && h1 > 0, "a box with a positive width and height");
  check (end == text + strlen (text) || end == text, "the end pointer is a whole pointer into the string (not half of one)");

  /* a doubled matrix: the box doubles */
  t = matrix (2 << 16, 0, 0, 2 << 16);
  checke (bbox (*hp, &t, &b2, &end), "Font_ScanString with a 2x matrix");
  w2 = b2.max.x - b2.min.x; h2 = b2.max.y - b2.min.y;
  say ("     2x box %d %d %d %d", b2.min.x, b2.min.y, b2.max.x, b2.max.y);
  check (abs (w2 - 2 * w1) <= 8 && abs (h2 - 2 * h1) <= 8, "the 2x box is twice the width and height");

  /* a mirror: x is negated */
  t = matrix (-(1 << 16), 0, 0, 1 << 16);
  checke (bbox (*hp, &t, &bm, &end), "Font_ScanString with a mirror matrix");
  say ("     mirrored box %d %d %d %d", bm.min.x, bm.min.y, bm.max.x, bm.max.y);
  check (abs (bm.min.x + b1.max.x) <= 8 && abs (bm.max.x + b1.min.x) <= 8, "the mirrored box is the plain box negated in x");

  /* a quarter turn: width and height swap */
  t = matrix (0, 1 << 16, -(1 << 16), 0);
  checke (bbox (*hp, &t, &br, &end), "Font_ScanString with a quarter turn");
  say ("     turned box %d %d %d %d", br.min.x, br.min.y, br.max.x, br.max.y);
  check (abs ((br.max.x - br.min.x) - h1) <= 8 && abs ((br.max.y - br.min.y) - w1) <= 8, "after a quarter turn the width and height are swapped");

  /* Font_StringWidth for comparison with the scan */
  {
    font_string fs;
    memset (&fs, 0, sizeof fs);
    fs.s = DL_PTR (text); fs.x = 1 << 30; fs.y = 1 << 30; fs.split = -1; fs.term = (int) strlen (text);
    Font_SetFont (*hp);
    checke (Font_StringWidth (&fs), "Font_StringWidth");
    say ("     StringWidth %d, box width %d", fs.x, w1);
    check (fs.x > 0 && abs (fs.x - b1.max.x) < 4000, "the string width is the right edge of the box (within 4 points)");
  }

  /* Font_Paint3 returns without error with a matrix and with a justify block (the painting itself is on screen in the window) */
  (void) e;
  Font_LoseFont (*hp);
}

static void paint3 (font_handle h, int ox, int oy, font_transformation *t, unsigned extra)
{
  unsigned plot = font_plot_GIVENHANDLE | extra | (t ? font_plot_TRANSMATRIX : 0);
  os_error *e = Font_Paint3 (h, text, plot, ox, oy, NULL, t, (int) strlen (text));
  if (e) say ("Font_Paint3 error: %s", e->errmess);
}

static BOOL redraw_handler (event_pollblock *event, void *reference)
{
  window_redrawblock rb;
  BOOL more;
  unsigned int hw = 0;
  font_handle *hp = (font_handle *) &hw;
  static int reported;
  (void) reference;
  rb.window = event->data.openblock.window;
  Wimp_RedrawWindow (&rb, &more);
  if (Font_FindFont (hp, "Homerton.Medium", 20 * 16, 20 * 16, 0, 0)) return TRUE;
  while (more)
  {
    int ox = (rb.rect.min.x - rb.scroll.x) * 400, oy = (rb.rect.max.y - rb.scroll.y) * 400;	/* OS units to millipoints */
    font_transformation t;
    ColourTrans_SetFontColours (*hp, 0xFFFFFF00, 0x00000000, 14);
    paint3 (*hp, ox + 20 * 400, oy - 120 * 400, NULL, 0);						/* plain */
    t = matrix (2 << 16, 0, 0, 2 << 16);
    paint3 (*hp, ox + 20 * 400, oy - 190 * 400, &t, 0);						/* twice the size */
    t = matrix (56756, 32768, -32768, 56756);								/* 30 degrees */
    paint3 (*hp, ox + 20 * 400, oy - 330 * 400, &t, 0);
    t = matrix (-(1 << 16), 0, 0, 1 << 16);								/* mirrored */
    paint3 (*hp, ox + 560 * 400, oy - 120 * 400, &t, 0);
    t = matrix (0, 1 << 16, -(1 << 16), 0);								/* a quarter turn */
    paint3 (*hp, ox + 480 * 400, oy - 200 * 400, &t, 0);
    Wimp_GetRectangle (&rb, &more);
  }
  Font_LoseFont (*hp);
  if (!reported) { reported = 1; say ("first redraw done"); }
  return TRUE;
}

static void quit (void) { Event_CloseDown (); exit (0); }
static BOOL close_handler (event_pollblock *event, void *reference) { (void) event; (void) reference; quit (); return TRUE; }

int main (void)
{
  logf = fopen ("<Wimp$ScrapDir>.font3test", "w");
  if (!logf) return 1;
  Event_Initialise ("DLFont3");
  EventMsg_Initialise ();
  Resource_Initialise ("DLFont3");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  run_checks ();
  say ("SUMMARY %d failed of %d", fails, checks);
  win = Window_Create ("main", 20);
  Event_Claim (event_REDRAW, win, event_ANY, redraw_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_CENTERED);
  for (;;) Event_Poll ();
  return 0;
}
