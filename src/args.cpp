#include <string.h>
#include <stdarg.h>
#include "args.h"
#include "log.h"

int 
process_args(int argc, char *argv[], params_t *params)
{
        if (argc < 2) {
                log("Error: Expected more arguments.\n");
                return ARG_INV_USG;
        }

        for (int i = 1; i < argc; i++) {
                if ((strcmp(argv[i], "-i") == 0) ||
                    (strcmp(argv[i], "--input") == 0)) {
                        params->src_filename = argv[++i];
                } else if ((strcmp(argv[i], "-v") == 0) ||
                           (strcmp(argv[i], "--verbose") == 0)) {
                        params->verbose = true;
                } else {
                        fprintf(stderr, "Error: Wrong parameters.\n");
                        return ARG_INV_USG;
                }
        }

        return ARG_NO_ERR;
}

int 
verbose_msg(bool verbose, const char *format, ...)
{
        if (!verbose)
                return 0;

        va_list args;
        va_start(args, format);
        fprintf(stderr, "VERBOSE: ");
        int ret = vfprintf(stderr, format, args);
        va_end(args);

        return ret;
}

