/* DragASpr.c - DragASprite_DragIcon (DragASpr/DragIcon.s), by hand for AArch64: drags an icon with DragASprite if the module is there and enabled, else with Wimp_DragBox */
#include <stdint.h>
#include <string.h>
#include "dl64.h"
#define X 0x20000

os_error *DragASprite_DragIcon (window_handle window, icon_handle icon)
{
  static uint32_t state[10];						/* the window state block, then the icon state follows at +8 (as in the original code) */
  static uint32_t drag[10];						/* window, type, bounding box (4), parent box (4) */
  uint64_t r[10];
  int32_t ox, oy;
  uint32_t flags;
  memset (state, 0, sizeof state); memset (drag, 0, sizeof drag);
  state[0] = (uint32_t) window;
  memset (r, 0, sizeof r); r[1] = (uintptr_t) state;
  if (swi_x (0x400CB | X, r)) return (os_error *) (uintptr_t) r[0];	/* Wimp_GetWindowState */
  ox = (int32_t) state[1] - (int32_t) state[5];				/* visible x0 - scroll x */
  oy = (int32_t) state[4] - (int32_t) state[6];				/* visible y1 - scroll y */
  drag[0] = (uint32_t) window; drag[1] = 5;
  state[1] = (uint32_t) icon;						/* the block still has the window handle */
  memset (r, 0, sizeof r); r[1] = (uintptr_t) state;
  if (swi_x (0x400CE | X, r)) return (os_error *) (uintptr_t) r[0];	/* Wimp_GetIconState: the icon block is at state + 8 */
  drag[2] = state[2] + (uint32_t) ox; drag[3] = state[3] + (uint32_t) oy; drag[4] = state[4] + (uint32_t) ox; drag[5] = state[5] + (uint32_t) oy;
  drag[6] = 0; drag[7] = 0; drag[8] = 0x7FFFFFFF; drag[9] = 0x7FFFFFFF;
  flags = state[2 + 4];							/* the icon flags: icon block + 16 */
  memset (r, 0, sizeof r); r[1] = (uintptr_t) "DragASprite_Start";
  if (!swi_x (0x39 | X, r))						/* OS_SWINumberFromString: is DragASprite there? */
  {
    memset (r, 0, sizeof r); r[0] = 161; r[1] = 28;
    swi_x (6 | X, r);							/* OS_Byte 161, 28: the DragASprite configuration */
    if ((uint32_t) r[2] & 2)						/* enabled */
    {
      uint64_t q[10] = { 0 };
      q[0] = 197;							/* centred, bbox = screen, limits the pointer, drop shadow */
      if (flags & 256) { q[1] = state[2 + 6]; q[2] = state[2 + 5]; }	/* indirected: the sprite area and the name */
      else { q[1] = 1; q[2] = (uintptr_t) &state[2 + 5]; }		/* the system area, the name inside the icon */
      q[3] = (uintptr_t) &drag[2]; q[4] = (uintptr_t) &drag[6];
      if (swi_x (0x42400 | X, q)) return (os_error *) (uintptr_t) q[0];	/* DragASprite_Start */
      return 0;
    }
  }
  memset (r, 0, sizeof r); r[1] = (uintptr_t) drag;
  if (swi_x (0x400D0 | X, r)) return (os_error *) (uintptr_t) r[0];	/* Wimp_DragBox */
  return 0;
}
