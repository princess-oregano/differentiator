#ifndef TEX_DUMP_H
#define TEX_DUMP_H

#include "tree.h"
#include "file.h"

void 
tex_diff_dump(tree_t *eq);
void
tex_sim_dump(tree_t *eq);
void
tex_eq_dump(tree_t *eq);
void
tex_subtree(tree_t *eq, int *pos, FILE *stream);
void
tex_end();
void
tex_begin(const char *filename);

#endif // TEX_DUMP_H

