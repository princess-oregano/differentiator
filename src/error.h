#ifndef ERROR_STACK_H
#define ERROR_STACK_H

#include <stdio.h>

struct error_t {
        bool ERR_ALLOC = 0;
        bool ERR_BAD_SIZE = 0;
        bool ERR_BAD_CAP = 0;
        bool ERR_INVALID_DATA = 0;
        bool ERR_STACK_CANARY = 0;
        bool ERR_DATA_CANARY = 0;
        bool ERR_STACK_HASH = 0;
};

union err_u {
        long long val;
        error_t type {};
};

// Prints all encountered errors stored in err variable.
void decypher_error(err_u err);

#endif // ERROR_STACK_H

