#ifndef TREE_DUMP_H
#define TREE_DUMP_H

#include "tree.h"
#include "types.h"

#define VAR_INFO(var) ((var_info_t) {__FILE__, __LINE__, __PRETTY_FUNCTION__, #var})

struct var_info_t {
        const char *file_name     = nullptr;
        int line                  = 0;
        const char *func_name     = nullptr;
        const char* init_var_name = nullptr;
};

// Makes graph dump of tree.
char *
tree_graph_dump(tree_t *tree, var_info_t var_info);

#endif // TREE_DUMP_H

