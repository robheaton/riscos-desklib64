/* hello.c - a small desktop application for RISC OS 64 using DeskLib64: a window with an indirected text icon that counts the clicks on it. */
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
#include "DeskLib:Error.h"

static window_handle win;
static icon_handle icon;
static char text[48];
static int clicks;

static BOOL click_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  clicks++;
  Icon_Printf (win, icon, "Hello from AArch64: %d clicks (button %d)", clicks, event->data.mouse.button.value);
  return TRUE;
}

static BOOL close_handler (event_pollblock *event, void *reference)
{
  (void) event; (void) reference;
  Event_CloseDown ();
  exit (0);
  return TRUE;
}

int main (void)
{
  window_block wb;
  icon_createblock ic;
  os_error *e;
  Event_Initialise ("DeskLib64 Hello");
  EventMsg_Initialise ();
  memset (&wb, 0, sizeof wb);
  wb.screenrect.min.x = 200; wb.screenrect.min.y = 300; wb.screenrect.max.x = 900; wb.screenrect.max.y = 700;
  wb.behind = -1;
  wb.flags.value = 0x8F000012u;							/* back, close, title, toggle icons; moveable; redrawn by the Wimp */
  wb.colours.cols.titlefore = 7; wb.colours.cols.titleback = 2; wb.colours.cols.workfore = 7; wb.colours.cols.workback = 1;
  wb.colours.cols.scrollouter = 3; wb.colours.cols.scrollinner = 1; wb.colours.cols.titlefocus = 12;
  wb.workarearect.min.x = 0; wb.workarearect.min.y = -400; wb.workarearect.max.x = 700; wb.workarearect.max.y = 0;
  wb.titleflags.value = 0x19; wb.workflags.value = 0;
  wb.spritearea = 1;
  wb.minsize.x = 100; wb.minsize.y = 100;
  strncpy (wb.title.text, "DeskLib64", 11);
  wb.numicons = 0;
  e = Wimp_CreateWindow (&wb, &win);
  if (e) Error_ReportFatal (0, "Wimp_CreateWindow failed: %s", e->errmess);
  memset (&ic, 0, sizeof ic);
  ic.window = win;
  ic.icondata.workarearect.min.x = 20; ic.icondata.workarearect.min.y = -120; ic.icondata.workarearect.max.x = 680; ic.icondata.workarearect.max.y = -60;
  ic.icondata.flags.value = 0x07001119u | (1u << 5) | (3u << 12);		/* text, centred, filled, indirected, button type click */
  strcpy (text, "Click me");
  ic.icondata.data.indirecttext.buffer = DL_PTR (text);
  ic.icondata.data.indirecttext.validstring = (dl_ptr) -1;
  ic.icondata.data.indirecttext.bufflen = sizeof text;
  e = Wimp_CreateIcon (&ic, &icon);
  if (e) Error_ReportFatal (0, "Wimp_CreateIcon failed: %s", e->errmess);
  Event_Claim (event_CLICK, win, icon, click_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_CENTERED);
  for (;;) Event_Poll ();
  return 0;
}
