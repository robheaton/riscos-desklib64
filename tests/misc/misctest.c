/* misctest.c - DeskLib64 test, command line: Coord, LinkList, TextFile, Kbd, Pointer, Hourglass, Module, Screen and KernelSWIs (OS_Byte, OS_Word).  Prints one line per check and a summary; the exit status is the number of failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "DeskLib:Core.h"
#include "DeskLib:Coord.h"
#include "DeskLib:LinkList.h"
#include "DeskLib:TextFile.h"
#include "DeskLib:Kbd.h"
#include "DeskLib:Pointer.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Module.h"
#include "DeskLib:Screen.h"
#include "DeskLib:KernelSWIs.h"
#include "DeskLib:Wimp.h"

static int fails, checks;
static void settle (void) { int i, a, b; for (i = 0; i < 5; i++) OS_Byte (osbyte_WAITFORVSYNC, 0, 0, &a, &b); }   /* give the pointer time to move */
static void check (int ok, const char *what) { checks++; if (!ok) fails++; printf ("%s %s\n", ok ? "PASS" : "FAIL", what); }
static void checke (os_error *e, const char *what)
{
  checks++;
  if (e) { fails++; printf ("FAIL %s: error %d %s\n", what, e->errnum, e->errmess); } else printf ("PASS %s\n", what);
}

typedef struct { linklist_header header; int value; } node;

int main (int argc, char **argv)
{
  char buf[128], path[256];
  const char *base = argc > 1 ? argv[1] : "<Wimp$ScrapDir>";
  setvbuf (stdout, NULL, _IONBF, 0);

  /* ---- Coord ---- */
  {
    wimp_rect a = { {0, 0}, {100, 100} }, b = { {10, 10}, {50, 50} }, c = { {90, 90}, {200, 200} }, d = { {300, 300}, {400, 400} }, u;
    wimp_point in = { 50, 50 }, out = { 150, 50 }, edge = { 0, 0 }, p;
    convert_block cb = { { {200, 100}, {1000, 700} }, { 30, -40 } };
    wimp_point o;
    wimp_rect r;
    check (Coord_PointInRect (&in, &a) && !Coord_PointInRect (&out, &a), "Coord_PointInRect");
    check (Coord_PointInRect (&edge, &a), "a point on the minimum edge is inside");
    check (Coord_RectContained (&b, &a) && !Coord_RectContained (&a, &b) && !Coord_RectContained (&c, &a), "Coord_RectContained");
    check (Coord_RectsOverlap (&a, &c) && Coord_RectsOverlap (&a, &b) && !Coord_RectsOverlap (&a, &d), "Coord_RectsOverlap");
    check (Coord_RectsIntersect (&a, &c) && !Coord_RectsIntersect (&a, &b) && !Coord_RectsIntersect (&a, &d), "Coord_RectsIntersect");
    Coord_RectUnion (&u, &a, &d);
    check (u.min.x == 0 && u.min.y == 0 && u.max.x == 400 && u.max.y == 400, "Coord_RectUnion");
    Coord_RectUnion (&u, &b, &c);
    check (u.min.x == 10 && u.min.y == 10 && u.max.x == 200 && u.max.y == 200, "Coord_RectUnion of two overlapping rectangles");
    Coord_WindowOrigin (&o, &cb);
    check (o.x == 200 - 30 && o.y == 700 + 40, "Coord_WindowOrigin");
    p.x = 5; p.y = -10; Coord_PointToScreen (&p, &cb);
    check (p.x == Coord_XToScreen (5, &cb) && p.y == Coord_YToScreen (-10, &cb) && p.x == 175 && p.y == 730, "Coord_PointToScreen");
    Coord_PointToWorkArea (&p, &cb);
    check (p.x == 5 && p.y == -10, "Coord_PointToWorkArea is the inverse");
    r = a; Coord_RectToScreen (&r, &cb);
    check (r.min.x == 170 && r.min.y == 740 && r.max.x == 270 && r.max.y == 840, "Coord_RectToScreen");
    Coord_RectToWorkArea (&r, &cb);
    check (r.min.x == 0 && r.min.y == 0 && r.max.x == 100 && r.max.y == 100, "Coord_RectToWorkArea is the inverse");
  }

  /* ---- LinkList ---- */
  {
    linklist_header head;
    node n[5];
    int i, sum;
    void *it;
    for (i = 0; i < 5; i++) { n[i].value = i; LinkList_InitItem (&n[i].header); }
    LinkList_Init (&head);
    check (LinkList_ListLength (&head) == 0 && LinkList_FirstItem (&head) == NULL, "an empty list");
    LinkList_AddToTail (&head, &n[1].header);
    LinkList_AddToTail (&head, &n[2].header);
    LinkList_AddToHead (&head, &n[0].header);
    check (LinkList_ListLength (&head) == 3 && LinkList_FirstItem (&head) == &n[0] && LinkList_LastItem (&head) == &n[2], "AddToHead and AddToTail");
    LinkList_InsertAfter (&head, &n[1].header, &n[3].header);
    LinkList_InsertBefore (&head, &n[0].header, &n[4].header);
    for (sum = 0, i = 0, it = LinkList_FirstItem (&head); it; it = LinkList_NextItem (it), i++) sum = sum * 10 + ((node *) it)->value;
    check (i == 5 && sum == 40132, "InsertBefore and InsertAfter: order 4 0 1 3 2");
    sum = 0;
    for (it = LinkList_LastItem (&head); it; it = LinkList_PreviousItem (it)) sum = sum * 10 + ((node *) it)->value;
    check (sum == 23104, "the previous pointers run the other way");
    check (LinkList_InList (&head, &n[3].header) && LinkList_InList (&head, &n[4].header), "InList finds members");
    LinkList_Unlink (&head, &n[3].header);
    LinkList_Unlink (&head, &n[4].header);
    check (!LinkList_InList (&head, &n[3].header) && LinkList_ListLength (&head) == 3 && LinkList_FirstItem (&head) == &n[0], "Unlink (the head and a middle item)");
    LinkList_Unlink (&head, &n[2].header);
    LinkList_Unlink (&head, &n[0].header);
    LinkList_Unlink (&head, &n[1].header);
    check (LinkList_ListLength (&head) == 0 && head.next == NULL && head.previous == NULL, "all unlinked");
  }

  /* ---- TextFile ---- */
  {
    FILE *f;
    char line[64];
    snprintf (path, sizeof path, "%s.dlmisc", base);
    f = fopen (path, "w");
    check (f != NULL, "open a scratch text file");
    if (f)
    {
      fputs ("   \t  first line\n  second,part;third\nlast", f);
      fclose (f);
      f = fopen (path, "r");
      TextFile_ReadToDelimiter (f, '\n', line, sizeof line);
      check (strcmp (line, "first line") == 0, "ReadToDelimiter skips leading blanks and stops at the newline");
      TextFile_ReadToDelimiter (f, ',', line, sizeof line);
      check (strcmp (line, "second") == 0, "a delimiter other than the newline");
      TextFile_ReadToDelimiter (f, ';', line, 4);
      check (strcmp (line, "par") == 0, "a result cut at maxlength - 1");
      TextFile_ReadToDelimiter (f, '\n', line, sizeof line);
      TextFile_ReadToDelimiter (f, '\n', line, sizeof line);
      check (strcmp (line, "last") == 0 && feof (f), "the last line without a newline ends at EOF");
      fclose (f);
      remove (path);
    }
    check (TextFile_Lowercase ('Q') == 'q' && TextFile_Lowercase ('q') == 'q' && TextFile_Lowercase ('5') == '5', "TextFile_Lowercase");
  }

  /* ---- Screen and KernelSWIs ---- */
  {
    int v, xe, ye, xw, yw, l2, bpp, r1, r2;
    check (Screen_CacheModeInfo () == TRUE, "Screen_CacheModeInfo");
    checke (Screen_ReadModeVariable (-1, SCREEN_VAR_XEigFactor, &xe), "Screen_ReadModeVariable, current mode");
    Screen_ReadModeVariable (-1, SCREEN_VAR_YEigFactor, &ye);
    Screen_ReadModeVariable (-1, SCREEN_VAR_XWindLimit, &xw);
    Screen_ReadModeVariable (-1, SCREEN_VAR_YWindLimit, &yw);
    Screen_ReadModeVariable (-1, SCREEN_VAR_Log2BPP, &l2);
    printf ("     mode %d, %d x %d OS units, eig %d %d, %d bpp\n", (int) screen_mode.screen_mode, screen_size.x, screen_size.y, screen_eig.x, screen_eig.y, screen_bpp);
    check (screen_eig.x == xe && screen_eig.y == ye, "screen_eig is the mode's eigen factors");
    check (screen_size.x == ((xw + 1) << xe) && screen_size.y == ((yw + 1) << ye), "screen_size is (limit + 1) << eig");
    check (screen_delta.x == (1 << xe) && screen_delta.y == (1 << ye), "screen_delta is a pixel in OS units");
    bpp = 1 << l2;
    check (screen_bpp == bpp && bpp >= 8 && bpp <= 32, "screen_bpp is 1 << Log2BPP");
    checke (OS_Byte (osbyte_READVDUVARIABLE, SCREEN_VAR_XEigFactor, 0, &r1, &r2), "OS_Byte 160 (read a VDU variable)");
    printf ("     eig %d, R1 %d, R2 %d\n", xe, r1, r2);
    check (r1 >= 0 && r1 <= 255 && r2 >= 0 && r2 <= 255, "R1 and R2 are the low and high bytes of the value (a BBC VDU variable number, not a mode variable)");
    checke (OS_Byte (osbyte_RW_BELLVOLUME, 0, 255, &r1, &r2), "OS_Byte 212 (read the bell volume)");
    v = r1;
    checke (OS_Byte (osbyte_RW_BELLVOLUME, 0, 255, &r1, &r2), "OS_Byte read again");
    check (r1 == v, "the same value both times");
    checke (OS_Byte (osbyte_RW_ESCAPECHARACTER, 0, 255, &r1, &r2), "OS_Byte 220 (read the escape character)");
    check (r1 == 27, "it is 27");
    r1 = r2 = 0x12345678;
    checke (OS_Byte (osbyte_RW_50HZCOUNTER, 0, 255, &r1, &r2), "OS_Byte 176 (read the 50 Hz counter)");
    check (r1 >= 0 && r1 <= 255 && r2 >= 0 && r2 <= 255, "the results are one byte each");
    {
      unsigned char c1[5], c2[5];
      unsigned long t1 = 0, t2 = 0;
      int i;
      memset (c1, 0, 5); memset (c2, 0, 5);
      c1[0] = 1; c2[0] = 1;
      checke (OS_Word (osword_READSYSCLOCK, c1), "OS_Word 1 (read the system clock)");
      for (i = 0; i < 100000; i++) ;
      checke (OS_Word (osword_READSYSCLOCK, c2), "OS_Word 1 again");
      for (i = 4; i >= 0; i--) { t1 = t1 << 8 | c1[i]; t2 = t2 << 8 | c2[i]; }
      printf ("     clock %lu then %lu\n", t1, t2);
      check (t1 > 0 && t2 >= t1 && t2 - t1 < 500, "the clock is running and does not go backwards");
    }
  }

  /* ---- Module ---- */
  {
    void *block = (void *) (uintptr_t) 0xDEADBEEF00000000ull;
    os_error *e;
    e = Module_Claim (1000, &block);
    checke (e, "Module_Claim 1000 bytes");
    check (((uintptr_t) block >> 32) == 0 && block != NULL, "the block is an address below 4 GB (no stale top half)");
    if (block) { memset (block, 0xA5, 1000); check (((unsigned char *) block)[999] == 0xA5, "the RMA block is writable"); }
    checke (Module_Free (block), "Module_Free");
    block = NULL;
    e = Module_Claim (0xFFFFFFF0u, &block);
    check (e != NULL, "an impossible size (4 GB) gives an error");
    if (e == NULL && block) Module_Free (block);
  }

  /* ---- Pointer ---- */
  {
    wimp_point pos = { 0, 0 }, want = { 400, 300 };
    wimp_rect box = { {100, 100}, {600, 500} };
    os_error *e;
    e = Pointer_GetPosition (&pos);
    checke (e, "Pointer_GetPosition");
    printf ("     pointer at %d, %d\n", pos.x, pos.y);
    check (pos.x >= 0 && pos.x < 20000 && pos.y >= 0 && pos.y < 20000, "a plausible position");
    checke (Pointer_SetPosition (want), "Pointer_SetPosition");
    settle (); Pointer_GetPosition (&pos);
    printf ("     set 400, 300: read %d, %d\n", pos.x, pos.y);
    check (abs (pos.x - 400) <= 4 && abs (pos.y - 300) <= 4, "it reads back (to a pixel)");
    checke (Pointer_RestrictToRect (box), "Pointer_RestrictToRect");
    want.x = 5000; want.y = 5; Pointer_SetPosition (want); settle (); Pointer_GetPosition (&pos);
    printf ("     restricted: set 5000, 5: read %d, %d\n", pos.x, pos.y);
    check (pos.x <= 600 && pos.y >= 100, "the pointer cannot leave the rectangle");
    checke (Pointer_Unrestrict (), "Pointer_Unrestrict");
    want.x = 1000; want.y = 900; Pointer_SetPosition (want); settle (); Pointer_GetPosition (&pos);
    printf ("     set 1000, 900: read %d, %d\n", pos.x, pos.y);
    check (pos.x > 700, "the pointer can leave it again");
    want.x = 600; want.y = 400; Pointer_SetPosition (want);
  }

  /* ---- Kbd ---- */
  {
    kbd_modifiers m;
    m = Kbd_GetModifiers (TRUE);
    printf ("     modifiers: alt %d ctrl %d shift %d\n", m.alt, m.ctrl, m.shift);
    check (m.alt == 0 && m.ctrl == 0 && m.shift == 0, "no modifier keys held (detailed)");
    m = Kbd_GetModifiers (FALSE);
    check (m.alt == 0 && m.ctrl == 0 && m.shift == 0, "no modifier keys held (simple)");
    check (Kbd_KeyDown (inkey_SHIFT) == FALSE && Kbd_KeyDown (inkey_CTRL) == FALSE && Kbd_KeyDown (inkey_ALT) == FALSE && Kbd_KeyDown (inkey_SELECT) == FALSE, "Kbd_KeyDown for the modifier keys and the Select button (none held)");
    check (Kbd_KeyDown (-1 - 0x4F) == FALSE, "Kbd_KeyDown for an ordinary key nobody holds (internal key 0x4F)");
  }

  /* ---- Hourglass (no output to check; it must not fault) ---- */
  Hourglass_On ();
  Hourglass_Percentage (40);
  Hourglass_Start (5);
  Hourglass_Percentage (90);
  Hourglass_Off ();
  Hourglass_Smash ();
  check (1, "Hourglass On, Start, Percentage, Off and Smash run");

  printf ("SUMMARY %d failed of %d\n", fails, checks);
  return fails;
}
