/*
    ####             #    #     # #
    #   #            #    #       #          The FreeWare C library for
    #   #  ##   ###  #  # #     # ###             RISC OS machines
    #   # #  # #     # #  #     # #  #   ___________________________________
    #   # ####  ###  ##   #     # #  #
    #   # #        # # #  #     # #  #    Please refer to the accompanying
    ####   ### ####  #  # ##### # ###    documentation for conditions of use
    ________________________________________________________________________

    File:    Font.c.ScanString
    Author:  Copyright © 2003 Antony Sidwell
    Version: 1.00 (29 Jan 2003)
    Purpose: Veneer for Font_ScanString.
             Eventually should be converted into assembler.
*/


#include <stdint.h>
#include "DeskLib:Font.h"
#include "DeskLib:SWI.h"

#ifndef SWI_Font_ScanString
#define SWI_Font_ScanString 0x400A1
#endif


os_error *Font_ScanString(font_handle font,
                          const char *string,
                          char **end,
                          unsigned int flags,
                          int *x, int *y,
                          font_scanstringblock *block,
                          font_transformation *transformation,
                          int *length)
{
  /* r1 on exit is an address: SWI () stores a register as a 4 byte int, so take it in an int and widen it, instead of
     passing 'end' (a char **, 8 bytes on 64 bits, whose top half would be left as it was) */
  int endaddr = 0;
  os_error *e = SWI(8, 8, SWI_Font_ScanString,
                    font, string, flags, *x, *y, block, transformation, *length,
                    NULL, &endaddr, NULL, x, y, NULL, NULL, length);
  if (end != NULL)
    *end = (char *) (uintptr_t) (unsigned int) endaddr;
  return e;
}


