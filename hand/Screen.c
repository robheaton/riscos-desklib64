/* Screen.c - Screen_ReadModeVariable (Screen/ReadMode.s): OS_ReadModeVariable; the carry flag means an invalid mode or variable */
#include <stdint.h>
#include "dl64.h"

os_error *Screen_ReadModeVariable (int mode, mode_variable variable, int *result)
{
  static struct { int errnum; char errmess[36]; } bad = { 0, "Invalid mode or variable number" };
  uint64_t r[10] = { 0 };
  unsigned long f;
  r[0] = (uint32_t) mode; r[1] = (uint32_t) variable;
  f = swi_f (0x35, r);							/* XOS_ReadModeVariable */
  if (f & 1) return (os_error *) (uintptr_t) r[0];
  if (f & 2) return (os_error *) (void *) &bad;
  if (result) *result = (int) (uint32_t) r[2];
  return 0;
}
