/*
    ####             #    #     # #
    #   #            #    #       #          The FreeWare C library for
    #   #  ##   ###  #  # #     # ###             RISC OS machines
    #   # #  # #     # #  #     # #  #   ___________________________________
    #   # ####  ###  ##   #     # #  #
    #   # #        # # #  #     # #  #    Please refer to the accompanying
    ####   ### ####  #  # ##### # ###    documentation for conditions of use
    ________________________________________________________________________

    File:    Template.Clone.c
    Author:  Copyright © 1992 Jason Williams
             Thanks to John Winters for supplying the code that I hacked
             changed, hacked, rewrote, and then wrote again from scratch!
    Version: 1.11 (13 Jul 1993)
    Purpose: Loading, cacheing, and retrieval of window templates
*/


#include "TempDefs.h"

/* Compile-time layout guard (DeskLib32): the window block must match the
   Wimp's format -- wimp_colourflags 8 bytes, window_block header 88 bytes --
   or copying templates corrupts memory.  If a compiler lays the structs out
   differently, one of these array sizes becomes -1 and the build stops here. */
typedef char desklib_check_colourflags[(sizeof(wimp_colourflags) == 8) ? 1 : -1];
typedef char desklib_check_windowblock[(sizeof(window_block) == 88) ? 1 : -1];



extern window_block *Template_Clone(const char *name, int maxtitlesize)
{
  template_record *t;
  window_block    *window;
  unsigned int     icon;
  icon_block      *icons;

  t = Template__FindTemplate(name);
  if (t == NULL)
    return(NULL);

  window = (window_block *) malloc(sizeof(window_block) +
                                (sizeof(icon_block) * t->windowdef->numicons));
  if (window == NULL)
    Error_ReportFatalInternal(ERR1, ERRMESS1);

  memcpy(window, t->windowdef, sizeof(window_block) +
                                (sizeof(icon_block) * t->windowdef->numicons));

  if (window->titleflags.data.indirected)
  {
    if (maxtitlesize == template_TITLEMIN)
      maxtitlesize = window->title.indirecttext.bufflen;
    else if (maxtitlesize < 1)
      maxtitlesize = 260;                         /* default ind. title size */

    window->title.indirecttext.buffer = DL_PTR ((char *) malloc(maxtitlesize));
    window->title.indirecttext.bufflen = maxtitlesize;
    if (((char *) DL_P (window->title.indirecttext.buffer)) == NULL)
      Error_ReportFatalInternal(ERR1, ERRMESS1);

    strncpy(((char *) DL_P (window->title.indirecttext.buffer)),
            ((char *) DL_P (t->windowdef->title.indirecttext.buffer)),
            window->title.indirecttext.bufflen-1);
    ((char *) DL_P (window->title.indirecttext.buffer))[window->title.indirecttext.bufflen-1] =0;
  }

  icons = (icon_block *) ((intptr_t)window + sizeof(window_block));
  for (icon = 0; icon < t->windowdef->numicons; icon++)
  {
    if (icons[icon].flags.data.indirected && icons[icon].flags.data.text)
    {
      char *oldtext = ((char *) DL_P (icons[icon].data.indirecttext.buffer));

      icons[icon].data.indirecttext.buffer = DL_PTR (
                                malloc(icons[icon].data.indirecttext.bufflen));
      if (((char *) DL_P (icons[icon].data.indirecttext.buffer)) == NULL)
        Error_ReportFatalInternal(ERR1, ERRMESS1);

      strncpy(((char *) DL_P (icons[icon].data.indirecttext.buffer)),
              oldtext, icons[icon].data.indirecttext.bufflen - 1);
      ((char *) DL_P (icons[icon].data.indirecttext.buffer))[
               icons[icon].data.indirecttext.bufflen - 1] = 0;

      oldtext = ((char *) DL_P (icons[icon].data.indirecttext.validstring));
      if ((intptr_t)oldtext > 0 && (uintptr_t)oldtext != 0xFFFFFFFFu)   /* a stored -1 means no validation string */
      {
        int size;

        size = strlencr(oldtext);
        icons[icon].data.indirecttext.validstring = DL_PTR ((char *) malloc(size + 1));
        if (((char *) DL_P (icons[icon].data.indirecttext.validstring)) == NULL)
          Error_ReportFatalInternal(ERR1, ERRMESS1);

        strncpy(((char *) DL_P (icons[icon].data.indirecttext.validstring)), oldtext, size);
        ((char *) DL_P (icons[icon].data.indirecttext.validstring))[size] = 0;
      }
    }
  }

  return(window);
}
