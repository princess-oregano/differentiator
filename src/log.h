#ifndef LOG_H
#define LOG_H

#include <stdio.h>

// Open log file.
void
open_log(const char *filename);
// Write message to log.
void
log(const char *format, ...);
// Return file pointer of current log file stream.
FILE *
get_logfile();
// Includes graphic object into log file.
void
include_graph(const char *path);
// Close current log file stream.
void
close_log();

#endif // LOG_H

