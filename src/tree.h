#ifndef TREE_H
#define TREE_H

typedef char* tree_data_t;

enum tree_error_t {
        ERR_NO_ERR = 0,
        ERR_ALLOC = 1,
        ERR_BAD_POS = 2,
        ERR_BAD_CAP = 3,
};

struct tree_node_t {
        tree_data_t data {};
        int left = -1;
        int right = -1;
        int next_free = -1;
};

struct tree_t {
        tree_node_t *nodes = nullptr;
        int root = 0;
        int cap = 0;
        int free = 0;
};

// Constructs tree with 'cap' free unbounded nodes.
int
tree_ctor(tree_t *tree, int cap);
// Insert a node with given data to given position.
int
node_insert(tree_t *tree, int *parent, tree_data_t data);
// Bounds two nodes.
void
node_bound(int *parent, int node);
// Finds node in a tree.
void
node_find(tree_node_t *node);
// Removes node and all children of it.
int
node_remove(tree_t *tree, int *pos);
// Destructs tree.
int
tree_dtor(tree_t *tree);

#endif // TREE_H

