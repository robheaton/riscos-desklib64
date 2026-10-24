/* save.c - DeskLib64 test: the Save library (save box, file icon drag, data transfer protocol) and Drag. Results are appended to <Wimp$ScrapDir>.savetest. */
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
#include "DeskLib:Save.h"
#include "DeskLib:Drag.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Error.h"

#define B_SAVE 1
#define B_QUIT 2
#define S_ICON 0
#define S_NAME 1
#define S_CANCEL 2
#define S_OK 3

static window_handle win, savewin;
static icon_handle status;
static char statustext[96];

static void say (const char *fmt, ...)
{
  va_list ap;
  char b[96];
  FILE *f;
  va_start (ap, fmt); vsnprintf (b, sizeof b, fmt, ap); va_end (ap);
  Icon_SetText (win, status, b);
  f = fopen ("<Wimp$ScrapDir>.savetest", "a");
  if (f) { fprintf (f, "%s\n", b); fclose (f); }
}

static BOOL filesaver (char *filename, void *ref)
{
  FILE *f = fopen (filename, "w");
  (void) ref;
  say ("filesaver: %s %s", filename, f ? "opened" : "FAILED to open");
  if (!f) return FALSE;
  fprintf (f, "DeskLib64 save test\n");
  fclose (f);
  return TRUE;
}

static void result (save_result r, void *ref)
{
  (void) ref;
  say ("save result %d", (int) r);
}

static void quit (void) { Event_CloseDown (); exit (0); }

static BOOL click_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  if (event->data.mouse.icon == B_SAVE)
  {
    save_saveblock *sb;
    savewin = Window_Create ("save", 20);
    sb = Save_InitSaveWindowHandler (savewin, FALSE, TRUE, TRUE, S_ICON, S_OK, S_CANCEL, S_NAME, filesaver, NULL, result, 32, 0xFFF, NULL);
    say ("save box open, block %s", sb ? "ok" : "NULL");
    Window_Show (savewin, open_CENTERED);
    Icon_SetCaret (savewin, S_NAME);
  }
  else if (event->data.mouse.icon == B_QUIT) quit ();
  return TRUE;
}

static BOOL close_handler (event_pollblock *event, void *reference) { (void) event; (void) reference; quit (); return TRUE; }

int main (void)
{
  icon_createblock ic;
  os_error *e;
  FILE *f = fopen ("<Wimp$ScrapDir>.savetest", "w");
  if (f) fclose (f);
  Event_Initialise ("DLSave");
  EventMsg_Initialise ();
  Resource_Initialise ("DLSave");
  Template_Initialise ();
  Template_LoadFile ("Templates");
  Drag_Initialise (TRUE);
  win = Window_Create ("main", 20);
  memset (&ic, 0, sizeof ic);
  ic.window = win;
  ic.icondata.workarearect.min.x = 20; ic.icondata.workarearect.min.y = -300; ic.icondata.workarearect.max.x = 580; ic.icondata.workarearect.max.y = -250;
  ic.icondata.flags.value = 0x07001119u | (1u << 5);
  strcpy (statustext, "Ready");
  ic.icondata.data.indirecttext.buffer = DL_PTR (statustext);
  ic.icondata.data.indirecttext.validstring = (dl_ptr) -1;
  ic.icondata.data.indirecttext.bufflen = sizeof statustext;
  e = Wimp_CreateIcon (&ic, &status);
  if (e) Error_ReportFatal (0, "Wimp_CreateIcon failed: %s", e->errmess);
  Event_Claim (event_CLICK, win, event_ANY, click_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_CENTERED);
  say ("Ready");
  for (;;) Event_Poll ();
  return 0;
}
