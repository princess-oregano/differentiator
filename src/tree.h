#ifndef TREE_H
#define TREE_H

///////////////////////////////////////////////////////////
// Enum of available operations with numbers.
enum op_t {
        OP_ADD = 1,
        OP_SUB = 2,
        OP_MUL = 3,
        OP_DIV = 4,
};

// Enum of available objects that can be differentiated.
enum val_type_t {
        VAL_POISON = 0,
        VAL_VAR = 1,
        VAL_NUM = 2,
        VAL_CONST = 3,
        VAL_OP  = 4, 
};

// Union containing one of avalable objects.
union value_t {
        char var;
        op_t op;
        const char *m_const;
        double num;
        char *trig;
};

// Data stored in tree node.
struct tree_data_t {
        val_type_t type = VAL_VAR;
        value_t val = {};
};
///////////////////////////////////////////////////////////

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

