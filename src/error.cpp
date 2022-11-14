#include <stdio.h>
#include "error.h"

void
decypher_error(err_u err)
{
        if (err.val == 0)
                return;

        if (err.type.ERR_ALLOC) {
                fprintf(stderr, "Couldn't allocate memory.\n");
        }

        if (err.type.ERR_BAD_SIZE) {
                fprintf(stderr, "Invalid size.\n");
        }

        if (err.type.ERR_ALLOC) {
                fprintf(stderr, "Invalid capacity.\n");
        }
}

