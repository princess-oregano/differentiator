#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdarg.h>
#include "log.h"

// Stream to print log to.
FILE *LOGSTREAM = nullptr;

void   // Add log_warning and log_error. Timestapms(log_time).
log(const char *format, ...)
{
        if (LOGSTREAM == nullptr)
                return;

        va_list arglist;

        va_start(arglist, format);

        vfprintf(LOGSTREAM, format, arglist);

        va_end(arglist);
}

// Prints starting lines to log.
static void
print_start_log()
{
        if (LOGSTREAM == nullptr)
                return;

        time_t rawtime;
        tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        log("<pre>\n\n");
        log("<h2>LOG FILE</h2>\n\n");
        log("TIME OF CREATION: %s\n\n", asctime(timeinfo));
        log("------------------------------------------------------------\n\n");
}

static void
print_end_log()
{
        if (LOGSTREAM == nullptr)
                return;

        time_t rawtime;
        struct tm * timeinfo;
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        log("------------------------------------------------------------\n\n");
        log("END OF LOG FILE\n\n");
        log("TIME OF CLOSURE: %s\n\n", asctime(timeinfo));
}

void
open_log(const char *filename)
{
        LOGSTREAM = fopen(filename, "w");
        setvbuf(LOGSTREAM, nullptr, _IONBF, 0);

        if (LOGSTREAM == nullptr) {
                fprintf(stderr, "Couldn't open log file.\n");
                return;
        }

        print_start_log();
}

FILE *
get_logfile()
{
        return LOGSTREAM;
}

void
include_graph(const char *path)
{
        if (LOGSTREAM == nullptr)
                return;

        FILE *test_ptr = fopen(path, "r");
        if (test_ptr != nullptr) {
                fclose(test_ptr);
        }
        else {
                fprintf(stderr, "Error: file '%s' is invalid.\n", path);
                return;
        }

        log("<img src=%s/>\n\n", path);
}

void
close_log()
{
        print_end_log();

        if (LOGSTREAM != nullptr)
                fclose(LOGSTREAM);
        else
                fprintf(stderr, "Error: LOGSTREAM == nullptr.\n");
}

