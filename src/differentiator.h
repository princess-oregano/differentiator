#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include <sys/stat.h>
#include "tree.h"

const char *const MATH_PI = "π";
const char *const MATH_E = "e";
const char *const MATH_ADD = "+";
const char *const MATH_SUB = "-";
const char *const MATH_MUL = "*";
const char *const MATH_DIV = "/";
const char *const MATH_SIN = "sin";
const char *const MATH_COS = "cos";
const char *const MATH_TAN = "tan";
const char *const MATH_VAR = "x";

// Error codes.
enum diff_err_t {
        D_ERR_NO_ERR = 0,
        D_ERR_ALLOC = 1,
        D_ERR_OPEN = 2,
        D_ERR_STATS = 3,
        D_ERR_SYNTAX = 4,
        D_ERR_MAP = 5,
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
diff_take();
// Saves the resulting defivative to a file.
int 
diff_save();

#endif // DIFF_H

