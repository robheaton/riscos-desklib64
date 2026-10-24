/* taskwindow.c - DeskLib64 test, desktop: the TaskWindow library.  Starts commands in task windows (TaskWindow_Start with an output function and a "died" function), sends input to one (TaskWindow_SendInput), suspends, resumes and kills another, and checks TaskWindow_Check.  Every step is logged to <Wimp$ScrapDir>.twlog. */
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
#include "DeskLib:TaskWindow.h"
#include "DeskLib:Error.h"

enum { B_ECHO, B_ECHOSTART, B_SEND, B_QUITIN, B_DELAY, B_SEND2, B_SUSPEND, B_RESUME, B_KILL, B_CHECK, B_QUIT, NB };

static window_handle win;
static icon_handle ic[NB];
static char buf[NB][48];
static FILE *logf;
static task_handle h_echo, h_in, h_delay;
static int tags[4] = { 1, 2, 3, 4 };

static void logline (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  if (logf) { vfprintf (logf, fmt, ap); fputc ('\n', logf); fflush (logf); }
  va_end (ap);
}

static void output_fn (char *data, size_t len, void *ref)
{
  char text[256];
  size_t i, n = 0;
  for (i = 0; i < len && n + 4 < sizeof text; i++)
  {
    unsigned char c = data[i];
    if (c == '\n') { text[n++] = '\\'; text[n++] = 'n'; }
    else if (c == '\r') { text[n++] = '\\'; text[n++] = 'r'; }
    else if (c >= 32 && c < 127) text[n++] = c;
    else n += sprintf (text + n, "<%02x>", c);
  }
  text[n] = 0;
  logline ("output ref=%d len=%d [%s]", *(int *) ref, (int) len, text);
}

static void died_fn (void *ref) { logline ("died ref=%d", *(int *) ref); }

static task_handle ego_handle;

static BOOL ego_handler (event_pollblock *e, void *ref)
{
  (void) ref;
  ego_handle = e->data.message.header.sender;
  logline ("TASKWINDOW_EGO from task %x", (unsigned) ego_handle);
  return FALSE;			/* the library handles it too */
}

static void start (const char *cmd, const char *name, task_handle *h, int *ref)
{
  os_error *e;
  *h = 0;
  e = TaskWindow_Start (cmd, name, 0, output_fn, died_fn, ref, h);
  logline ("TaskWindow_Start '%s': %s handle returned=%x", cmd, e ? e->errmess : "ok", (unsigned) *h);
}

static BOOL click_handler (event_pollblock *e, void *ref)
{
  icon_handle i = e->data.mouse.icon;
  (void) ref;
  if (e->data.mouse.button.value != button_SELECT) return FALSE;
  if (i == ic[B_ECHO]) start ("Echo hello from the taskwindow", "DLTW echo", &h_echo, &tags[0]);
  else if (i == ic[B_ECHOSTART]) start ("<DLTaskWindow$Dir>.twecho", "DLTW input", &h_in, &tags[1]);
  else if (i == ic[B_SEND]) { h_in = ego_handle; TaskWindow_SendInput (h_in, "ping one\n", 9); logline ("SendInput 'ping one'"); }
  else if (i == ic[B_QUITIN]) { TaskWindow_SendInput (h_in, "quit\n", 5); logline ("SendInput 'quit'"); }
  else if (i == ic[B_DELAY]) { start ("<DLTaskWindow$Dir>.twecho", "DLTW two", &h_delay, &tags[2]); }
  else if (i == ic[B_SEND2]) { h_delay = ego_handle; TaskWindow_SendInput (h_delay, "second\n", 7); logline ("SendInput 'second' to %x", (unsigned) h_delay); }
  else if (i == ic[B_SUSPEND]) { h_delay = ego_handle; TaskWindow_Suspend (h_delay); logline ("Suspend %x", (unsigned) h_delay); }
  else if (i == ic[B_RESUME]) { TaskWindow_Resume (h_delay); logline ("Resume %x", (unsigned) h_delay); }
  else if (i == ic[B_KILL]) { TaskWindow_Kill (h_delay); logline ("Kill %x", (unsigned) h_delay); }
  else if (i == ic[B_CHECK]) logline ("TaskWindow_Check in this task = %d", TaskWindow_Check ());
  else if (i == ic[B_QUIT])
  {
    logline ("quit");
    if (logf) fclose (logf);
    Event_CloseDown ();
    exit (0);
  }
  return TRUE;
}

static BOOL close_handler (event_pollblock *e, void *ref)
{
  (void) e; (void) ref;
  Event_CloseDown ();
  exit (0);
  return TRUE;
}

static icon_handle mkicon (int x0, int y0, int x1, int y1, const char *label, char *b)
{
  icon_createblock c;
  icon_handle h;
  os_error *er;
  memset (&c, 0, sizeof c);
  c.window = win;
  c.icondata.workarearect.min.x = x0; c.icondata.workarearect.min.y = y0; c.icondata.workarearect.max.x = x1; c.icondata.workarearect.max.y = y1;
  c.icondata.flags.value = 0x07001119u | (1u << 5) | (3u << 12) | 4u;
  strcpy (b, label);
  c.icondata.data.indirecttext.buffer = DL_PTR (b);
  c.icondata.data.indirecttext.validstring = (dl_ptr) -1;
  c.icondata.data.indirecttext.bufflen = 48;
  er = Wimp_CreateIcon (&c, &h);
  if (er) Error_ReportFatal (0, "Wimp_CreateIcon failed: %s", er->errmess);
  return h;
}

int main (void)
{
  window_block wb;
  os_error *er;
  static const char *labels[NB] = { "Echo command", "Start twecho", "Send input", "Send quit", "Start twecho 2", "Send to 2", "Suspend", "Resume", "Kill", "Check", "Quit" };
  int i;
  Event_Initialise ("DeskLib64 TaskWindow");
  EventMsg_Initialise ();
  logf = fopen ("<Wimp$ScrapDir>.twlog", "w");
  logline ("start");
  memset (&wb, 0, sizeof wb);
  wb.screenrect.min.x = 200; wb.screenrect.min.y = 250; wb.screenrect.max.x = 700; wb.screenrect.max.y = 950;
  wb.behind = -1;
  wb.flags.value = 0x8F000012u;
  wb.colours.cols.titlefore = 7; wb.colours.cols.titleback = 2; wb.colours.cols.workfore = 7; wb.colours.cols.workback = 1;
  wb.colours.cols.scrollouter = 3; wb.colours.cols.scrollinner = 1; wb.colours.cols.titlefocus = 12;
  wb.workarearect.min.x = 0; wb.workarearect.min.y = -700; wb.workarearect.max.x = 500; wb.workarearect.max.y = 0;
  wb.titleflags.value = 0x19;
  wb.spritearea = 1;
  wb.minsize.x = 100; wb.minsize.y = 100;
  strncpy (wb.title.text, "DLTaskWin", 11);
  er = Wimp_CreateWindow (&wb, &win);
  if (er) Error_ReportFatal (0, "Wimp_CreateWindow failed: %s", er->errmess);
  for (i = 0; i < NB; i++) ic[i] = mkicon (20, -20 - 55 * i - 45, 480, -20 - 55 * i, labels[i], buf[i]);
  EventMsg_Claim (message_TASKWINDOW_EGO, event_ANY, ego_handler, NULL);
  Event_Claim (event_CLICK, win, event_ANY, click_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  Window_Show (win, open_WHEREVER);
  logline ("ready");
  while (TRUE) Event_Poll ();
  return 0;
}
