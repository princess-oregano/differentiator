#ifndef TEX_DUMP_H
#define TEX_DUMP_H

#include "tree.h"
#include "file.h"

void 
tex_tree_dump(tree_t *eq, const char *filename);
void
tex_subtree(tree_t *eq, int *pos, FILE *stream);

#endif // TEX_DUMP_H

