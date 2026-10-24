/* twecho.c - helper for tests/taskwindow: runs in a TaskWindow, prints whether TaskWindow_Check sees it, then echoes each line it reads from its input until it reads "quit". */
#include <stdio.h>
#include <string.h>
#include "DeskLib:TaskWindow.h"

int main (void)
{
  char line[128];
  setvbuf (stdout, NULL, _IONBF, 0);
  printf ("twecho: TaskWindow_Check=%d\n", TaskWindow_Check () != 0);
  while (fgets (line, sizeof line, stdin))
  {
    line[strcspn (line, "\r\n")] = 0;
    printf ("twecho: got [%s]\n", line);
    if (strcmp (line, "quit") == 0) break;
  }
  printf ("twecho: done\n");
  return 0;
}
