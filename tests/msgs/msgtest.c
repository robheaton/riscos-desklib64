/* msgtest.c - DeskLib64 test, command line: MessageTrans (MsgTrans_*, MessageTrans_*) and the Msgs library.  Needs the files Messages and Msgs next to the program and the variable DLMsgs$Dir set to that directory.  Prints one line per check and a summary; the exit status is the number of failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DeskLib:Core.h"
#include "DeskLib:Resource.h"
#include "DeskLib:MsgTrans.h"
#include "DeskLib:Msgs.h"

static int fails, checks;
static void check (int ok, const char *what) { checks++; if (!ok) fails++; printf ("%s %s\n", ok ? "PASS" : "FAIL", what); }
static void checke (os_error *e, const char *what)
{
  checks++;
  if (e) { fails++; printf ("FAIL %s: error %d %s\n", what, e->errnum, e->errmess); } else printf ("PASS %s\n", what);
}

int main (void)
{
  msgtrans_filedesc *fd = NULL;
  msgtrans_filedesc raw;
  char buf[256], small[16], *p;
  int flags = 0, size = 0, n;
  os_error *e;
  static char filebuf[4096];

  Resource_Initialise ("DLMsgs");
  printf ("resource path: %s\n", resource_pathname);

  /* MessageTrans through the helper that mallocs the buffer */
  checke (MsgTrans_LoadFile (&fd, "Messages"), "MsgTrans_LoadFile Messages (found through the resource path)");
  check (fd != NULL, "got a file descriptor");
  if (fd)
  {
    checke (MsgTrans_Lookup (fd, "Hello", buf, sizeof buf), "MsgTrans_Lookup Hello");
    check (strcmp (buf, "Hello, world") == 0, "Hello is 'Hello, world'");
    checke (MsgTrans_LookupPS (fd, "Greet", buf, sizeof buf, "Rob", "late", NULL, NULL), "MsgTrans_LookupPS Greet with two parameters");
    check (strcmp (buf, "Hello Rob, you are late") == 0, "%0 and %1 substituted");
    checke (MsgTrans_LookupPS (fd, "Both", buf, sizeof buf, "a", "b", "c", "d"), "MsgTrans_LookupPS Both with four parameters");
    check (strcmp (buf, "a and b and c and d") == 0, "four parameters substituted");
    checke (MsgTrans_Lookup (fd, "Nothere:the default", buf, sizeof buf), "a missing token with a default");
    check (strcmp (buf, "the default") == 0, "the default text is used");
    e = MsgTrans_Lookup (fd, "Nothere", buf, sizeof buf);
    check (e != NULL, "a missing token without a default is an error");
    if (e) printf ("     (error %d: %s)\n", e->errnum, e->errmess);
    checke (MsgTrans_LookupPS (fd, "Long", small, sizeof small, NULL, NULL, NULL, NULL), "a result longer than the buffer");
    check (strlen (small) < sizeof small, "the result is terminated inside the buffer");
    printf ("     small buffer holds '%s'\n", small);
    checke (MsgTrans_LoseFile (fd), "MsgTrans_LoseFile");
  }

  /* the SWI veneers directly, with a buffer in the program's heap */
  {
    char path[256];
    snprintf (path, sizeof path, "%sMessages", resource_pathname);
    checke (MessageTrans_FileInfo (path, &flags, &size), "MessageTrans_FileInfo");
    printf ("     flags %d size %d\n", flags, size);
    check (size > 0 && size < (int) sizeof filebuf, "a plausible size");
    memset (&raw, 0, sizeof raw);
    checke (MessageTrans_OpenFile (&raw, path, filebuf), "MessageTrans_OpenFile with a buffer");
    p = buf; n = sizeof buf;
    checke (MessageTrans_Lookup (&raw, "Quit", &p, &n, NULL, NULL, NULL, NULL), "MessageTrans_Lookup Quit");
    check (n == 4 && strcmp (buf, "Quit") == 0, "size 4 and text Quit");
    p = NULL; n = 0;
    checke (MessageTrans_Lookup (&raw, "Hello", &p, &n, NULL, NULL, NULL, NULL), "MessageTrans_Lookup with no buffer (a read-only pointer)");
    check (p != NULL && n == 12 && strncmp (p, "Hello, world", 12) == 0, "the pointer reads 'Hello, world'");
    checke (MessageTrans_CloseFile (&raw), "MessageTrans_CloseFile");
  }

  /* the global messages (descriptor 0) */
  checke (MsgTrans_Lookup (0, "NoSuch:fallback", buf, sizeof buf), "global lookup with a default");
  check (strcmp (buf, "fallback") == 0, "the global default is used");

  /* the Msgs library */
  check (Msgs_LoadFile ("Msgs") != 0, "Msgs_LoadFile Msgs");
  check (Msgs_Lookup ("Win.Title", buf, sizeof buf) != 0 && strcmp (buf, "My window title") == 0, "Msgs_Lookup Win.Title");
  check (Msgs_Lookup ("Win.Quit", buf, sizeof buf) != 0 && strcmp (buf, "Quit now") == 0, "Msgs_Lookup Win.Quit");
  check (Msgs_Lookup ("Err.NoFile", buf, sizeof buf) != 0 && strcmp (buf, "File My window title was not found") == 0, "an include <Win.Title> is expanded");
  check (Msgs_Lookup ("Win.Nothere:a default", buf, sizeof buf) != 0 && strcmp (buf, "a default") == 0, "Msgs_Lookup with a default");
  check (Msgs_Lookup ("Win.Nothere", buf, sizeof buf) == 0 && buf[0] == 0, "a missing tag is FALSE with an empty result");
  check (Msgs_printf (buf, "fmt.count", 3, "the folder") != 0 && strcmp (buf, "3 files in the folder") == 0, "Msgs_printf formats with a looked up format");
  Msgs_DropGroup ("Win");
  check (Msgs_Lookup ("Win.Title", buf, sizeof buf) == 0, "Msgs_DropGroup removes the group");

  printf ("SUMMARY %d failed of %d\n", fails, checks);
  return fails;
}
