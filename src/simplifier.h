#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "tree.h"

const double THRESHOLD = 0.000001;

enum sim_err_t {
        SIM_NO_ERR = 0,
};

// Simplifies equation using two methods.
void
sim_eq(tree_t *eq, int *pos);
// Performs convolution of constants.
void
sim_const(tree_t *eq, int *pos, bool *changed);
// Removes neutral elements(like multiplying by 0, 1, etc.).
void
sim_neutral(tree_t *eq, int *pos, bool *changed);

#endif // SIMPLIFIER_H

