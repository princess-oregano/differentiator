#ifndef ARGS_H
#define ARGS_H

enum arg_err_t {
        ARG_NO_ERR = 0,
        ARG_INV_USG = 1,
};

struct params_t {
        char *src_filename = nullptr;
        bool verbose       = false;
};

// Processes command line arguments.
int
process_args(int argc, char *argv[], params_t *params);
// Prints info about program processes in verbose mode.
int 
verbose_msg(bool verbose, const char *format, ...);

#endif // ARGS_H

