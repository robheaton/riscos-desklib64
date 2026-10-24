/* pane.c - DeskLib64 test: the Pane library (a tool bar attached to a window with Pane_CreateAndLink) and Pane2 (panes added with Pane2_*), in resizable windows.  Clicks in the panes and the window positions are appended to <Wimp$ScrapDir>.panetest. */
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
#include "DeskLib:Pane.h"
#include "DeskLib:Pane2.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Error.h"

static window_handle main1, pane1, main2, pane2;

static void say (const char *fmt, ...)
{
  va_list ap;
  FILE *f = fopen ("<Wimp$ScrapDir>.panetest", "a");
  if (!f) return;
  va_start (ap, fmt); vfprintf (f, fmt, ap); va_end (ap);
  fputc ('\n', f);
  fclose (f);
}

static void where (const char *what, window_handle w)
{
  window_state ws;
  Wimp_GetWindowState (w, &ws);
  say ("%s: screen %d,%d to %d,%d scroll %d,%d open %d", what, ws.openblock.screenrect.min.x, ws.openblock.screenrect.min.y, ws.openblock.screenrect.max.x, ws.openblock.screenrect.max.y, ws.openblock.scroll.x, ws.openblock.scroll.y, ws.flags.data.open);
}

static BOOL click_handler (event_pollblock *event, void *reference)
{
  const char *names = (const char *) reference;
  say ("click in %s: icon %d buttons %d", names, event->data.mouse.icon, (int) event->data.mouse.button.value);
  if (event->data.mouse.window == pane1 && event->data.mouse.icon == 2) { Event_CloseDown (); exit (0); }
  if (event->data.mouse.window == pane1 && event->data.mouse.icon == 0) { where ("main1", main1); where ("pane1", pane1); }
  if (event->data.mouse.window == pane2 && event->data.mouse.icon == 0) { where ("main2", main2); where ("pane2", pane2); }
  return TRUE;
}

static BOOL close_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  say ("close of window %x", (unsigned) event->data.openblock.window);
  Event_CloseDown ();
  exit (0);
  return TRUE;
}

int main (void)
{
  wimp_point off, size;
  FILE *f = fopen ("<Wimp$ScrapDir>.panetest", "w");
  if (f) fclose (f);
  Event_Initialise ("DLPane");
  EventMsg_Initialise ();
  Resource_Initialise ("DLPane");
  Template_Initialise ();
  Template_LoadFile ("Templates");

  /* Pane: master and pane created together, offset and size given in code */
  off.x = 20; off.y = -60;
  size.x = 520; size.y = 60;
  main1 = Pane_CreateAndLink ("main", "pane", 20, 20, &off, &size, pane_OPEN | pane_FIXED | pane_RESIZE | pane_HORIZ);
  say ("Pane_CreateAndLink returned %x", (unsigned) main1);
  if (main1 == 0) Error_ReportFatal (0, "Pane_CreateAndLink failed");
  pane1 = Pane_GetSysHandle (main1) ? Pane_GetSysHandle (main1)->pane : 0;
  say ("pane window %x, flags %x", (unsigned) pane1, Pane_GetSysHandle (main1) ? (unsigned) Pane_GetSysHandle (main1)->flags.value : 0u);
  Event_Claim (event_OPEN, main1, event_ANY, Pane_OpenEventHandler, NULL);
  Event_Claim (event_OPEN, pane1, event_ANY, Handler_OpenWindow, NULL);
  Event_Claim (event_CLICK, pane1, event_ANY, click_handler, (void *) "pane1");
  Event_Claim (event_CLOSE, main1, event_ANY, close_handler, NULL);

  /* Pane2: a main window with a pane placed by the template */
  main2 = Pane2_CreateAndAddMain ("main2", 20);
  say ("Pane2_CreateAndAddMain returned %x", (unsigned) main2);
  pane2 = Pane2_CreateAndAddPane ("pane2", 20, main2, NULL, NULL, pane2_PANETOP | pane2_MAINTOP | pane2_HRESIZE);
  say ("Pane2_CreateAndAddPane returned %x", (unsigned) pane2);
  Event_Claim (event_CLICK, pane2, event_ANY, click_handler, (void *) "pane2");
  Event_Claim (event_CLOSE, main2, event_ANY, close_handler, NULL);

  Pane_Show (main1, open_NEARLAST);
  Pane2_Show (main2, open_NEARLAST);
  where ("main1", main1); where ("pane1", pane1); where ("main2", main2); where ("pane2", pane2);
  for (;;) Event_Poll ();
  return 0;
}
