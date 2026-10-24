/* menu2.c - DeskLib64 test, desktop: Menu2 (a menu attached to an icon with the Menu button, a ticked and a shaded item, a static submenu, a dynamic submenu made by a function, the flags function, the select function with Select and Adjust) and the Error library (Error_Report, Error_Check, Error_CheckSilent: the error box is shown and the call returns after Continue).  Every step is logged to <Wimp$ScrapDir>.menu2log. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "DeskLib:Core.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Wimp.h"
#include "DeskLib:WimpSWIs.h"
#include "DeskLib:Window.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Menu.h"
#include "DeskLib:Menu2.h"
#include "DeskLib:Error.h"

static window_handle win;
static icon_handle ic_menu, ic_report, ic_check, ic_quit;
static char text[4][48];
static FILE *logf;
static int flagscalls, dyncount;
static menu2_handle mainmenu, submenu, dynmenu;
static int tag_main = 1, tag_sub = 2, tag_dyn = 3;

static void logline (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
#include <stdarg.h>
static void logline (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  if (logf) { vfprintf (logf, fmt, ap); fputc ('\n', logf); fflush (logf); }
  va_end (ap);
}

static void main_flags (menu_ptr menu, void *reference)
{
  menu_item *items = (menu_item *) &menu[1];
  flagscalls++;
  items[0].menuflags.data.ticked = (flagscalls & 1) ? 1 : 0;		/* Alpha is ticked on every other opening (an Adjust click re-scans) */
  logline ("flags main ref=%d call=%d ticked=%d", *(int *) reference, flagscalls, items[0].menuflags.data.ticked);
}

static void main_select (int item, void *reference) { logline ("select main item=%d ref=%d", item, *(int *) reference); }
static void sub_select (int item, void *reference) { logline ("select sub item=%d ref=%d", item, *(int *) reference); }
static void dyn_select (int item, void *reference) { logline ("select dyn item=%d ref=%d", item, *(int *) reference); }

static menu2_handle main_sub (int item, event_pollblock *event, void *reference)
{
  (void) event;
  logline ("subfn main item=%d ref=%d", item, *(int *) reference);
  if (item == 3) return submenu;
  if (item == 4) return dynmenu;
  return 0;
}

static menu_ptr dyn_make (void *reference)
{
  char spec[64];
  dyncount++;
  snprintf (spec, sizeof spec, "Made %d,~Shaded,Longer than twelve characters", dyncount);
  logline ("makefn dyn ref=%d count=%d", *(int *) reference, dyncount);
  return Menu_New ("Dynamic menu title", spec);			/* a title longer than 12 characters and an indirected item */
}

static BOOL click_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  if (event->data.mouse.button.value != button_SELECT) return FALSE;
  if (event->data.mouse.icon == ic_report)
  {
    logline ("Error_Report before");
    Error_Report (7, "DeskLib64 Error_Report test %d", 42);
    logline ("Error_Report after");
  }
  else if (event->data.mouse.icon == ic_check)
  {
    os_error fake;
    BOOL r;
    fake.errnum = 99;
    strcpy (fake.errmess, "DeskLib64 Error_Check test");
    logline ("Error_CheckSilent(NULL)=%d Error_CheckSilent(err)=%d Error_Check(NULL)=%d", Error_CheckSilent (NULL), Error_CheckSilent (&fake), Error_Check (NULL));
    logline ("Error_Check before");
    r = Error_Check (&fake);
    logline ("Error_Check after returned %d", r);
  }
  else if (event->data.mouse.icon == ic_quit)
  {
    logline ("quit");
    if (logf) fclose (logf);
    Event_CloseDown ();
    exit (0);
  }
  return TRUE;
}

static BOOL warn_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  logline ("MENUWARN seen by EventMsg, selection %d %d", event->data.message.data.words[3], event->data.message.data.words[4]);
  return FALSE;
}

static BOOL close_handler (event_pollblock *event, void *reference)
{
  (void) event; (void) reference;
  logline ("closed");
  Event_CloseDown ();
  exit (0);
  return TRUE;
}

static icon_handle make_icon (int y, const char *label, char *buf, unsigned flags)
{
  icon_createblock ic;
  icon_handle h;
  os_error *e;
  memset (&ic, 0, sizeof ic);
  ic.window = win;
  ic.icondata.workarearect.min.x = 20; ic.icondata.workarearect.min.y = y - 56; ic.icondata.workarearect.max.x = 680; ic.icondata.workarearect.max.y = y;
  ic.icondata.flags.value = flags;
  strcpy (buf, label);
  ic.icondata.data.indirecttext.buffer = DL_PTR (buf);
  ic.icondata.data.indirecttext.validstring = (dl_ptr) -1;
  ic.icondata.data.indirecttext.bufflen = 48;
  e = Wimp_CreateIcon (&ic, &h);
  if (e) Error_ReportFatal (0, "Wimp_CreateIcon failed: %s", e->errmess);
  return h;
}

int main (void)
{
  window_block wb;
  os_error *e;
  unsigned flags = 0x07001119u | (1u << 5) | (3u << 12);
  char path[64];
  Event_Initialise ("DeskLib64 Menu2");
  EventMsg_Initialise ();
  snprintf (path, sizeof path, "<Wimp$ScrapDir>.menu2log");
  logf = fopen (path, "w");
  logline ("start");
  memset (&wb, 0, sizeof wb);
  wb.screenrect.min.x = 200; wb.screenrect.min.y = 300; wb.screenrect.max.x = 900; wb.screenrect.max.y = 700;
  wb.behind = -1;
  wb.flags.value = 0x8F000012u;
  wb.colours.cols.titlefore = 7; wb.colours.cols.titleback = 2; wb.colours.cols.workfore = 7; wb.colours.cols.workback = 1;
  wb.colours.cols.scrollouter = 3; wb.colours.cols.scrollinner = 1; wb.colours.cols.titlefocus = 12;
  wb.workarearect.min.x = 0; wb.workarearect.min.y = -400; wb.workarearect.max.x = 700; wb.workarearect.max.y = 0;
  wb.titleflags.value = 0x19; wb.workflags.value = 0;
  wb.spritearea = 1;
  wb.minsize.x = 100; wb.minsize.y = 100;
  strncpy (wb.title.text, "DLMenu2", 11);
  e = Wimp_CreateWindow (&wb, &win);
  if (e) Error_ReportFatal (0, "Wimp_CreateWindow failed: %s", e->errmess);
  ic_menu = make_icon (-20, "Menu-click here", text[0], flags);
  ic_report = make_icon (-100, "Error_Report", text[1], flags);
  ic_check = make_icon (-180, "Error_Check", text[2], flags);
  ic_quit = make_icon (-260, "Quit", text[3], flags);
  mainmenu = Menu2_Create ("Menu2 test", "Alpha,Beta,~Gamma|>Static sub,>Dynamic sub,Delta", NULL, main_flags, main_sub, main_select, &tag_main);
  submenu = Menu2_Create ("Static", "One,Two,!Three", NULL, NULL, NULL, sub_select, &tag_sub);
  dynmenu = Menu2_Create ("Dynamic", "", dyn_make, NULL, NULL, dyn_select, &tag_dyn);
  logline ("menus %s %s %s", mainmenu ? "ok" : "NULL", submenu ? "ok" : "NULL", dynmenu ? "ok" : "NULL");
  Menu2_AttachMenu (win, ic_menu, mainmenu, button_MENU);
  EventMsg_Claim (message_MENUWARN, event_ANY, warn_handler, NULL);
  Event_Claim (event_CLICK, win, event_ANY, click_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_CENTERED);
  logline ("ready");
  while (TRUE) Event_Poll ();
  return 0;
}
