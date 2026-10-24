/* demo2.c - DeskLib64 test: a window loaded from a template file, a menu with ticks and shading, icon operations and messages in a status line. */
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
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Error.h"

#define ICON_QUIT 1

static window_handle win;
static icon_handle status;
static char statustext[64];
static menu_ptr menu;
static int ticked, beeps;

static void say (const char *fmt, int v) { Icon_Printf (win, status, fmt, v); }

static void quit (void) { Event_CloseDown (); exit (0); }

static BOOL click_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  if (event->data.mouse.button.data.menu) { Menu_Show (menu, event->data.mouse.pos.x - 64, event->data.mouse.pos.y); return TRUE; }
  if (event->data.mouse.icon == ICON_QUIT) quit ();
  return TRUE;
}

static BOOL menu_handler (event_pollblock *event, void *reference)
{
  int item = event->data.selection[0];
  (void) reference;
  switch (item)
  {
    case 0: say ("Info chosen (item %d)", item); break;
    case 1: beeps++; say ("Beep number %d", beeps); break;
    case 2: ticked = !ticked; Menu_SetFlags (menu, 2, ticked, 0); say ("Tick is now %d", ticked); break;
    case 4: quit (); break;
    default: say ("Item %d", item); break;
  }
  return TRUE;
}

static BOOL close_handler (event_pollblock *event, void *reference) { (void) event; (void) reference; quit (); return TRUE; }

int main (void)
{
  icon_createblock ic;
  os_error *e;
  Event_Initialise ("DLDemo2");
  EventMsg_Initialise ();
  Resource_Initialise ("DLDemo2");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  win = Window_Create ("main", 20);
  memset (&ic, 0, sizeof ic);							/* the status line: an indirected text icon made in code */
  ic.window = win;
  ic.icondata.workarearect.min.x = 20; ic.icondata.workarearect.min.y = -140; ic.icondata.workarearect.max.x = 580; ic.icondata.workarearect.max.y = -90;
  ic.icondata.flags.value = 0x07001119u | (1u << 5);
  strcpy (statustext, "Ready");
  ic.icondata.data.indirecttext.buffer = DL_PTR (statustext);
  ic.icondata.data.indirecttext.validstring = (dl_ptr) -1;
  ic.icondata.data.indirecttext.bufflen = sizeof statustext;
  e = Wimp_CreateIcon (&ic, &status);
  if (e) Error_ReportFatal (0, "Wimp_CreateIcon failed: %s", e->errmess);
  menu = Menu_New ("DLDemo2", "Info,Beep,Ticked,~Shaded,Quit");
  if (!menu) Error_ReportFatal (0, "Menu_New failed");
  Event_Claim (event_CLICK, win, event_ANY, click_handler, NULL);
  Event_Claim (event_MENU, event_ANY, event_ANY, menu_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_CENTERED);
  for (;;) Event_Poll ();
  return 0;
}
