/* envtest.c - DeskLib64 test, command line: Str (the string functions that end at a control character, case-insensitive compares, hex and CR conversions), Time (the clock, conversions of a fixed time, formats) and Environment (system variables, GS expansion, commands, modules, tasks, the OS version).  Prints one line per check and a summary; the exit status is the number of failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DeskLib:Core.h"
#include "DeskLib:Str.h"
#include "DeskLib:Time.h"
#include "DeskLib:Environment.h"

static int fails, checks;
static void check (int ok, const char *what) { checks++; if (!ok) fails++; printf ("%s %s\n", ok ? "PASS" : "FAIL", what); }
static void checke (os_error *e, const char *what)
{
  checks++;
  if (e) { fails++; printf ("FAIL %s: error %d %s\n", what, e->errnum, e->errmess); } else printf ("PASS %s\n", what);
}

int main (void)
{
  char buf[256], b2[64];
  unsigned char clock[5], fixed[5];
  time_ordinals ord;
  os_error *e;
  unsigned t1, t2;
  char *end;
  int i;

  setvbuf (stdout, NULL, _IONBF, 0);		/* so that the last line shows if a call faults */

  /* ---- Str ---- */
  check (strlencr ("abc\rdef") == 3 && strlencr ("") == 0 && strlencr ("abc\n") == 3, "strlencr stops at any control character");
  check (strcmpcr ("abc\r", "abc") == 0 && strcmpcr ("abc\r", "abd\r") < 0 && strcmpcr ("b", "a\r") > 0, "strcmpcr");
  strcpy (buf, "dest:");
  check (strcatcr (buf, "tail\rignored") == buf && strcmp (buf, "dest:tail") == 0, "strcatcr appends up to the control character and returns the destination");
  memset (buf, 'x', sizeof buf);
  check (strcpycr (buf, "copy\rignored") == buf && strcmp (buf, "copy") == 0, "strcpycr copies up to the control character and terminates");
  memset (buf, 'x', sizeof buf);
  strncpycr (buf, "abcdefgh\r", 4);
  check (strncmp (buf, "abcd", 4) == 0, "strncpycr copies at most n characters");
  check (strcmp (Str_LeafName ("ADFS::Disc.$.Dir.File"), "File") == 0 && strcmp (Str_LeafName ("Plain"), "Plain") == 0, "Str_LeafName");
  check (stricmp ("Hello", "hELLO") == 0 && stricmp ("abc", "abd") < 0 && stricmp ("b", "A") > 0, "stricmp");
  check (strnicmp ("HELLOworld", "hellothere", 5) == 0 && strnicmp ("abcX", "abcY", 4) != 0, "strnicmp");
  check (stricmpcr ("Abc\r", "aBC") == 0, "stricmpcr");
  check (strnicmpcr ("ABCdef\r", "abcXYZ\r", 3) == 0, "strnicmpcr");
  { char *d = strdup ("dup me"); check (d != NULL && strcmp (d, "dup me") == 0, "strdup"); free (d); }
  strcpy (buf, "make cr"); Str_MakeCR (buf, 100);
  check (buf[7] == '\r' && buf[0] == 'm', "Str_MakeCR");
  Str_MakeASCIIZ (buf, 100);
  check (buf[7] == 0 && strcmp (buf, "make cr") == 0, "Str_MakeASCIIZ");
  check (Str_MakeHex (0) == '0' && Str_MakeHex (9) == '9' && Str_MakeHex (10) == 'A' && Str_MakeHex (15) == 'F' && Str_MakeHex (16) == '?', "Str_MakeHex");
  check (Str_DecodeHex ('0') == 0 && Str_DecodeHex ('9') == 9 && Str_DecodeHex ('A') == 10 && Str_DecodeHex ('F') == 15 && Str_DecodeHex ('G') == -1, "Str_DecodeHex");

  /* ---- Time ---- */
  checke (Time_ReadClock (clock), "Time_ReadClock");
  check (clock[4] >= 0x49, "the clock's top byte is plausible for a date after 1998");
  t1 = Time_Monotonic ();
  for (i = 0; i < 200000; i++) { __asm__ volatile (""); }
  t2 = Time_Monotonic ();
  check (t2 >= t1, "Time_Monotonic does not go backwards");
  printf ("     monotonic %u then %u\n", t1, t2);
  /* 1 January 2000, 12:34:56.78 (a Saturday): centiseconds since 1 January 1900 */
  {
    unsigned long long days = 36524ull;					/* 1900-01-01 to 2000-01-01 */
    unsigned long long cs = ((days * 86400ull) + 12ull * 3600 + 34 * 60 + 56) * 100 + 78;
    for (i = 0; i < 5; i++) fixed[i] = (unsigned char) (cs >> (8 * i));
  }
  memset (&ord, 0, sizeof ord);
  checke (Time_ConvertTimeToOrdinals (fixed, &ord), "Time_ConvertTimeToOrdinals");
  printf ("     %u-%u-%u %u:%u:%u.%u wday %u yday %u\n", ord.year, ord.month, ord.mday, ord.hour, ord.min, ord.sec, ord.cs, ord.wday, ord.yday);
  check (ord.year == 2000 && ord.month == 1 && ord.mday == 1, "the date is 1 January 2000");
  check ((ord.hour == 12 || ord.hour == 13) && ord.min == 34 && ord.sec == 56 && ord.cs == 78, "the time is 12:34:56.78 in local time (the hour may be shifted by the time zone)");
  check (ord.wday == 7 && ord.yday == 1, "a Saturday (7) and day 1 of the year");
  memset (&ord, 0, sizeof ord);
  checke (Time_ConvertTimeToUTCOrdinals (fixed, &ord), "Time_ConvertTimeToUTCOrdinals");
  check (ord.year == 2000 && ord.month == 1 && ord.hour == 12, "the UTC ordinals are 12:xx on 1 January 2000");
  memset (buf, 0, sizeof buf);
  checke (Time_ConvertDateAndTime (fixed, buf, sizeof buf, (char *) "%24:%MI:%SE %DY/%MN/%CE%YR"), "Time_ConvertDateAndTime");
  printf ("     '%s'\n", buf);
  check (strstr (buf, ":34:56 01/01/2000") != NULL || strstr (buf, ":34:56 1/1/2000") != NULL, "the formatted minutes, seconds and date");
  memset (buf, 0, sizeof buf);
  checke (Time_ConvertDateAndTime (fixed, buf, sizeof buf, (char *) "%W3 %M3 %DY%ST %CE%YR"), "weekday and month names");
  printf ("     '%s'\n", buf);
  check (strncmp (buf, "Sat Jan 01st 2000", 17) == 0 || strncmp (buf, "Sat Jan 1st 2000", 16) == 0, "the names and the ordinal suffix");
  memset (buf, 0, sizeof buf);
  checke (Time_ConvertStandardDateAndTime (fixed, buf, sizeof buf), "Time_ConvertStandardDateAndTime");
  printf ("     '%s'\n", buf);
  check (strstr (buf, "2000") != NULL, "it contains the year");

  /* ---- Environment ---- */
  check (Environment_SysVarRead ("DLEnv$Missing", buf, sizeof buf) == FALSE, "a variable that does not exist reads as FALSE");
  checke (Environment_SysVarSet ("DLEnv$Test", "hello", sysvar_STRING), "Environment_SysVarSet a string");
  memset (buf, 0, sizeof buf);
  check (Environment_SysVarRead ("DLEnv$Test", buf, sizeof buf) == TRUE && strcmp (buf, "hello") == 0, "Environment_SysVarRead returns it");
  checke (Environment_SysVarSet ("DLEnv$Num", "42", sysvar_NUMBER), "a number variable");
  memset (buf, 0, sizeof buf);
  Environment_SysVarRead ("DLEnv$Num", buf, sizeof buf);
  check (buf[0] == 42 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0, "its value is the word 42 (a number variable reads back as binary)");
  /* GS expansion, and the pointer it returns (an address through a char ** parameter) */
  end = (char *) (unsigned long) 0xDEADBEEF00000000ull;
  memset (buf, 0, sizeof buf);
  checke (Environment_ExpandString ("say <DLEnv$Test> now", buf, sizeof buf, &end), "Environment_ExpandString");
  check (strcmp (buf, "say hello now") == 0, "the variable is expanded");
  printf ("     end pointer %p\n", (void *) end);
  check (((unsigned long) end >> 32) == 0 || end == NULL, "the end pointer has no stale top half");
  checke (Environment_SysVarDelete ("DLEnv$Test"), "Environment_SysVarDelete");
  check (Environment_SysVarRead ("DLEnv$Test", buf, sizeof buf) == FALSE, "the variable is gone");
  Environment_SysVarDelete ("DLEnv$Num");
  checke (Environment_Command ("Set DLEnv$Cmd %s", "viacommand"), "Environment_Command with a format");
  memset (buf, 0, sizeof buf);
  check (Environment_SysVarRead ("DLEnv$Cmd", buf, sizeof buf) && strcmp (buf, "viacommand") == 0, "the command ran");
  checke (Environment__OS_CLI ("Unset DLEnv$Cmd"), "OS_CLI (the macro's function)");
  check (Environment_ModuleIsActive ("UtilityModule") != FALSE, "Environment_ModuleIsActive UtilityModule");
  check (Environment_ModuleIsActive ("NoSuchModuleHere") == FALSE, "a module that is not there");
  i = Environment_OSVersion ();
  printf ("     OS version %d\n", i);
  check (i >= 100, "Environment_OSVersion is a plausible number");
  {
    char **list = NULL;
    int n = 0, found = 0;
    e = Environment_TaskList (&list, &n);
    checke (e, "Environment_TaskList");
    printf ("     %d tasks\n", n);
    check (n > 0 && list != NULL, "at least one task");
    for (i = 0; i < n && list; i++) { if (i < 4) printf ("     task %s\n", list[i]); if (strcmp (list[i], "Filer") == 0) found = 1; }
    if (n > 0) check (found, "the Filer is among the tasks");
    Environment_TaskListFree (list, n);
  }
  check (Environment_TaskIsActive ("Filer") != FALSE, "Environment_TaskIsActive Filer");
  check (Environment_TaskIsActive ("NoSuchTask") == FALSE, "and a task that is not running");
  (void) b2;
  printf ("SUMMARY %d failed of %d\n", fails, checks);
  return fails;
}
