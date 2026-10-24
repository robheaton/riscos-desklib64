/* Menu.c - Menu_FontMenu and Menu_FontMenuDecode (Menu/FontMenu3.s), by hand for AArch64: the Font Manager builds the menu in a buffer that we malloc */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dl64.h"
#define X 0x20000

static void *fontmenu_def, *fontmenu_ind, *answer;
static uint32_t answer_size;

static void free_data (void)
{
  if (fontmenu_def) free (fontmenu_def);
  if (fontmenu_ind) free (fontmenu_ind);
  fontmenu_def = fontmenu_ind = 0;
}

menu_ptr Menu_FontMenu (BOOL sysfont, char *tick)
{
  uint64_t r[10] = { 0 };
  uint32_t flags = sysfont ? (7u << 19) : (5u << 19);			/* 2_111000 or 2_101000, shifted by 16 */
  uint32_t menusize, indsize;
  void *def, *ind;
  free_data ();
  r[0] = 0; r[1] = 0; r[2] = flags; r[3] = 0; r[4] = 0; r[5] = 0; r[6] = (uintptr_t) tick;
  if (swi_x (0x40091 | X, r)) return (menu_ptr) fontmenu_def;		/* Font_ListFonts: the sizes */
  menusize = (uint32_t) r[3]; indsize = (uint32_t) r[5];
  def = malloc (menusize);
  if (!def) return (menu_ptr) fontmenu_def;
  ind = malloc (indsize);
  if (!ind) { free (def); return (menu_ptr) fontmenu_def; }
  fontmenu_ind = ind; fontmenu_def = def;
  memset (r, 0, sizeof r);
  r[0] = (uintptr_t) ind; r[1] = (uintptr_t) def; r[2] = flags; r[3] = menusize; r[4] = (uintptr_t) ind; r[5] = indsize; r[6] = (uintptr_t) tick;
  if (swi_x (0x40091 | X, r)) free_data ();				/* the menu itself */
  return (menu_ptr) fontmenu_def;
}

char *Menu_FontMenuDecode (int *selection)
{
  uint64_t r[10] = { 0 };
  uint32_t need;
  r[0] = 0; r[1] = (uintptr_t) fontmenu_def; r[2] = (uintptr_t) selection; r[3] = 0;
  if (swi_x (0x400A0 | X, r)) return 0;					/* Font_DecodeMenu: the size of the answer */
  need = (uint32_t) r[4];
  if (answer_size < need)
  {
    free (answer);
    answer = malloc (need);
    if (!answer) return 0;
    answer_size = need;
  }
  memset (r, 0, sizeof r);
  r[0] = 0; r[1] = (uintptr_t) fontmenu_def; r[2] = (uintptr_t) selection; r[3] = (uintptr_t) answer; r[4] = need;	/* r4 is the size of the buffer (the assembler left it from the first call) */
  if (swi_x (0x400A0 | X, r)) return 0;
  return (char *) answer;
}
