#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "system.h"

void
system_wformat(const char *format, ...)
{
        va_list arglist;

        va_start(arglist, format);

        char *cmd = (char *) calloc(255, sizeof(char));
        if (cmd == nullptr) {
                fprintf(stderr, "Couldn't allocate memory for system cmd.\n");
                va_end(arglist);
                return;
        }

        vsprintf(cmd, format, arglist);
        system(cmd);

        va_end(arglist);

        free(cmd);
}

