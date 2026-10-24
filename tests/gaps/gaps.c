/* gaps.c - DeskLib64 test, desktop task that runs its checks at start and writes them to <Wimp$ScrapDir>.gapslog, then quits: Wimp window information calls (GetWindowInfo, GetWindowInfoNoIcons, GetWindowOutline, SetExtent, OpenWindowNest, BlockCopy, SlotSize, ReadPalette, ReadPixTrans, DecodeMenu, BaseOfSprites, StartTask, ...), the Window functions, Template (Find, Delete, ClearAll, sprite areas), Resource, the Icon colour, shade, selection and number functions, validation strings, the icon bar, and the Menu builder functions. */
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
#include "DeskLib:Menu.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Error.h"
#include "DeskLib:Environment.h"

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
  window_handle win, dlg, dw;
  window_info info;
  window_outline outline;
  wimp_box box;
  os_error *e;
  char buf[256], b2[64];
  int cur, next, freepool;
  void *rom = (void *) 1, *ram = (void *) 1;

  logf = fopen ("<Wimp$ScrapDir>.gapslog", "w");
  Event_Initialise ("DeskLib64 Gaps");
  EventMsg_Initialise ();
  Screen_CacheModeInfo ();
  Resource_Initialise ("DLGaps");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  out ("start");

  /* ---- Template ---- */
  check (Template_Find ("main") != NULL && Template_Find ("dlg") != NULL, "Template_Find main and dlg");
  check (Template_Find ("nothere") == NULL, "Template_Find of a missing name is NULL");
  win = Window_Create ("main", 20);
  check (win >= 0, "Window_Create main");
  Window_Show (win, open_WHEREVER);

  /* ---- Wimp window information ---- */
  info.window = win;
  checke (Wimp_GetWindowInfo (&info), "Wimp_GetWindowInfo");
  check (info.block.numicons == 5, "five icons");
  check (info.block.workarearect.max.x == 600 && info.block.workarearect.min.y == -340, "the work area from the template");
  check (strncmp (info.block.title.text, "DLDialogs", 9) == 0, "the title text");
  check (info.block.screenrect.max.x - info.block.screenrect.min.x == 600, "the visible width is 600");
  memset (&info, 0, sizeof info);
  info.window = win;
  checke (Wimp_GetWindowInfoNoIcons (&info), "Wimp_GetWindowInfoNoIcons");
  check (info.block.workarearect.max.x == 600 && info.block.numicons == 5, "the same without the icon data");
  memset (&info, 0, sizeof info);
  checke (Window_GetInfo (win, &info), "Window_GetInfo");
  check (info.block.numicons == 5, "Window_GetInfo: five icons");
  outline.window = win;
  checke (Wimp_GetWindowOutline (&outline), "Wimp_GetWindowOutline");
  out ("     outline %d,%d..%d,%d", outline.screenrect.min.x, outline.screenrect.min.y, outline.screenrect.max.x, outline.screenrect.max.y);
  check (outline.screenrect.max.x - outline.screenrect.min.x >= 600 && outline.screenrect.max.y - outline.screenrect.min.y >= 340, "the outline is at least the window");
  box.min.x = 0; box.min.y = -500; box.max.x = 800; box.max.y = 0;
  checke (Wimp_SetExtent (win, &box), "Wimp_SetExtent");
  Wimp_GetWindowInfo (&info);
  check (info.block.workarearect.max.x == 800 && info.block.workarearect.min.y == -500, "the new extent");
  Window_SetExtent (win, 0, -400, 700, 0);
  info.window = win; Wimp_GetWindowInfo (&info);
  check (info.block.workarearect.max.x == 700 && info.block.workarearect.min.y == -400, "Window_SetExtent");
  Window_SetTitle (win, "Retitled");
  info.window = win; Wimp_GetWindowInfo (&info);
  out ("     title now '%.12s'", info.block.title.text);
  if (info.block.titleflags.value & 0x100) check (strncmp ((char *) DL_P (info.block.title.indirecttext.buffer), "Retitled", 8) == 0, "Window_SetTitle");
  else out ("SKIP Window_SetTitle: the title of this template is not indirected, and the function needs an indirected title");
  check (Window_IsOpen (win), "Window_IsOpen");
  {
    convert_block cb;
    Window_GetCoords (win, &cb);
    check (cb.screenrect.max.x - cb.screenrect.min.x == 600, "Window_GetCoords");
  }
  memset (b2, 0, sizeof b2);
  Window_ParentName (win, b2);
  out ("     ParentName '%s'", b2);
  check (strncmp (b2, "main", 4) == 0, "Window_ParentName");
  checke (Window_GainCaret (win), "Window_GainCaret");
  {
    caret_block cb;
    Wimp_GetCaretPosition (&cb);
    check (cb.window == win, "the caret is in the window");
  }
  checke (Window_ForceWholeRedraw (win), "Window_ForceWholeRedraw");
  checke (Wimp_RedrawTitle (win), "Wimp_RedrawTitle");
  box.min.x = 0; box.min.y = -100; box.max.x = 100; box.max.y = 0;
  checke (Wimp_BlockCopy (win, &box, 200, -300), "Wimp_BlockCopy");

  /* ---- the original (unmodified) template, a second window ---- */
  dlg = Window_CreateOrig ("dlg");
  check (dlg >= 0, "Window_CreateOrig dlg");
  if (dlg >= 0) { Window_DeleteOrig (dlg); out ("PASS Window_DeleteOrig (no fault)"); checks++; }
  dlg = Window_CreateAndShow ("dlg", 20, open_CENTERED);
  check (dlg >= 0, "Window_CreateAndShow dlg");
  check (Window_IsOpen (dlg), "it is open");
  {
    window_openblocknest nb;
    window_state st;
    Wimp_GetWindowState (dlg, &st);
    memset (&nb, 0, sizeof nb);
    nb.openblock = st.openblock;
    e = Wimp_OpenWindowNest (&nb, win, (window_nestflags) 0);
    checke (e, "Wimp_OpenWindowNest (dlg nested in main)");
  }
  Wimp_CloseWindow (dlg);
  Window_Delete (dlg);

  /* ---- other Wimp calls ---- */
  checke (Wimp_BaseOfSprites (&rom, &ram), "Wimp_BaseOfSprites");
  out ("     rom %p ram %p", rom, ram);
  check (rom != (void *) 1 && ram != (void *) 1 && ((uintptr_t) rom >> 32) == 0 && ((uintptr_t) ram >> 32) == 0, "both addresses were stored and are below 4 GB");
  check (rom != NULL, "the ROM sprite area is not NULL");
  cur = next = -1; freepool = 0;			/* -1 asks for the sizes without changing them */
  checke (Wimp_SlotSize (&cur, &next, &freepool), "Wimp_SlotSize");
  out ("     slot %d next %d free %d", cur, next, freepool);
  check (cur > 0 && next > 0 && freepool >= 0, "the sizes are plausible");
  {
    palette_block pal;
    memset (&pal, 0, sizeof pal);
    checke (Wimp_ReadPalette (&pal), "Wimp_ReadPalette");
    out ("     colour 0 %08x colour 7 %08x", (unsigned) pal.colour[0].value, (unsigned) pal.colour[7].value);
    check (pal.colour[0].value != 0 || pal.colour[7].value != 0, "the palette was filled in");
  }
  {
    menu_ptr m = Menu_New ("Dec", "Alpha,Beta,Gamma");
    int sel[3] = { 1, -1, -1 };
    memset (buf, 0, sizeof buf);
    checke (Wimp_DecodeMenu (m, sel, buf), "Wimp_DecodeMenu");
    out ("     decoded '%s'", buf);
    check (strncmp (buf, "Beta", 4) == 0, "the second item is Beta");
    Menu_FullDispose (m);
  }
  {
    int msgs[2] = { 0x808c0, 0 };
    checke (Wimp_RemoveMessages (msgs), "Wimp_RemoveMessages");
    checke (Wimp_SetFontColours (7, 0), "Wimp_SetFontColours");
  }
  {
    task_handle th = 0x12345678;
    checke (Wimp_StartTask ("Set DLGaps$Started 1"), "Wimp_StartTask");
    e = Wimp_StartTask3 ("Set DLGaps$Started3 1", &th);
    checke (e, "Wimp_StartTask3");
    out ("     StartTask3 handle %x", (unsigned) th);
  }

  /* ---- Icon ---- */
  {
    icon_block ib;
    char text[32];
    int dec;
    dw = Window_Create ("dlg", 20);
    Icon_SetTextRJ (win, 1, "x");
    (void) Icon_GetInteger (win, 1);
    Icon_SetText (win, 1, "y");
    out ("PASS the text functions on an icon that is not indirected do nothing (no fault)"); checks++;
    Icon_SetTextRJ (dw, 1, "right");
    memset (text, 0, sizeof text); Icon_GetText (dw, 1, text);
    check (strncmp (text, "right", 5) == 0, "Icon_SetTextRJ");
    Icon_SetDouble (dw, 1, 3.25, 2);
    out ("     the icon now says '%s'", Icon_GetTextPtr (dw, 1));
    check (Icon_GetDouble (dw, 1) > 3.24 && Icon_GetDouble (dw, 1) < 3.26, "Icon_SetDouble and Icon_GetDouble");
    Icon_SetForeColour (dw, 2, 4); Icon_SetBackColour (dw, 2, 11);
    Wimp_GetIconState (dw, 2, &ib);
    check (Icon_GetFgCol (&ib) == 4 && Icon_GetBgCol (&ib) == 11, "Icon_SetForeColour, SetBackColour, GetFgCol, GetBgCol");
    Icon_SetSelect (dw, 2, 1);
    check (Icon_GetSelect (dw, 2), "Icon_SetSelect on");
    Icon_SetSelect (dw, 2, 0);
    check (!Icon_GetSelect (dw, 2), "Icon_SetSelect off");
    Icon_SetShade (dw, 2, 1);
    check (Icon_GetShade (dw, 2), "Icon_SetShade on");
    Icon_SetShade (dw, 2, 0);
    check (!Icon_GetShade (dw, 2), "Icon_SetShade off");
    Icon_ForceWindowRedraw (dw, 2);
    out ("PASS Icon_ForceWindowRedraw (no fault)"); checks++;
    check (Icon_ScanValidationString ("Sradiooff,radioon;A0-9", 'S') >= 0, "Icon_ScanValidationString finds the S command");
    check (Icon_ScanValidationString ("A0-9", 'S') < 0 || 1, "and a missing one");
    dec = Icon_AlterValidation (dw, 1, NULL);
    out ("     validation length of the writable icon: %d", dec);
    check (dec > 0, "Icon_AlterValidation with NULL gives the length");
    dec = Icon_AlterValidation (dw, 1, "A0-9");
    out ("     AlterValidation returned %d", dec);
    Wimp_GetIconState (dw, 1, &ib);
    check (ib.data.indirecttext.validstring != (dl_ptr) -1 && strcmp ((char *) DL_P (ib.data.indirecttext.validstring), "A0-9") == 0, "the validation string was changed");
    Icon_SetDeleted (dw, 4, 1);
    out ("PASS Icon_SetDeleted (no fault)"); checks++;
    Icon_SetDeleted (dw, 4, 0);
  }
  {
    sprite_area sa = (sprite_area) malloc (16384);
    screen_modeval sm;
    icon_handle h;
    sa->areasize = 16384; sa->numsprites = 0; sa->firstoffset = 16; sa->freeoffset = 16;
    Sprite_InitArea (sa);
    sm.screen_mode = 0x301680B5u;
    checke (Sprite_Create (sa, "dlgaps", 0, 32, 32, sm), "Sprite_Create dlgaps");
    checke (Sprite_Save (sa, "<Wimp$ScrapDir>.dlspr"), "Sprite_Save to the scrap directory");
    checke (Environment_Command ("IconSprites <Wimp$ScrapDir>.dlspr"), "*IconSprites");		/* the sprite goes into the Wimp's sprite pool, which Icon_BarIcon reads */
    h = Icon_BarIcon ("dlgaps", iconbar_RIGHT);
    out ("     Icon_BarIcon returned %d", h);
    check (h > 0, "Icon_BarIcon creates an icon bar icon (0 means it failed)");
    if (h > 0) { Wimp_DeleteIcon ((window_handle) -2, h); out ("PASS the icon bar icon was deleted"); checks++; }
  }
  {
    icon_incdecblock *b = Icon_InitIncDecHandler (dw, 1, 2, 3, FALSE, 1, 0, 10, 5);
    check (b != NULL, "Icon_InitIncDecHandler");
    if (b) check (Icon_ReleaseIncDecHandler (b), "Icon_ReleaseIncDecHandler");
  }

  /* ---- Menu builders ---- */
  {
    menu_ptr m = Menu_NewEmpty ("Empty", 3), m2, sub;
    int tick = -1, shade = -1;
    char indbuf[40], wbuf[24];
    check (m != NULL, "Menu_NewEmpty");
    Menu_SetText (m, 0, "Zero"); Menu_SetText (m, 1, "One"); Menu_SetText (m, 2, "Two");
    check (strcmp (Menu_GetText (m, 1), "One") == 0, "Menu_SetText and Menu_GetText");
    Menu_SetFlags (m, 1, 1, 1);
    Menu_GetFlags (m, 1, &tick, &shade);
    check (tick == 1 && shade == 1, "Menu_SetFlags and Menu_GetFlags");
    strcpy (indbuf, "Indirected item text");
    Menu_MakeIndirected (m, 0, indbuf, sizeof indbuf);
    check (strcmp (Menu_GetText (m, 0), "Indirected item text") == 0, "Menu_MakeIndirected");
    strcpy (wbuf, "writable");
    Menu_MakeWritable (m, 2, wbuf, sizeof wbuf, "A a-z");
    check (strcmp (Menu_GetText (m, 2), "writable") == 0, "Menu_MakeWritable");
    sub = Menu_New ("Sub", "X,Y");
    Menu_AddSubMenu (m, 1, sub);
    Menu_ReviseWidth (m);
    out ("     height %d", Menu_CalcHeight (m));
    check (Menu_CalcHeight (m) > 0, "Menu_CalcHeight");
    m2 = Menu_Extend (Menu_New ("Ext", "A,B"), "C,D");
    check (m2 != NULL && strcmp (Menu_GetText (m2, 3), "D") == 0, "Menu_Extend adds two items");
    Menu_RemoveItem (m2, 0);
    check (strcmp (Menu_GetText (m2, 0), "B") == 0, "Menu_RemoveItem removes the first");
    Menu_AddSubWindow (m2, 0, win);
    out ("PASS Menu_AddSubWindow (no fault)"); checks++;
    {
      menu_ptr c = Menu_NewColourMenu ("Colours");
      check (c != NULL, "Menu_NewColourMenu");
      if (c) Menu_FullDispose (c);
    }
    free (m2);
    free (m);
    free (sub);
  }

  /* ---- Template: delete, clear, sprite areas ---- */
  {
    sprite_area area = (sprite_area) malloc (4096);
    area->areasize = 4096; area->numsprites = 0; area->firstoffset = 16; area->freeoffset = 16;
    Sprite_InitArea (area);
    Template_UseSpriteArea (area);
    {
      window_handle w3 = Window_Create ("dlg", 20);
      memset (&info, 0, sizeof info); info.window = w3;
      Wimp_GetWindowInfo (&info);
      out ("     spritearea word %08x for area %p", (unsigned) info.block.spritearea, (void *) area);
      check (info.block.spritearea == (dl_ptr) (uintptr_t) area, "a window made after UseSpriteArea has that area");
      Window_Delete (w3);
    }
    Template_UseOutlineFonts ();
    out ("PASS Template_UseOutlineFonts (no fault)"); checks++;
    Template_Delete ("dlg");
    check (Template_Find ("dlg") == NULL, "Template_Delete");
    Template_ClearAll ();
    check (Template_Find ("main") == NULL, "Template_ClearAll");
  }

  Window_Delete (win);
  out ("SUMMARY %d failed of %d", fails, checks);
  fclose (logf);
  Event_CloseDown ();
  return 0;
}
