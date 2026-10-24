/* controls.c - DeskLib64 test, desktop: radio and option icons (Icon_SetRadios, Icon_WhichRadio, Icon_WhichRadioInEsg, Icon_GetSelect), Icon_ShadeGroup and Icon_SelectGroup, the IncDec handler (Icon_InitIncDecHandler), the old icon slider (Icon_SetSlider, Icon_ReadSlider, Icon_DragSlider, Icon_UpdateSlider) and the Slider library (Slider_Redraw in a user redrawn window, Slider_SetValue, Slider_ReadValue, Slider_Drag with an update function).  The Report icon writes the state of everything to <Wimp$ScrapDir>.ctllog. */
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
#include "DeskLib:Slider.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Error.h"

enum { R1, R2, R3, OPT, TXT, INC, DEC, SBASE, SLID, REPORT, SHADE, SETS, QUIT, NICONS };

static window_handle wa, wb;
static icon_handle ic[NICONS], sbase_b;
static char buf[NICONS + 1][48];
static FILE *logf;
static slider_info slider;
static int oldpos = 250, shaded, updates;
static BOOL dragging;
static int nulls, updates_old;
static icon_incdecblock *incdec;

static void logline (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  if (logf) { vfprintf (logf, fmt, ap); fputc ('\n', logf); fflush (logf); }
  va_end (ap);
}

static icon_handle mkicon (window_handle w, int x0, int y0, int x1, int y1, const char *label, unsigned flags, char *b)
{
  icon_createblock c;
  icon_handle h;
  os_error *e;
  memset (&c, 0, sizeof c);
  c.window = w;
  c.icondata.workarearect.min.x = x0; c.icondata.workarearect.min.y = y0; c.icondata.workarearect.max.x = x1; c.icondata.workarearect.max.y = y1;
  c.icondata.flags.value = flags;
  if (b) { strcpy (b, label); c.icondata.data.indirecttext.buffer = DL_PTR (b); c.icondata.data.indirecttext.validstring = (dl_ptr) -1; c.icondata.data.indirecttext.bufflen = 48; }
  e = Wimp_CreateIcon (&c, &h);
  if (e) Error_ReportFatal (0, "Wimp_CreateIcon failed: %s", e->errmess);
  return h;
}

#define TEXT      0x00000001u
#define BORDER    0x00000004u
#define HCENTRE   0x00000008u
#define VCENTRE   0x00000010u
#define FILLED    0x00000020u
#define INDIRECT  0x00000100u
#define TYPE(t)   ((unsigned) (t) << 12)
#define ESG(g)    ((unsigned) (g) << 16)
#define FG(c)     ((unsigned) (c) << 24)
#define BG(c)     ((unsigned) (c) << 28)
#define BASIC     (TEXT | HCENTRE | VCENTRE | FILLED | BORDER | INDIRECT | FG (7) | BG (1))

static void report (void)
{
  icon_handle r;
  int esg;
  r = Icon_WhichRadio (wa, ic[R1], ic[R3]);
  esg = Icon_WhichRadioInEsg (wa, 1);
  logline ("radio: WhichRadio=%d (R1=%d R2=%d R3=%d) InEsg=%d", r, ic[R1], ic[R2], ic[R3], esg);
  logline ("option: selected=%d shaded=%d", Icon_GetSelect (wa, ic[OPT]), Icon_GetShade (wa, ic[R1]));
  logline ("incdec: value=%d text='%s'", Icon_GetInteger (wa, ic[TXT]), Icon_GetTextPtr (wa, ic[TXT]));
  logline ("old slider: ReadSlider=%d (last %d)", Icon_ReadSlider (wa, ic[SBASE], ic[SLID]), oldpos);
  logline ("Slider library: ReadValue=%d updates=%d", Slider_ReadValue (&slider), updates);
}

static int slider_update (slider_info *s, void *ref)
{
  (void) ref;
  updates++;
  logline ("slider update value=%d", Slider_ReadValue (s));
  return 0;
}

static BOOL click_handler (event_pollblock *e, void *ref)
{
  icon_handle i = e->data.mouse.icon;
  (void) ref;
  if (e->data.mouse.window == wb)
  {
    if (e->data.mouse.button.value == button_SELECT || e->data.mouse.button.value == button_ADJUST)
    {
      int closed = 0, value = -1;
      os_error *er;
      logline ("Slider_Drag begin");
      er = Slider_Drag (&slider, &closed, &value, NULL);
      logline ("Slider_Drag end value=%d closed=%d error=%s", value, closed, er ? er->errmess : "none");
    }
    return TRUE;
  }
  if (i == ic[REPORT]) report ();
  else if (i == ic[SHADE])
  {
    icon_handle list[4];
    list[0] = ic[R1]; list[1] = ic[R2]; list[2] = ic[R3]; list[3] = -1;
    shaded = !shaded;
    Icon_ShadeGroup (wa, list, shaded);
    logline ("ShadeGroup %d", shaded);
  }
  else if (i == ic[SETS])
  {
    icon_handle list[3];
    int got = 0;
    list[0] = ic[R1]; list[1] = ic[R3]; list[2] = -1;
    Icon_SelectGroup (wa, list, TRUE);
    logline ("SelectGroup TRUE: R1=%d R3=%d", Icon_GetSelect (wa, ic[R1]), Icon_GetSelect (wa, ic[R3]));
    Icon_SetRadios (wa, ic[R1], ic[R3], ic[R2]);
    logline ("SetRadios R2: WhichRadio=%d (R2 is %d)", Icon_WhichRadio (wa, ic[R1], ic[R3]), ic[R2]);
    oldpos = Icon_SetSlider (wa, ic[SBASE], ic[SLID], 750);
    logline ("Icon_SetSlider 750 returned %d, Read %d; 1500 gives %d; -5 gives %d", oldpos, Icon_ReadSlider (wa, ic[SBASE], ic[SLID]), Icon_SetSlider (wa, ic[SBASE], ic[SLID], 1500), Icon_SetSlider (wa, ic[SBASE], ic[SLID], -5));
    oldpos = Icon_SetSlider (wa, ic[SBASE], ic[SLID], 500);
    Slider_SetValue (&slider, 70, &got, NULL);
    logline ("Slider_SetValue 70 -> %d (read %d); 500 -> %d", got, Slider_ReadValue (&slider), (Slider_SetValue (&slider, 500, &got, NULL), got));
    Slider_SetValue (&slider, 40, &got, NULL);
    Window_ForceRedraw (wb, -2000, -2000, 2000, 2000);
  }
  else if (i == ic[SBASE] || i == ic[SLID])
  {
    if (dragging) return TRUE;
    oldpos = Icon_DragSlider (wa, ic[SBASE], ic[SLID]);
    dragging = TRUE; nulls = 0; updates_old = 0;
    logline ("Icon_DragSlider start %d", oldpos);
  }
  else if (i == ic[QUIT])
  {
    logline ("quit");
    if (logf) fclose (logf);
    Event_CloseDown ();
    exit (0);
  }
  else if (i == ic[R1] || i == ic[R2] || i == ic[R3] || i == ic[OPT]) logline ("click icon %d: WhichRadio=%d option=%d", i, Icon_WhichRadio (wa, ic[R1], ic[R3]), Icon_GetSelect (wa, ic[OPT]));
  return TRUE;
}

static BOOL null_handler (event_pollblock *e, void *ref)
{
  mouse_block m;
  (void) e; (void) ref;
  nulls++;
  if (!dragging) return FALSE;
  Wimp_GetPointerInfo (&m);
  if (m.button.value == 0) { dragging = FALSE; logline ("Icon_DragSlider end %d (null events %d, updates %d)", oldpos, nulls, updates_old); return TRUE; }
  oldpos = Icon_UpdateSlider (wa, ic[SBASE], ic[SLID], oldpos);
  updates_old++;
  return TRUE;
}

static BOOL redraw_b (event_pollblock *e, void *ref)
{
  window_redrawblock blk;
  BOOL more;
  icon_block fake;
  (void) ref;
  blk.window = e->data.openblock.window;
  Wimp_RedrawWindow (&blk, &more);
  while (more)
  {
    int ox = blk.rect.min.x - blk.scroll.x, oy = blk.rect.max.y - blk.scroll.y;
    memset (&fake, 0, sizeof fake);
    fake.workarearect.min.x = ox + 20; fake.workarearect.min.y = oy - 100; fake.workarearect.max.x = ox + 680; fake.workarearect.max.y = oy - 40;
    fake.flags.value = FILLED | BORDER | FG (7) | BG (0);
    Wimp_PlotIcon (&fake);
    {
      wimp_rect ir;
      os_error *er;
      static int logged;
      Icon_ScreenPos (wb, sbase_b, &ir);
      er = Slider_Redraw (&slider, &blk.cliprect);
      if (logged++ < 3) logline ("redraw: icon %d,%d..%d,%d clip %d,%d..%d,%d value %d delta %d,%d error %s", ir.min.x, ir.min.y, ir.max.x, ir.max.y, blk.cliprect.min.x, blk.cliprect.min.y, blk.cliprect.max.x, blk.cliprect.max.y, slider.value, screen_delta.x, screen_delta.y, er ? er->errmess : "none");
    }
    Wimp_GetRectangle (&blk, &more);
  }
  return TRUE;
}

static BOOL close_handler (event_pollblock *e, void *ref)
{
  (void) e; (void) ref;
  logline ("closed");
  Event_CloseDown ();
  exit (0);
  return TRUE;
}

static void mkwindow (window_block *wb_, int x0, int y0, int x1, int y1, const char *title, unsigned flags, window_handle *h)
{
  os_error *er;
  memset (wb_, 0, sizeof *wb_);
  wb_->screenrect.min.x = x0; wb_->screenrect.min.y = y0; wb_->screenrect.max.x = x1; wb_->screenrect.max.y = y1;
  wb_->behind = -1;
  wb_->flags.value = flags;
  wb_->colours.cols.titlefore = 7; wb_->colours.cols.titleback = 2; wb_->colours.cols.workfore = 7; wb_->colours.cols.workback = 1;
  wb_->colours.cols.scrollouter = 3; wb_->colours.cols.scrollinner = 1; wb_->colours.cols.titlefocus = 12;
  wb_->workarearect.min.x = 0; wb_->workarearect.min.y = y0 - y1; wb_->workarearect.max.x = x1 - x0; wb_->workarearect.max.y = 0;
  wb_->titleflags.value = 0x19;
  wb_->spritearea = 1;
  wb_->minsize.x = 100; wb_->minsize.y = 100;
  strncpy (wb_->title.text, title, 11);
  er = Wimp_CreateWindow (wb_, h);
  if (er) Error_ReportFatal (0, "Wimp_CreateWindow failed: %s", er->errmess);
}

int main (void)
{
  window_block w;
  Event_Initialise ("DeskLib64 Controls");
  EventMsg_Initialise ();
  Screen_CacheModeInfo ();
  logf = fopen ("<Wimp$ScrapDir>.ctllog", "w");
  logline ("start");
  mkwindow (&w, 100, 400, 800, 1000, "DLControls", 0x8F000012u, &wa);
  ic[R1] = mkicon (wa, 20, -60, 220, -10, "Radio 1", BASIC | TYPE (11) | ESG (1), buf[R1]);
  ic[R2] = mkicon (wa, 20, -120, 220, -70, "Radio 2", BASIC | TYPE (11) | ESG (1), buf[R2]);
  ic[R3] = mkicon (wa, 20, -180, 220, -130, "Radio 3", BASIC | TYPE (11) | ESG (1), buf[R3]);
  ic[OPT] = mkicon (wa, 240, -60, 440, -10, "Option", BASIC | TYPE (11), buf[OPT]);
  ic[TXT] = mkicon (wa, 240, -120, 340, -70, "", BASIC | TYPE (0), buf[TXT]);
  ic[INC] = mkicon (wa, 350, -120, 390, -70, "+", BASIC | TYPE (2), buf[INC]);
  ic[DEC] = mkicon (wa, 400, -120, 440, -70, "-", BASIC | TYPE (2), buf[DEC]);
  ic[SBASE] = mkicon (wa, 20, -260, 440, -210, "", FILLED | BORDER | TYPE (6) | FG (7) | BG (0) | TEXT | INDIRECT, buf[SBASE]);
  ic[SLID] = mkicon (wa, 24, -256, 24 + 104, -214, "", FILLED | TYPE (6) | FG (7) | BG (10) | TEXT | INDIRECT, buf[SLID]);
  ic[REPORT] = mkicon (wa, 20, -330, 200, -280, "Report", BASIC | TYPE (3), buf[REPORT]);
  ic[SHADE] = mkicon (wa, 220, -330, 400, -280, "Shade radios", BASIC | TYPE (3), buf[SHADE]);
  ic[SETS] = mkicon (wa, 420, -330, 600, -280, "Set things", BASIC | TYPE (3), buf[SETS]);
  ic[QUIT] = mkicon (wa, 20, -400, 200, -350, "Quit", BASIC | TYPE (3), buf[QUIT]);
  Icon_SetRadios (wa, ic[R1], ic[R3], ic[R2]);
  incdec = Icon_InitIncDecHandler (wa, ic[TXT], ic[INC], ic[DEC], FALSE, 5, 0, 20, 10);
  logline ("incdec block %s", incdec ? "ok" : "NULL");
  oldpos = Icon_SetSlider (wa, ic[SBASE], ic[SLID], 250);
  Event_Claim (event_NULL, event_ANY, event_ANY, null_handler, NULL);
  Event_Claim (event_CLICK, wa, event_ANY, click_handler, NULL);
  Event_Claim (event_CLOSE, wa, event_ANY, close_handler, NULL);

  mkwindow (&w, 100, 100, 800, 300, "DLSlider", 0x8F000002u, &wb);	/* no auto redraw: the slider is drawn in the redraw loop */
  sbase_b = mkicon (wb, 20, -100, 680, -40, "", FILLED | BORDER | TYPE (6) | FG (7) | BG (0) | TEXT | INDIRECT, buf[NICONS]);
  memset (&slider, 0, sizeof slider);
  slider.window = wb; slider.icon = sbase_b; slider.limits.min = 0; slider.limits.max = 100;
  slider.colour.foreground = 10; slider.colour.background = 1;
  slider.border.x = 4; slider.border.y = 4;
  slider.update = slider_update;
  Slider_SetValue (&slider, 40, NULL, NULL);
  updates = 0;
  Event_Claim (event_REDRAW, wb, event_ANY, redraw_b, NULL);
  Event_Claim (event_CLICK, wb, event_ANY, click_handler, NULL);
  Event_Claim (event_CLOSE, wb, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (wa, open_WHEREVER);
  Window_Show (wb, open_WHEREVER);
  logline ("ready");
  while (TRUE) Event_Poll ();
  return 0;
}
