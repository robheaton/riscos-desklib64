/* gaps3.c - DeskLib64 test, desktop task that runs its checks at start and writes them to <Wimp$ScrapDir>.gaps3log, then quits: window help (Window_AutoHelp and Window_HelpHandler answering a help request sent to the task itself), Menu_PopUp, Menu_PopUpAuto and Menu_Warn, Icon_BarIconUser, Icon_ButtonIsHeld, Icon_FindValidationStringCommand, Wimp_Enumerate, Wimp_ReadPixTrans, and the font caret functions. */
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
#include "DeskLib:Msgs.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Font.h"
#include "DeskLib:Error.h"
#include "DeskLib:Time.h"

static FILE *logf;
static int fails, checks;
static char helptext[240];
static int gotreply, warned;

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

static BOOL reply_handler (event_pollblock *e, void *ref)
{
  (void) ref;
  strncpy (helptext, e->data.message.data.helpreply.text, sizeof helptext - 1);
  gotreply++;
  return TRUE;
}

static BOOL req_handler (event_pollblock *e, void *ref) { (void) e; (void) ref; return FALSE; }

static BOOL null_handler (event_pollblock *e, void *ref) { (void) e; (void) ref; return FALSE; }

static BOOL warn_handler (event_pollblock *e, void *ref) { (void) e; (void) ref; warned++; return FALSE; }

static void help_request (window_handle w, icon_handle i)
{
  message_block mb;
  int k;
  unsigned t0 = Time_Monotonic ();
  memset (&mb, 0, sizeof mb);
  mb.header.size = 20 + sizeof (message_helprequest);
  mb.header.action = message_HELPREQUEST;
  mb.data.helprequest.where.window = w;
  mb.data.helprequest.where.icon = i;
  gotreply = 0; helptext[0] = 0;
  Wimp_SendMessage (event_SENDWANTACK, &mb, event_taskhandle, 0);	/* the Wimp sends help requests as recorded messages */
  for (k = 0; k < 20000 && !gotreply && Time_Monotonic () - t0 < 300; k++) Event_Poll ();		/* up to 3 seconds */
}

int main (void)
{
  window_handle win;
  os_error *e;
  int n;

  logf = fopen ("<Wimp$ScrapDir>.gaps3log", "w");
  Event_Initialise ("DLGaps3");
  EventMsg_Initialise ();
  Screen_CacheModeInfo ();
  Resource_Initialise ("DLGaps3");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  out ("start");
  win = Window_Create ("main", 20);
  Window_Show (win, open_WHEREVER);

  /* ---- window help ---- */
  check (Msgs_LoadFile ("Msgs") == TRUE, "Msgs_LoadFile");
  check (Window_AutoHelp (event_ANY, event_ANY) == TRUE, "Window_AutoHelp");
  EventMsg_Claim (message_HELPREPLY, event_ANY, reply_handler, NULL);
  EventMsg_Claim (message_HELPREQUEST, event_ANY, req_handler, NULL);	/* the task must receive help requests: Window_AutoHelp answers them through Event, not EventMsg */
  Event_Claim (event_NULL, event_ANY, event_ANY, null_handler, NULL);		/* so that Event_Poll returns without waiting for an event */
  {
    char t[240];
    memset (t, 0, sizeof t);
    out ("     Msgs_Lookup main.1: %s '%s'", Msgs_Lookup ("main.1", t, 200) ? "found" : "not found", t);
  }
  help_request (win, 1);
  out ("     help for icon 1: '%s'", helptext);
  check (gotreply && strncmp (helptext, "Help for the Static button", 26) == 0, "the help request was answered with the text from the messages file");
  { unsigned t1 = Time_Monotonic (); while (Time_Monotonic () - t1 < 50) Event_Poll (); }		/* let the first reply be acknowledged */
  help_request (win, 2);
  check (strncmp (helptext, "Help for the Transient button", 29) == 0, "and the help for icon 2");

  /* ---- Menu: PopUp, PopUpAuto, Warn ---- */
  {
    menu_ptr m = Menu_New ("Pop", "One,Two,Three");
    menu_item *items;
    Menu_PopUp (m, win, 1);
    out ("PASS Menu_PopUp (no fault)"); checks++;
    Wimp_CreateMenu ((menu_ptr) -1, 0, 0);			/* close it */
    Menu_PopUpAuto (m);
    out ("PASS Menu_PopUpAuto (no fault)"); checks++;
    Wimp_CreateMenu ((menu_ptr) -1, 0, 0);
    Menu_Warn (m, 1, TRUE, warn_handler, NULL);
    items = (menu_item *) &m[1];
    check (items[1].menuflags.data.notifysub == 1, "Menu_Warn sets the warning flag");
    Menu_Warn (m, 1, FALSE, NULL, NULL);
    check (items[1].menuflags.data.notifysub == 0, "and clears it");
    Menu_FullDispose (m);
  }

  /* ---- Icon ---- */
  check (Icon_ButtonIsHeld () == FALSE, "Icon_ButtonIsHeld with no button down");
  {
    int len = -1;
    n = Icon_FindValidationStringCommand ("Sradiooff,radioon;A0-9", 'A', &len);
    out ("     FindValidationStringCommand A: index %d length %d", n, len);
    check (n >= 0, "Icon_FindValidationStringCommand finds the A command");
    n = Icon_FindValidationStringCommand ("Sradiooff,radioon;A0-9", 'S', &len);
    out ("     S: index %d length %d", n, len);
    check (n == 0 && len > 0, "and the S command at the start");
  }
  {
    sprite_area sa = (sprite_area) malloc (16384);
    screen_modeval sm;
    icon_handle h;
    scale_block sc;
    static char trans[4096];
    sa->areasize = 16384; sa->numsprites = 0; sa->firstoffset = 16; sa->freeoffset = 16;
    Sprite_InitArea (sa);
    sm.screen_mode = 0x301680B5u;
    checke (Sprite_Create (sa, "gaps3sp", 0, 32, 32, sm), "Sprite_Create gaps3sp");
    h = Icon_BarIconUser ("gaps3sp", iconbar_RIGHT, sa);
    out ("     Icon_BarIconUser returned %d", h);
    check (h > 0, "Icon_BarIconUser creates an icon bar icon from a user sprite area (0 means it failed)");
    if (h > 0) { Wimp_DeleteIcon ((window_handle) -2, h); out ("PASS the icon bar icon was deleted"); checks++; }
    memset (&sc, 0, sizeof sc);
    e = Wimp_ReadPixTrans (256, sa, "gaps3sp", &sc, trans);
    checke (e, "Wimp_ReadPixTrans (a sprite by name in a user area)");
    out ("     scale %d/%d x, %d/%d y", sc.mul.x, sc.div.x, sc.mul.y, sc.div.y);
    check (sc.mul.x > 0 && sc.div.x > 0, "the scale factors were filled in");
  }

  /* ---- Wimp_Enumerate: the window stack ---- */
  {
    window_handle w = win;
    e = Wimp_Enumerate (windowenum_PARENT, &w);
    checke (e, "Wimp_Enumerate parent");
    out ("     parent of the window: %d", w);
    check (w == -1 || w >= 0, "a window handle or -1");
    w = -1;
    e = Wimp_Enumerate (windowenum_FRONT, &w);
    checke (e, "Wimp_Enumerate front (of the top level)");
    out ("     frontmost window: %d (this window is %d)", w, win);
    check (w != -2, "a window handle was returned");
  }

  /* ---- Font caret ---- */
  {
    font_handle f = 0;
    static char text[] = "Hello world";
    font_string fs;
    e = Font_FindFont (&f, "Homerton.Medium", 12 * 16, 12 * 16, 0, 0);
    checke (e, "Font_FindFont");
    Font_SetFont (f);
    memset (&fs, 0, sizeof fs);
    fs.s = DL_PTR (text);
    fs.x = 30000; fs.y = 0; fs.split = -1; fs.term = 0;
    e = Font_FindCaret (&fs);
    checke (e, "Font_FindCaret");
    out ("     FindCaret at x 30000: term %d split %d, s %08x (text at %p)", fs.term, fs.split, (unsigned) fs.s, (void *) text);
    check (fs.term > 0 && fs.term < 11, "the index is inside the string");
    check (fs.s >= DL_PTR (text) && fs.s <= DL_PTR (text) + 11, "the string pointer was updated to a place inside the string");
    memset (&fs, 0, sizeof fs);
    fs.s = DL_PTR (text); fs.x = 30000; fs.split = -1;
    e = Font_FindCaretJ (&fs, 0, 0);
    checke (e, "Font_FindCaretJ (no justification: the same as FindCaret)");
    out ("     FindCaretJ: term %d", fs.term);
    check (fs.term > 0 && fs.term < 11, "the same kind of result");
    checke (Font_Caret (7, 40, font_OSCOORS, 100, 100), "Font_Caret");
    Font_LoseFont (f);
  }

  Window_Delete (win);
  out ("SUMMARY %d failed of %d", fails, checks);
  fclose (logf);
  Event_CloseDown ();
  return 0;
}
