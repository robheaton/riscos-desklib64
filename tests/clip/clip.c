/* clip.c - DeskLib64 test: the Clipboard library.  Two tasks are started from the same application: "owner" (the !Run file) and "paster" (the Paster file).  After a short delay the paster claims the clipboard with Clipboard_Claim and asks for data with Clipboard_Request, and also sends the same two messages itself with Wimp_SendMessage to see the Wimp's answer; the owner logs the messages it receives.  Both write to <Wimp$ScrapDir>.cliptest_<role> and exit by themselves. */
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
#include "DeskLib:Clipboard.h"
#include "DeskLib:Error.h"

static FILE *logf;
static int ticks, got_claim, got_request;
static int is_owner;

static void say (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt); vfprintf (logf, fmt, ap); va_end (ap);
  fputc ('\n', logf);
  fflush (logf);
}

static BOOL claim_handler (event_pollblock *event, void *reference)
{
  unsigned flags; memcpy (&flags, &event->data.message.data.claimentity, sizeof flags);
  (void) reference;
  got_claim++;
  say ("received ClaimEntity: size %d flags word %08x (selection bits %d clipboard bit %d) from task %x", event->data.message.header.size, flags, (int) event->data.message.data.claimentity.selectionclaimed & 3, (int) event->data.message.data.claimentity.clipboardclaimed & 1, (unsigned) event->data.message.header.sender);
  return TRUE;
}

static BOOL request_handler (event_pollblock *event, void *reference)
{
  message_datarequest *d = &event->data.message.data.datarequest;
  int i;
  (void) reference;
  got_request++;
  say ("received DataRequest: size %d window %x internal %u x %d y %d senddata %d", event->data.message.header.size, (unsigned) d->window, d->internal_handle, d->x, d->y, (int) d->senddata & 1);
  for (i = 0; i < 6 && d->filetypes[i] != -1; i++) say ("     filetype %03x", (unsigned) d->filetypes[i]);
  return TRUE;
}

static BOOL null_handler (event_pollblock *event, void *reference)
{
  (void) event; (void) reference;
  ticks++;
  if (!is_owner && ticks == 20)
  {
    int types[3];
    message_block m;
    os_error *e;
    types[0] = 0xFFF; types[1] = 0xFFD; types[2] = -1;
    say ("paster: Clipboard_Claim (TRUE, TRUE)");
    Clipboard_Claim (TRUE, TRUE);
    say ("paster: Clipboard_Request (window 0x1234, internal 77, no send, types FFF FFD)");
    Clipboard_Request (0x1234, 77, FALSE, NULL, types);
    /* the same two messages sent directly, to see any error the Wimp gives */
    memset (&m, 0, sizeof m);
    m.header.size = 24; m.header.action = message_CLAIMENTITY;
    { unsigned three = 3; memcpy (&m.data.claimentity, &three, sizeof three); }
    e = Wimp_SendMessage (event_SEND, &m, 0, 0);
    say ("paster: Wimp_SendMessage ClaimEntity size 24: %s", e ? e->errmess : "ok");
    memset (&m, 0, sizeof m);
    m.header.size = 44 + 4 * 2; m.header.action = message_DATAREQUEST;
    m.data.datarequest.window = 0x1234; m.data.datarequest.internal_handle = 78; m.data.datarequest.filetypes[0] = 0xFFF; m.data.datarequest.filetypes[1] = -1;
    e = Wimp_SendMessage (event_SEND, &m, 0, 0);
    say ("paster: Wimp_SendMessage DataRequest size %d: %s", m.header.size, e ? e->errmess : "ok");
    memset (&m, 0, sizeof m);
    m.header.size = 45; m.header.action = message_DATAREQUEST;
    e = Wimp_SendMessage (event_SEND, &m, 0, 0);
    say ("paster: Wimp_SendMessage with the size 45 that Clipboard_Request would use for one type: %s", e ? e->errmess : "ok");
  }
  if (ticks > 80)
  {
    say ("%s done: %d claims and %d requests received", is_owner ? "owner" : "paster", got_claim, got_request);
    fclose (logf);
    Event_CloseDown ();
    exit (0);
  }
  return TRUE;
}

int main (int argc, char **argv)
{
  char name[200];
  is_owner = !(argc > 1 && strcmp (argv[1], "paster") == 0);
  snprintf (name, sizeof name, "<Wimp$ScrapDir>.cliptest_%s", is_owner ? "owner" : "paster");
  logf = fopen (name, "w");
  if (!logf) return 1;
  Event_Initialise (is_owner ? "DLClipOwner" : "DLClipPaster");
  EventMsg_Initialise ();
  EventMsg_Claim (message_CLAIMENTITY, event_ANY, claim_handler, NULL);
  EventMsg_Claim (message_DATAREQUEST, event_ANY, request_handler, NULL);
  Event_Claim (event_NULL, event_ANY, event_ANY, null_handler, NULL);
  say ("%s started", is_owner ? "owner" : "paster");
  for (;;) Event_PollIdle (10);
  return 0;
}
