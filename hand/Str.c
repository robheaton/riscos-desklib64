/* Str.c - strcatcr and strcpycr (Str/*.s): string functions that end at a control character (code below 32) */
#include "dl64.h"

char *strcatcr (char *s1, const char *s2)
{
  char *d = s1;
  while ((unsigned char) *d > 31) d++;
  while ((unsigned char) *s2 > 31) *d++ = *s2++;
  *d = 0;
  return s1;
}

char *strcpycr (char *s1, const char *s2)
{
  int i = 0;
  while ((unsigned char) s2[i] > 31) { s1[i] = s2[i]; i++; }
  s1[i] = 0;
  return s1;
}
