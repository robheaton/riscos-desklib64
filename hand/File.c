/* File.c - DeskLib's File veneers (File/FileOps.s and File/GetType.s), written by hand for AArch64: OS_Find, OS_Args, OS_GBPB, OS_BGet, OS_BPut, OS_File */
#include <stdint.h>
#include <string.h>
#include "dl64.h"
#define X 0x20000

os_error *file_lasterror = 0;				/* the last file error (a pointer: 64 bits here) */

static os_error *swi (unsigned long no, uint64_t *r) { return swi_x (no | X, r) ? (os_error *) (uintptr_t) r[0] : 0; }

file_handle File_Open (const char *filename, file_access access)
{
  uint64_t r[10] = { 0 };
  os_error *e;
  r[0] = (uint32_t) access; r[1] = (uintptr_t) filename; r[2] = 0;
  e = swi (0x0D, r);						/* OS_Find */
  file_lasterror = e;
  return e ? 0 : (file_handle) (uint32_t) r[0];
}

os_error *File_Close (file_handle h)
{
  uint64_t r[10] = { 0 };
  r[0] = 0; r[1] = (uint32_t) h;
  return file_lasterror = swi (0x0D, r);
}

BOOL File_EOF (file_handle h)
{
  uint64_t r[10] = { 0 };
  os_error *e;
  r[0] = 5; r[1] = (uint32_t) h;
  e = swi (0x09, r);						/* OS_Args 5 */
  file_lasterror = e;
  return (BOOL) (uint32_t) r[2];
}

os_error *File_Seek (file_handle h, file_position position)
{
  uint64_t r[10] = { 0 };
  r[0] = 1; r[1] = (uint32_t) h; r[2] = (uint32_t) position;
  return file_lasterror = swi (0x09, r);
}

file_position File_ReturnPos (file_handle h)
{
  uint64_t r[10] = { 0 };
  r[0] = 0; r[1] = (uint32_t) h;
  file_lasterror = swi (0x09, r);
  return (file_position) (uint32_t) r[2];
}

int File_ReadExtent (file_handle h)
{
  uint64_t r[10] = { 0 };
  os_error *e;
  r[0] = 2; r[1] = (uint32_t) h;
  e = swi (0x09, r);
  file_lasterror = e;
  return e ? -1 : (int) (uint32_t) r[2];
}

os_error *File_WriteBytes (file_handle h, const void *buffer, int numbytes)
{
  uint64_t r[10] = { 0 };
  r[0] = 2; r[1] = (uint32_t) h; r[2] = (uintptr_t) buffer; r[3] = (uint32_t) numbytes;
  return file_lasterror = swi (0x0C, r);			/* OS_GBPB 2 */
}

int File_ReadBytes (file_handle h, void *buffer, int numbytes)
{
  uint64_t r[10] = { 0 };
  r[0] = 4; r[1] = (uint32_t) h; r[2] = (uintptr_t) buffer; r[3] = (uint32_t) numbytes;
  file_lasterror = swi (0x0C, r);				/* OS_GBPB 4 */
  return (int) (uint32_t) r[3];					/* the number of bytes not read */
}

int File_Read8 (file_handle h)
{
  uint64_t r[10] = { 0 };
  os_error *e;
  r[0] = 0; r[1] = (uint32_t) h;
  e = swi (0x0A, r);						/* OS_BGet */
  file_lasterror = e;
  return e ? -1 : (int) (uint32_t) r[0];
}

int File_Read32 (file_handle h)
{
  uint64_t r[10] = { 0 };
  static uint32_t word;						/* a buffer below 4 GB for the OS */
  os_error *e;
  word = 0;
  r[0] = 4; r[1] = (uint32_t) h; r[2] = (uintptr_t) &word; r[3] = 4;
  e = swi (0x0C, r);
  file_lasterror = e;
  return e ? -1 : (int) word;
}

int File_Read32R (file_handle h)
{
  uint32_t v = (uint32_t) File_Read32 (h);
  return (int) (((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) | (v >> 24));
}

os_error *File_Write8 (file_handle h, int byte)
{
  uint64_t r[10] = { 0 };
  r[0] = (uint32_t) byte; r[1] = (uint32_t) h;
  return file_lasterror = swi (0x0B, r);			/* OS_BPut */
}

os_error *File_Write32 (file_handle h, int word)
{
  uint64_t r[10] = { 0 };
  static uint32_t w;
  w = (uint32_t) word;
  r[0] = 2; r[1] = (uint32_t) h; r[2] = (uintptr_t) &w; r[3] = 4;
  return file_lasterror = swi (0x0C, r);
}

os_error *File_Write32R (file_handle h, int word)
{
  uint32_t v = (uint32_t) word;
  return File_Write32 (h, (int) (((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) | (v >> 24)));
}

int File_GetType (const char *filename)
{
  uint64_t r[10] = { 0 };
  r[0] = 17; r[1] = (uintptr_t) filename;
  if (swi (0x08, r)) return -1;					/* OS_File 17 */
  if (!((uint32_t) r[0] & 1)) return -1;			/* a file or an image file */
  return (int) (((uint32_t) r[2] << 12) >> 20);			/* the file type: bits 8 to 19 of the load address */
}
