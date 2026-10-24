/* dialogs.c - DeskLib64 test: Dialog2 (static, transient and menu-leaf dialogue boxes) with indirected writable, option and radio icons loaded from a template file.
   What happens is shown in the status line and appended to <Wimp$ScrapDir>.dlgtest.  (The older Dialog library is not tested here: see docs/STATUS.md.) */
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
#include "DeskLib:Dialog.h"
#include "DeskLib:Dialog2.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Template.h"
#include "DeskLib:Error.h"

#define B_STATIC 1
#define B_TRANS 2
#define B_QUIT 3
#define D_NAME 1
#define D_SHOUT 2
#define D_RED 3
#define D_BLUE 5
#define D_CANCEL 6
#define D_OK 7

static window_handle win;
static icon_handle status;
static char statustext[96];
static menu_ptr menu;
static dialog2_block *dstatic, *dtrans, *dleaf;
static FILE *logf;

static void say (const char *fmt, ...)
{
  va_list ap;
  char b[96];
  va_start (ap, fmt); vsnprintf (b, sizeof b, fmt, ap); va_end (ap);
  Icon_SetText (win, status, b);
  logf = fopen ("<Wimp$ScrapDir>.dlgtest", "a");
  if (logf) { fprintf (logf, "%s\n", b); fclose (logf); }
}

static void quit (void) { Event_CloseDown (); exit (0); }

static void d_open (dialog2_block *d)
{
  say ("%s opened", (const char *) d->reference);
  Icon_SetText (d->window, D_NAME, "Fred");
  Icon_SetCaret (d->window, D_NAME);
}

static void d_ok (dialog2_block *d)
{
  char name[40];
  Icon_GetText (d->window, D_NAME, name);
  say ("%s OK: name=%s shout=%d colour=%d", (const char *) d->reference, name, Icon_GetSelect (d->window, 2), Icon_WhichRadio (d->window, D_RED, D_BLUE) - D_RED);
}

static BOOL click_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  if (event->data.mouse.button.data.menu) { Menu_Show (menu, event->data.mouse.pos.x - 64, event->data.mouse.pos.y); return TRUE; }
  switch (event->data.mouse.icon)
  {
    case B_STATIC: Dialog2_OpenDialogStatic (dstatic, open_CENTERED); break;
    case B_TRANS: Dialog2_OpenDialogMenu (dtrans, open_UNDERPOINTER); break;
    case B_QUIT: quit (); break;
  }
  return TRUE;
}

static BOOL menu_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  switch (event->data.selection[0])
  {
    case 0: say ("menu: Info"); break;
    case 2: quit (); break;
  }
  return TRUE;
}

static BOOL warn_handler (event_pollblock *event, void *reference)
{
  (void) reference;
  say ("menu warning, item %d", event->data.message.data.words[3]);
  Dialog2_OpenDialogMenuLeaf (event, dleaf);
  return TRUE;
}

static BOOL close_handler (event_pollblock *event, void *reference) { (void) event; (void) reference; quit (); return TRUE; }

int main (void)
{
  icon_createblock ic;
  os_error *e;
  logf = fopen ("<Wimp$ScrapDir>.dlgtest", "w"); if (logf) fclose (logf);
  Event_Initialise ("DLDialogs");
  EventMsg_Initialise ();
  Resource_Initialise ("DLDialogs");
  Template_Initialise ();
  Template_LoadFile ("Templates");
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
  menu = Menu_New ("DLDialogs", "Info,>Dialog,Quit");
  if (!menu) Error_ReportFatal (0, "Menu_New failed");
  dstatic = Dialog2_CreateDialogBlock ("dlg", D_OK, D_CANCEL, d_open, d_ok, (void *) "static");
  dtrans = Dialog2_CreateDialogBlock ("dlg", D_OK, D_CANCEL, d_open, d_ok, (void *) "transient");
  dleaf = Dialog2_CreateDialogBlock ("dlg", D_OK, D_CANCEL, d_open, d_ok, (void *) "leaf");
  if (!dstatic || !dtrans || !dleaf) Error_ReportFatal (0, "Dialog2_CreateDialogBlock failed");
  Event_Claim (event_CLICK, win, event_ANY, click_handler, NULL);
  Event_Claim (event_MENU, event_ANY, event_ANY, menu_handler, NULL);
  Event_Claim (event_CLOSE, win, event_ANY, close_handler, NULL);
  Event_Claim (event_OPEN, event_ANY, event_ANY, Handler_OpenWindow, NULL);
  EventMsg_Claim (message_MENUWARN, event_ANY, warn_handler, NULL);
  Window_Show (win, open_CENTERED);
  say ("Ready");
  for (;;) Event_Poll ();
  return 0;
}
