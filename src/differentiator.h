#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include <sys/stat.h>
#include "tree.h"

// Error codes.
enum diff_err_t {
        D_ERR_NO_ERR = 0,
        D_ERR_ALLOC = 1,
        D_ERR_OPEN = 2,
        D_ERR_STATS = 3,
        D_ERR_SYNTAX = 4,
        D_ERR_MAP = 5,
        D_ERR_EMPTY = 6,
};

// Structure to store information about file.
struct file_t {
        FILE  *stream = nullptr;
        struct stat stats = {};
};

// Parses the contents from stream.
int
diff_parse(tree_t *tree);
// Takes a derivative of function built from tree.
int
diff_take(tree_t *eq, tree_t *diff, int *epos, int *dpos);

#endif // DIFF_H

