/* gaps2.c - DeskLib64 test, desktop task that runs its checks at start and writes them to <Wimp$ScrapDir>.gaps2log, then quits: Pane2 (added to windows from templates: handles, numbers, variables, flags, open, hide, delete), Pane (Link, SetFlags, Hide, Delete), the Handler functions (called with events made by the test), Resource (automatic path, path, sprites, choices) and the Font functions that were not reached by the font tests. */
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
#include "DeskLib:Icon.h"
#include "DeskLib:Pane.h"
#include "DeskLib:Pane2.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Font.h"
#include "DeskLib:Error.h"

static FILE *logf;
static int fails, checks;

static void out (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vfprintf (logf, fmt, ap); fputc ('\n', logf); fflush (logf);
  va_end (ap);
}
static void check (int ok, const char *what) { checks++; if (!ok) fails++; out ("%s %s", ok ? "PASS" : "FAIL", what); }
static void checke (os_error *e, const char *what)
{
  checks++;
  if (e) { fails++; out ("FAIL %s: error %d %s", what, e->errnum, e->errmess); } else out ("PASS %s", what);
}

int main (void)
{
  window_handle m, p, p2, w1, w2;
  wimp_point off, size, o2, s2;
  window_state st;
  event_pollblock ev;
  os_error *e;
  char buf[300];
  int n;

  logf = fopen ("<Wimp$ScrapDir>.gaps2log", "w");
  Event_Initialise ("DLGaps2");
  EventMsg_Initialise ();
  Screen_CacheModeInfo ();
  out ("start");

  /* ---- Resource ---- */
  check (Resource_InitialiseAuto () == TRUE, "Resource_InitialiseAuto finds <DLGaps2$Dir>");
  out ("     path '%s'", resource_pathname);
  check (strstr (resource_pathname, "DLGaps2") != NULL, "the path names the application");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  check (Template_Find ("main") != NULL, "Template_LoadFile through the automatic path");
  Resource_InitialisePath ("DLGaps2Res");
  check (strcmp (resource_pathname, "DLGaps2Res:") == 0, "Resource_InitialisePath sets the path");
  Resource_InitialiseAuto ();		/* already initialised once: leaves the path */
  Resource_Initialise ("DLGaps2");
  out ("     path now '%s'", resource_pathname);
  {
    sprite_area sa = (sprite_area) malloc (16384);
    screen_modeval sm;
    void *rom = NULL, *ram = NULL;
    sprite_info si;
    sa->areasize = 16384; sa->numsprites = 0; sa->firstoffset = 16; sa->freeoffset = 16;
    Sprite_InitArea (sa);
    sm.screen_mode = 0x301680B5u;
    Sprite_Create (sa, "gaps2sp", 0, 16, 16, sm);
    checke (Sprite_Save (sa, "<DLGaps2$Dir>.Sprites"), "Sprite_Save <DLGaps2$Dir>.Sprites");
    Resource_LoadSprites ();
        (void) rom; (void) ram;
    check (resource_sprites != (sprite_area) 1 && resource_sprites != NULL, "Resource_LoadSprites made a user sprite area");
    e = Sprite_ReadInfo (resource_sprites, "gaps2sp", &si);
    checke (e, "the sprite from the file is in that area");
    remove ("<DLGaps2$Dir>.Sprites");
  }
  Resource_ChoicesInit ("DLGaps2Group", "DLGaps2", FALSE);
  memset (buf, 0, sizeof buf);
  Resource_ChoicesPath (buf, "Options", FALSE);
  out ("     choices read path '%s'", buf);
  check (strcmp (buf, "Choices:DLGaps2Group.DLGaps2") == 0, "Resource_ChoicesPath (read): one file, named after the application");
  memset (buf, 0, sizeof buf);
  Resource_ChoicesPath (buf, "Options", TRUE);
  out ("     choices write path '%s'", buf);
  check (strcmp (buf, "<Choices$Write>.DLGaps2Group.DLGaps2") == 0, "Resource_ChoicesPath (write)");

  /* ---- Pane2 ---- */
  m = Pane2_CreateAndAddMain ("main", 20);
  check (m >= 0, "Pane2_CreateAndAddMain");
  off.x = 0; off.y = 0; size.x = 300; size.y = 100;
  p = Pane2_CreateAndAddPane ("dlg", 20, m, &off, &size, pane2_PANETOP | pane2_MAINTOP | pane2_HRESIZE);
  check (p >= 0, "Pane2_CreateAndAddPane");
  check (Pane2_GetMainHandle (p) == m, "Pane2_GetMainHandle");
  check (Pane2_GetPaneHandle (m, 1) == p, "Pane2_GetPaneHandle 1");
  check (Pane2_GetPaneNumber (m, p) == 1, "Pane2_GetPaneNumber");
  memset (&o2, 0x55, sizeof o2); memset (&s2, 0x55, sizeof s2);
  check (Pane2_ReadVars (m, 1, &o2, &s2) == TRUE, "Pane2_ReadVars");
  out ("     pane offset %d,%d size %d,%d", o2.x, o2.y, s2.x, s2.y);
  check (s2.x == 300 && s2.y == 100, "the size that was given");
  o2.x = 10; o2.y = 20; s2.x = 250; s2.y = 90;
  check (Pane2_SetVars (m, 1, &o2, &s2) == TRUE, "Pane2_SetVars");
  memset (&off, 0, sizeof off); memset (&size, 0, sizeof size);
  Pane2_ReadVars (m, 1, &off, &size);
  check (off.x == 10 && off.y == 20 && size.x == 250 && size.y == 90, "the new variables read back");
  n = Pane2_SetFlags (m, 1, pane2_PANETOP | pane2_MAINTOP);
  out ("     SetFlags returned %x", n);
  p2 = Window_Create ("dlg", 20);
  check (Pane2_AddPane (m, p2, &o2, &s2, pane2_MAINRIGHT | pane2_VRESIZE) == TRUE, "Pane2_AddPane (a window made separately)");
  check (Pane2_GetPaneNumber (m, p2) == 2 && Pane2_GetPaneHandle (m, 2) == p2, "it is pane 2");
  Pane2_Show (m, open_WHEREVER);
  check (Window_IsOpen (m) && Window_IsOpen (p), "Pane2_Show opens the main window and the pane");
  Wimp_GetWindowState (m, &st);
  st.openblock.screenrect.max.x += 40;
  n = Pane2_OpenWindow (m, &st.openblock);
  out ("     Pane2_OpenWindow returned %d", n);
  check (Window_IsOpen (m), "Pane2_OpenWindow leaves the main window open");
  Pane2_Hide (m);
  check (!Window_IsOpen (m) && !Window_IsOpen (p) && !Window_IsOpen (p2), "Pane2_Hide closes all of them");
  Pane2_DeletePane (m, 2);
  check (Pane2_GetPaneHandle (m, 2) == 0, "Pane2_DeletePane removes pane 2 (the handle is 0 now)");
  Pane2_Delete (m);
  out ("PASS Pane2_Delete (no fault)"); checks++;
  w1 = Window_Create ("main", 20);
  w2 = Window_Create ("dlg", 20);
  check (Pane2_AddMain (w1) == TRUE, "Pane2_AddMain (a window made separately)");
  off.x = 0; off.y = 0; size.x = 200; size.y = 80;
  check (Pane2_AddPane (w1, w2, &off, &size, pane2_PANETOP | pane2_MAINTOP) == TRUE, "Pane2_AddPane");
  Pane2_ReadIcon (w1, 0, "dlg", &off, &size);
  out ("     Pane2_ReadIcon: offset %d,%d size %d,%d", off.x, off.y, size.x, size.y);
  out ("PASS Pane2_ReadIcon (no fault)"); checks++;
  Pane2_Delete (w1);

  /* ---- Pane ---- */
  w1 = Window_Create ("main", 20);
  w2 = Window_Create ("dlg", 20);
  off.x = 0; off.y = 0; size.x = 300; size.y = 100;
  Pane_Link (w1, w2, &off, &size, pane_OPEN | pane_FIXED | pane_RESIZE | pane_HORIZ);
  check (Pane_GetSysHandle (w1) != NULL && Pane_GetSysHandle (w1)->pane == w2, "Pane_Link makes the link");
  n = Pane_SetFlags (w1, pane_FIXED | pane_RESIZE);			/* not pane_OPEN: that marks the pane as already open */
  out ("     Pane_SetFlags returned %x", n);
  Pane_Show (w1, open_WHEREVER);
  out ("     after Pane_Show: main open %d, pane open %d", Window_IsOpen (w1), Window_IsOpen (w2));
  check (Window_IsOpen (w1), "Pane_Show opens the main window");
  Pane_Hide (w1);
  check (!Window_IsOpen (w1) && !Window_IsOpen (w2), "Pane_Hide closes both");
  Pane_Delete (w1);
  out ("PASS Pane_Delete (no fault)"); checks++;

  /* ---- Handler, with events made here ---- */
  w1 = Window_Create ("main", 20);
  Window_Show (w1, open_WHEREVER);
  memset (&ev, 0, sizeof ev);
  ev.type = event_OPEN;
  Wimp_GetWindowState (w1, &st);
  ev.data.openblock = st.openblock;
  check (Handler_OpenWindow (&ev, NULL) == TRUE, "Handler_OpenWindow");
  ev.type = event_CLOSE;
  ev.data.openblock.window = w1;
  check (Handler_CloseWindow (&ev, NULL) == TRUE && !Window_IsOpen (w1), "Handler_CloseWindow closes the window");
  ev.type = event_REDRAW;
  ev.data.openblock.window = w1;
  Window_Show (w1, open_WHEREVER);
  check (Handler_NullRedraw (&ev, NULL) == TRUE, "Handler_NullRedraw");
  check (Handler_HatchRedraw (&ev, NULL) == TRUE, "Handler_HatchRedraw");
  memset (&ev, 0, sizeof ev);
  ev.type = event_CLICK;
  ev.data.mouse.window = w1; ev.data.mouse.icon = 1;
  ev.data.mouse.button.value = button_SELECT;
  check (Handler_ClickOK (&ev, NULL) == TRUE && !Icon_GetSelect (w1, 1), "Handler_ClickOK selects, waits and deselects the icon");
  ev.data.mouse.button.value = button_MENU;
  check (Handler_ClickOK (&ev, NULL) == FALSE, "Handler_ClickOK ignores the Menu button");
  check (Handler_ModeChange (&ev, NULL) == FALSE, "Handler_ModeChange");
  memset (&ev, 0, sizeof ev);
  ev.type = event_CLOSE;
  ev.data.openblock.window = w1;
  check (Handler_DeleteWindow (&ev, NULL) == TRUE, "Handler_DeleteWindow");

  /* ---- Font ---- */
  {
    font_handle f = 0;
    font_info fi;
    font_thresholds t1, t2;
    font_state fs;
    font_array fa;
    e = Font_FindFont (&f, "Homerton.Medium", 12 * 16, 12 * 16, 0, 0);
    checke (e, "Font_FindFont");
    memset (&fi, 0, sizeof fi);
    checke (Font_CharBBox (f, 'A', 0, &fi), "Font_CharBBox A");
    out ("     A: %d,%d..%d,%d", fi.minx, fi.miny, fi.maxx, fi.maxy);
    check (fi.maxx > fi.minx && fi.maxy > fi.miny, "the box of an A has a size");
    checke (Font_ReadThresholds (&t1), "Font_ReadThresholds");
    t2 = t1;
    checke (Font_SetThresholds (&t2), "Font_SetThresholds (the same values)");
    memset (&t2, 0, sizeof t2);
    Font_ReadThresholds (&t2);
    check (memcmp (&t1, &t2, sizeof t1) == 0, "the thresholds are unchanged");
    checke (Font_SetFontColours (f, 0xFFFFFF00u, 0x00000000u, 14), "Font_SetFontColours");
    checke (Font_SetPalette (0, 15, 0, 0xFFFFFF00u, 0x00000000u), "Font_SetPalette");
    memset (&fs, 0, sizeof fs);
    checke (Font_FutureFont (&fs), "Font_FutureFont");
    out ("     future font %d, offset %d", fs.font, fs.offset);
    memset (&fa, 0, sizeof fa);
    fa.fonts[f & 255] = 1;
    Font_LoseAllFonts (&fa);
    out ("PASS Font_LoseAllFonts (no fault)"); checks++;
  }

  out ("SUMMARY %d failed of %d", fails, checks);
  fclose (logf);
  Event_CloseDown ();
  return 0;
}
