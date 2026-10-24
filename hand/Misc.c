/* Misc.c - eight veneers that return a pointer through a pointer parameter (the ARM code stored a 32 bit word there; here the whole 64 bit pointer is stored) or read arguments from the
   stack through ip: Environment_ExpandString, MessageTrans_Lookup, Module_Claim, Sprite_AppendP, Sprite_Get, Sprite_ReadControl, Sprite_Select, Wimp_BaseOfSprites.  Written from the assembler files. */
#include <stdint.h>
#include "dl64.h"
#define X 0x20000
#define ERR(r) ((os_error *) (uintptr_t) (r)[0])
#define PTR(v) ((void *) (uintptr_t) (uint32_t) (v))			/* a register that holds an address below 4 GB */

os_error *Environment_ExpandString (const char *string, char *buffer, int size, char **end)
{
  uint64_t r[10] = { 0 };
  r[0] = (uintptr_t) string; r[1] = (uintptr_t) buffer; r[2] = (uint32_t) size;
  if (swi_x (0x27 | X, r)) return ERR (r);				/* OS_GSTrans */
  if (end) *end = (char *) PTR (r[0]);
  return 0;
}

os_error *MessageTrans_Lookup (msgtrans_filedesc *filedesc, const char *token, char **buffer, int *size, const char *p1, const char *p2, const char *p3, const char *p4)
{
  uint64_t r[10] = { 0 };
  os_error *e;
  r[0] = (uintptr_t) filedesc; r[1] = (uintptr_t) token; r[2] = (uintptr_t) *buffer; r[3] = (uint32_t) *size;
  r[4] = (uintptr_t) p1; r[5] = (uintptr_t) p2; r[6] = (uintptr_t) p3; r[7] = (uintptr_t) p4;
  e = swi_x (0x41502 | X, r) ? ERR (r) : 0;				/* MessageTrans_Lookup */
  *buffer = (char *) PTR (r[2]); *size = (int) (uint32_t) r[3];		/* stored whether or not it failed, as the original did */
  return e;
}

os_error *Module_Claim (unsigned size, void **block)
{
  uint64_t r[10] = { 0 };
  r[0] = 6; r[3] = size;
  if (swi_x (0x1E | X, r)) return ERR (r);				/* OS_Module 6 */
  *block = PTR (r[2]);
  return 0;
}

os_error *Sprite_Get (sprite_area area, const char *name, int a3, int a4, int a5, int a6, int a7, sprite_header **result)
{
  uint64_t r[10] = { 0 };
  r[0] = 256 + 16; r[1] = (uintptr_t) area; r[2] = (uintptr_t) name; r[3] = (uint32_t) a3; r[4] = (uint32_t) a4; r[5] = (uint32_t) a5; r[6] = (uint32_t) a6; r[7] = (uint32_t) a7;
  if (swi_x (0x2E | X, r)) return ERR (r);				/* OS_SpriteOp 16 */
  *result = (sprite_header *) PTR (r[2]);
  return 0;
}

int Sprite_ReadControl (sprite_area area, int *size, sprite_header **first, sprite_header **free)
{
  uint64_t r[10] = { 0 };
  r[0] = 256 + 8; r[1] = (uintptr_t) area;
  if (swi_x (0x2E | X, r)) return 0;					/* OS_SpriteOp 8: R2 = size, R3 = the number of sprites, R4 = offset of the first, R5 = of the free space */
  if (size) *size = (int) (uint32_t) r[2];
  if (first) *first = (sprite_header *) ((char *) area + (uint32_t) r[4]);
  if (free) *free = (sprite_header *) ((char *) area + (uint32_t) r[5]);
  return (int) (uint32_t) r[3];
}

/* OS_SpriteOp 35 with the pointer form takes the address of the second sprite too; the DeskLib assembler passed its name, which the OS refuses ("Can't append sprite").  The second sprite is looked up by name first. */
os_error *Sprite_AppendP (sprite_area area, sprite_header *sprite1, const char *name2, int dir)
{
  uint64_t r[10] = { 0 };
  r[0] = 256 + 24; r[1] = (uintptr_t) area; r[2] = (uintptr_t) name2;
  if (swi_x (0x2E | X, r)) return ERR (r);				/* OS_SpriteOp 24: select name2 */
  r[3] = r[2]; r[0] = 512 + 35; r[1] = (uintptr_t) area; r[2] = (uintptr_t) sprite1; r[4] = (uint32_t) dir;
  if (swi_x (0x2E | X, r)) return ERR (r);				/* OS_SpriteOp 35, pointer form: R2 and R3 are both sprite pointers */
  return 0;
}

os_error *Sprite_Select (sprite_area area, const char *name, sprite_header **result)
{
  uint64_t r[10] = { 0 };
  r[0] = 256 + 24; r[1] = (uintptr_t) area; r[2] = (uintptr_t) name;
  if (swi_x (0x2E | X, r)) return ERR (r);				/* OS_SpriteOp 24 */
  if (result) *result = (sprite_header *) PTR (r[2]);
  return 0;
}

os_error *Wimp_BaseOfSprites (void **rom, void **ram)
{
  uint64_t r[10] = { 0 };
  if (swi_x (0x400EA | X, r)) return ERR (r);
  *rom = PTR (r[0]); *ram = PTR (r[1]);
  return 0;
}
