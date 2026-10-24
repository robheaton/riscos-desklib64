/*
 * This file is part of DeskLib, the C library for RISC OS.
 * Please see accompanying documentation for terms of use.
 *
 *       http://www.riscos.info/index.php/DeskLib
 *
 *
 * Module:  Environment
 * File:    SysVars.c
 * Author:  Tim Browse, Adam Richardson
 * Purpose: Functions to read & write environment variable values.
 *
 * Version History
 * 30/05/1994: Creation
 * 24/08/2007: Don't try and fill buffer if 0 passed as buffer length
 * 28/09/2007: Moved from KernelSWIs mode to Environment module, added SetSysVar,
 *             DeleteSysVar, ExpandSysVar
 *
 */

#include "DeskLib:Core.h"
#include "DeskLib:Environment.h"
#include "DeskLib:SWI.h"
#include <stdlib.h>
#include <string.h>

BOOL Environment_SysVarRead(const char *varname, char *buf, int bufsize)
{
  BOOL returnvalue = FALSE;
  int result = 0;
  os_error *error;

  /* Check for existence of this system variable.  With a buffer size of -1 the OS gives the error "Buffer overflow"
     (&1E4) for a variable that exists and "not found" (&124) for one that does not.  The output registers are not
     returned when a SWI gives an error (they were, in the Shared C Library's _kernel_swi, which the original code
     relied on: it tested R2, which here would be whatever was on the stack). */
  error = SWI(3, 3, SWI_OS_ReadVarVal, varname, buf, -1, NULL, NULL, &result);
  if (error == NULL || error->errnum == 0x1E4)
  {
    /* Variable exists, return TRUE */
    returnvalue = TRUE;

    if (bufsize && buf)
    {
      /* Valid buffer provided, read in the value */
      result = 0;
      error = SWI(5, 3, SWI_OS_ReadVarVal,
                  varname, buf, bufsize, 0, 0,
                  NULL, NULL, &result);

      /* Ensure correct termination */
      if (error == NULL && result >= 0 && result < bufsize)
        buf[result] = 0;
      else
        buf[bufsize - 1] = 0;         /* the value did not fit: cut it */
    }
  }

  return returnvalue;
}

os_error *Environment_SysVarSet(const char *varname, const char *value, sysvar_type type)
{
  os_error *error;
  int length, number;

  length = strlen(value);

  switch (type)
  {
    case sysvar_NUMBER:
      number = atoi(value);
      error = SWI(5, 0, SWI_OS_SetVarVal, varname, &number, 4, 0, type);   /* R1 points at the word (it was passed the number itself) */
      break;
    default:
      error = SWI(5, 0, SWI_OS_SetVarVal, varname, value, length, 0, type);
      break;
  }
  return error;
}

os_error *Environment_SysVarDelete(const char *varname)
{
  os_error *error;

  error = SWI(5, 0, SWI_OS_SetVarVal, varname, NULL, -1, 0, 0);

  return error;
}
