/* filetest.c - DeskLib64 test, command line: the File and Filing libraries (OS_File, OS_Find, OS_GBPB, OS_Args, OS_FSControl veneers and the SWI () macros).  It works in a scratch directory made inside the directory given as the first argument (default <Wimp$ScrapDir>) and removes it at the end.  Prints one line per check and a summary; the exit status is the number of failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DeskLib:Core.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"

static int fails, checks;
static void check (int ok, const char *what) { checks++; if (!ok) fails++; printf ("%s %s\n", ok ? "PASS" : "FAIL", what); }
static void checke (os_error *e, const char *what)
{
  checks++;
  if (e) { fails++; printf ("FAIL %s: error %d %s\n", what, e->errnum, e->errmess); } else printf ("PASS %s\n", what);
}

static int found_files, found_dirs, started, ended;
static os_error *start_dir (const char *dirname, filing_fulldirentry *d) { (void) dirname; (void) d; started++; return NULL; }
static os_error *found_file (const char *dirname, filing_fulldirentry *d) { (void) dirname; if (d->objtype == filing_DIRECTORY) found_dirs++; else found_files++; return NULL; }
static os_error *end_dir (const char *dirname, filing_fulldirentry *d) { (void) dirname; (void) d; ended++; return NULL; }

int main (int argc, char **argv)
{
  char base[200], dir[260], f1[300], f2[300], f3[300], sub[300], buf[512], leaf[64], path[300];
  const char data[] = "0123456789abcdef";
  file_handle h;
  int size = 0, n, i, off, number;
  os_error *e;
  unsigned char date[5];
  filing_objtype ot;
  int load, exec, len, attr, ftype;
  filing_dirdata dd;
  static char rdbuf[2048];
  char *text;

  strncpy (base, argc > 1 ? argv[1] : "<Wimp$ScrapDir>", sizeof base - 1);
  base[sizeof base - 1] = 0;
  snprintf (dir, sizeof dir, "%s.dltest", base);
  snprintf (f1, sizeof f1, "%s.one", dir);
  snprintf (f2, sizeof f2, "%s.two", dir);
  snprintf (f3, sizeof f3, "%s.three", dir);
  snprintf (sub, sizeof sub, "%s.sub", dir);
  printf ("scratch directory %s\n", dir);

  /* make the directory (File_CreateDir is a SWI () macro: its arguments go through varargs) */
  File_Delete (f1); File_Delete (f2); File_Delete (f3); File_Delete (sub); File_Delete (dir);
  checke (File_CreateDir (dir), "File_CreateDir (a SWI () macro)");
  check (!File_Exists (dir), "File_Exists is false for a directory (as documented)");
  check (File_IsDirectory (dir), "File_IsDirectory is true for it");
  checke (File_CreateDir (sub), "a sub directory");

  /* write a file with every kind of write */
  h = File_Open (f1, file_WRITE);
  check (h != 0 && h != file_READERROR, "File_Open for writing");
  checke (File_WriteBytes (h, data, 16), "File_WriteBytes 16 bytes");
  checke (File_Write8 (h, 'Z'), "File_Write8");
  checke (File_Write32 (h, 0x11223344), "File_Write32");
  checke (File_Write32R (h, 0x11223344), "File_Write32R (reversed)");
  n = File_printf (h, "line %d %s\n", 42, "text");
  check (n == 13, "File_printf returned 13");
  check (File_ReturnPos (h) == 16 + 1 + 4 + 4 + 13, "File_ReturnPos after the writes is 38");
  check (File_ReadExtent (h) == 38, "File_ReadExtent is 38");
  checke (File_Close (h), "File_Close");

  check (File_Exists (f1), "File_Exists");
  check (!File_Exists (f2), "File_Exists is false for a missing file");
  check (!File_IsDirectory (f1), "File_IsDirectory is false for a file");
  check (File_Size (f1) == 38, "File_Size is 38");
  checke (File_GetLength (f1, &size), "File_GetLength (a SWI () macro with a pointer)");
  check (size == 38, "File_GetLength gave 38");

  /* read it back */
  h = File_Open (f1, file_READ);
  check (h != 0, "File_Open for reading");
  memset (buf, 0, sizeof buf);
  n = File_ReadBytes (h, buf, 16);
  check (n == 0 && memcmp (buf, data, 16) == 0, "File_ReadBytes: 0 bytes not read, and the 16 bytes are right");
  check (File_Read8 (h) == 'Z', "File_Read8");
  check ((unsigned) File_Read32 (h) == 0x11223344u, "File_Read32");
  check ((unsigned) File_Read32R (h) == 0x11223344u, "File_Read32R");
  checke (File_Seek (h, 4), "File_Seek 4");
  check (File_ReturnPos (h) == 4, "File_ReturnPos is 4");
  check (File_Read8 (h) == '4', "the byte at 4 is '4'");
  check (!File_EOF (h), "not at the end");
  checke (File_Seek (h, 38), "File_Seek to the end");
  check (File_EOF (h), "File_EOF at 38");
  checke (File_Close (h), "File_Close (read)");

  /* append */
  h = File_Open (f1, file_APPEND);
  check (h != 0, "File_Open for append");
  checke (File_Seek (h, 38), "seek to the end");
  checke (File_WriteBytes (h, "XY", 2), "append two bytes");
  checke (File_Close (h), "close");
  check (File_Size (f1) == 40, "the size is now 40");

  /* load */
  memset (rdbuf, 0, sizeof rdbuf);
  checke (File_LoadTo (f1, rdbuf, &size), "File_LoadTo");
  check (size == 40 && memcmp (rdbuf, data, 16) == 0 && rdbuf[38] == 'X', "the loaded data is right");
  text = File_AllocLoad0 (f1);
  check (text != NULL && strlen (text) >= 16 && memcmp (text, data, 16) == 0, "File_AllocLoad0 returned the data");
  if (text) { check (text[40] == 0, "and it is zero terminated"); free (text); }

  /* types and dates */
  File_SetType (f1, 0xFFF);
  check (File_GetType (f1) == 0xFFF, "File_SetType / File_GetType text");
  File_SetType (f1, 0xFFD);
  check (File_GetType (f1) == 0xFFD, "type changed to data");
  check (File_GetType (dir) == -1, "File_GetType of a directory is -1 (not a file)");
  memset (date, 0, sizeof date);
  File_Date (f1, date);
  check (date[0] || date[1] || date[2] || date[3] || date[4], "File_Date is not zero");
  printf ("     date %02x%02x%02x%02x%02x\n", date[4], date[3], date[2], date[1], date[0]);

  /* copy and delete (File_Copy is a SWI () macro with two pointers) */
  checke (File_Copy (f1, f2, 0), "File_Copy (a SWI () macro with two pointers)");
  check (File_Exists (f2) && File_Size (f2) == 40, "the copy exists with the same size");
  checke (File_Copy (f1, f3, 0), "second copy");
  checke (File_Delete (f3), "File_Delete");
  check (!File_Exists (f3), "it is gone");
  e = File_Delete (f3);
  check (e == NULL || e != NULL, "deleting a missing file does not crash");

  /* Filing: catalogue and single entries */
  checke (Filing_ReadCatalogue (f1, &ot, &load, &exec, &len, &attr, &ftype), "Filing_ReadCatalogue");
  check (ot == filing_FILE && len == 40 && ftype == 0xFFD, "a file, length 40, type data");
  printf ("     load %08x exec %08x attr %x\n", (unsigned) load, (unsigned) exec, (unsigned) attr);
  checke (Filing_ReadCatalogue (dir, &ot, &load, &exec, &len, &attr, &ftype), "catalogue of the directory");
  check (ot == filing_DIRECTORY, "a directory");
  checke (Filing_ReadCatalogue (f3, &ot, &load, &exec, &len, &attr, &ftype), "catalogue of a missing file");
  check (ot == filing_NOTFOUND, "not found");
  {
    filing_direntry de;
    filing_fulldirentry fe;
    memset (&de, 0, sizeof de); memset (&fe, 0, sizeof fe);
    checke (Filing_SingleDirEntry (f1, &de, sizeof de), "Filing_SingleDirEntry");
    check (de.length == 40 && de.objtype == filing_FILE && strcmp (de.name, "one") == 0, "length 40, a file named one");
    checke (Filing_SingleFullDirEntry (f1, &fe, sizeof fe), "Filing_SingleFullDirEntry");
    printf ("     full entry: length %d type %d SIN %x name '%s' date %02x%02x%02x%02x%02x\n", fe.length, (int) fe.objtype, (unsigned) fe.SIN, fe.name, (unsigned) fe.date[4] & 255, (unsigned) fe.date[3] & 255, (unsigned) fe.date[2] & 255, (unsigned) fe.date[1] & 255, (unsigned) fe.date[0] & 255);
    check (fe.length == 40 && fe.objtype == filing_FILE && strcmp (fe.name, "one") == 0, "full entry: length, type and name (SIN is file system dependent: not checked)");
  }

  /* Filing: reading a directory three ways */
  memset (&dd, 0, sizeof dd);
  checke (Filing_OpenDir (dir, &dd, 1024, readdirtype_NAMEONLY), "Filing_OpenDir (names)");
  n = 0;
  while (Filing_ReadDir (&dd) != NULL) { n++; printf ("     name %s\n", dd.act.name); }
  printf ("     read %d names, read count %d, offset %d\n", n, dd.read, dd.offset);
  check (n == 3, "three objects: one, two, sub");
  check (Filing_CloseDir (&dd) != 0, "Filing_CloseDir");

  memset (&dd, 0, sizeof dd);
  checke (Filing_OpenDir (dir, &dd, 2048, readdirtype_DIRENTRY), "Filing_OpenDir (entries)");
  n = 0; i = 0;
  while (Filing_ReadDir (&dd) != NULL)
  {
    n++;
    if (dd.act.direntry->objtype == filing_DIRECTORY) i++;
    printf ("     %s type %d length %d\n", dd.act.direntry->name, (int) dd.act.direntry->objtype, dd.act.direntry->length);
  }
  printf ("     read %d entries of which %d directories\n", n, i);
  check (n == 3 && i == 1, "three entries, one a directory");
  Filing_CloseDir (&dd);

  memset (&dd, 0, sizeof dd);
  checke (Filing_OpenDir (dir, &dd, 2048, readdirtype_FULLDIRENTRY), "Filing_OpenDir (full entries)");
  n = 0;
  while (Filing_ReadDir (&dd) != NULL) n++;
  printf ("     read %d full entries\n", n);
  check (n == 3, "three full entries");
  Filing_CloseDir (&dd);

  /* the raw GBPB veneers */
  number = 100; off = 0;
  memset (rdbuf, 0, sizeof rdbuf);
  checke (Filing_ReadDirNames (dir, rdbuf, &number, &off, sizeof rdbuf, NULL), "Filing_ReadDirNames");
  check (number == 3, "it read 3 names");
  {
    filing_direntry ents[8];
    number = 100; off = 0;
    memset (ents, 0, sizeof ents);
    checke (Filing_ReadDirEntry (dir, ents, &number, &off, sizeof ents, "t*"), "Filing_ReadDirEntry with the match t*");
    check (number == 1, "only two matches t*: 'two' (one)");
  }

  /* Filing: names */
  check (Filing_GetPathname (f1, path) != NULL && strcmp (path, dir) == 0, "Filing_GetPathname");
  check (Filing_GetLeafname (f1, leaf) != NULL && strcmp (leaf, "one") == 0, "Filing_GetLeafname");
  check (Filing_FindLeafname (f1) != NULL && strcmp (Filing_FindLeafname (f1), "one") == 0, "Filing_FindLeafname");
  check (Filing_MakePath (path, dir, "three") != NULL && strcmp (path, f3) == 0, "Filing_MakePath");
  {
    int spare = 0;
    memset (buf, 0, sizeof buf);
    checke (Filing_CanonicalisePath (f1, buf, sizeof buf, &spare), "Filing_CanonicalisePath");
    check (strlen (buf) > 0 && strstr (buf, "one") != NULL, "a canonical name ending in one");
    printf ("     canonical: %s\n", buf);
  }

  /* Filing_ScanDir: the whole tree */
  started = ended = found_files = found_dirs = 0;
  checke (Filing_ScanDir (dir, start_dir, found_file, end_dir), "Filing_ScanDir");
  check (found_files == 2 && found_dirs == 1, "found two files and one directory");
  check (started == ended && started >= 1, "every directory started was ended");
  printf ("     started %d ended %d files %d dirs %d\n", started, ended, found_files, found_dirs);

  /* tidy up */
  File_Delete (f1); File_Delete (f2); File_Delete (sub);
  e = File_Delete (dir);
  check (e == NULL && !File_Exists (dir), "the scratch directory is removed");

  printf ("SUMMARY %d failed of %d\n", fails, checks);
  return fails;
}
