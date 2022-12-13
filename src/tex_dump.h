#ifndef TEX_DUMP_H
#define TEX_DUMP_H

#include "tree.h"
#include "file.h"

const int REPLACE_NUM = 20;

struct substitution_t {
        char letter = '\0';
        int subtree = 0;
};

struct replace_t {
        substitution_t sub[REPLACE_NUM];
        int size = 0;
};

void 
tex_diff_dump(tree_t *eq);
void
tex_sim_dump(tree_t *eq);
void
tex_eq_dump(tree_t *eq);
void
tex_subtree(tree_t *eq, int *pos, bool repl, FILE *stream);
void
tex_end();
void
tex_begin(const char *filename);

#endif // TEX_DUMP_H

