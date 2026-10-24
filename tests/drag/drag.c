/* drag.c - DeskLib64 test: DragASpr (the DragASprite module veneers) and, through DragASprite_DragIcon, the Wimp's icon and window state calls.  Dragging the sprite icon starts a sprite drag with DragASprite_DragIcon; Start begins one with DragASprite_Start; Stop ends it with DragASprite_Stop.  What happens is appended to <Wimp$ScrapDir>.dragtest. */
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
#include "DeskLib:DragASpr.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Error.h"

static window_handle win;

static void say (const char *fmt, ...)
{
  va_list ap;
  FILE *f = fopen ("<Wimp$ScrapDir>.dragtest", "a");
  if (!f) return;
  va_start (ap, fmt); vfprintf (f, fmt, ap); va_end (ap);
  fputc ('\n', f);
  fclose (f);
}

static BOOL click_handler (event_pollblock *event, void *reference)
{
  os_error *e;
  (void) reference;
  say ("click: icon %d buttons %d", event->data.mouse.icon, (int) event->data.mouse.button.value);
  if (event->data.mouse.icon == 1 && event->data.mouse.button.value == 64)		/* drag with Select */
  {
    e = DragASprite_DragIcon (win, 1);
    say ("DragASprite_DragIcon: %s", e ? e->errmess : "ok");
  }
  else if (event->data.mouse.icon == 2)
  {
    wimp_rect bound, parent;
    bound.min.x = 0; bound.min.y = 0; bound.max.x = 64; bound.max.y = 64;
    parent.min.x = 0; parent.min.y = 0; parent.max.x = 1900; parent.max.y = 1000;
    e = DragASprite_Start (0xC5, (void *) 1, "file_fff", &bound, &parent);
    say ("DragASprite_Start: %s", e ? e->errmess : "ok");
  }
  else if (event->data.mouse.icon == 3)
  {
    e = DragASprite_Stop ();
    say ("DragASprite_Stop: %s", e ? e->errmess : "ok");
  }
  else if (event->data.mouse.icon == 4) { Event_CloseDown (); exit (0); }
  return TRUE;
}

static BOOL userdrag_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  say ("user drag ended: box %d,%d to %d,%d", event->data.screenrect.min.x, event->data.screenrect.min.y, event->data.screenrect.max.x, event->data.screenrect.max.y);
  DragASprite_Stop ();
  return TRUE;
}

static BOOL close_handler (event_pollblock *event, void *reference) { (void) event; (void) reference; Event_CloseDown (); exit (0); return TRUE; }

int main (void)
{
  FILE *f = fopen ("<Wimp$ScrapDir>.dragtest", "w");
  if (f) fclose (f);
  Event_Initialise ("DLDrag");
  EventMsg_Initialise ();
  Resource_Initialise ("DLDrag");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  win = Window_Create ("main", 20);
  Event_Claim (event_CLICK, win, event_ANY, click_handler, NULL);
  Event_Claim (event_USERDRAG, event_ANY, event_ANY, userdrag_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_CENTERED);
  say ("ready");
  for (;;) Event_Poll ();
  return 0;
}
