#ifndef DIFF_H
#define DIFF_H

#include "tree.h"

const char *const MATH_PI = "π";
const char *const MATH_E = "e";

enum op_t {
        OP_ADD = 0,
        OP_SUB = 2,
        OP_MUL = 3,
        OP_DIV = 4,
};

enum diff_t {
        DIFF_VAR = 0,
        DIFF_NUM = 1,
        DIFF_CONST = 2,
        DIFF_OP  = 3, 
        DIFF_TRIG = 4,
};

union value_t {
        char var;
        op_t op;
        const char *m_const;
        double num;
        char *trig;
};

struct data_t {
        diff_t type = DIFF_VAR;
        value_t val = {};
};

// Parses the contents from stream.
int
diff_parse();
// Takes a derivative of function built from tree.
int
diff_take();
// Saves the resulting defivative to a file.
int 
diff_save();

#endif // DIFF_H

