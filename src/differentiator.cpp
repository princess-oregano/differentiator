#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include "differentiator.h"
#include "parser.h"
#include "tree.h"
#include "tree_dump.h"
#include "log.h"
#include "types.h"

////////////////////////////////////////////////////////////////////////////////

// Makes a structure with info about file.
static int
get_file(const char *filename, file_t *file, const char *mode)
{
        if ((file->stream = fopen(filename, mode)) == nullptr) {
                log("Error: Couldn't open %s.\n", filename);

                return D_ERR_OPEN;
        }

        if (stat(filename, &file->stats) != 0) {
                log("Error: Coudn't get stats of %s.\n", filename);
                return D_ERR_STATS;
        }

        return D_ERR_STATS;
}

// Reads file and puts its contents to a buffer.
static int
read_file(char **buffer, file_t *file)
{
        assert(file);
        assert(buffer);

        *buffer = (char *) mmap(NULL, (size_t) file->stats.st_size, PROT_READ,
                               MAP_PRIVATE, fileno(file->stream), 0);

        if (*buffer == MAP_FAILED) {
                log("Error: Couldn't allocate memory.\n");
                log("Exiting %s.\n", __PRETTY_FUNCTION__);
                return D_ERR_MAP;
        }

        return D_ERR_NO_ERR;
}

int
diff_parse(tree_t *tree)
{
        file_t file {};
        char *buffer = nullptr;
        tok_arr_t tok_arr {};

        get_file("test", &file, "r");
        read_file(&buffer, &file);

        lexer(buffer, &tok_arr);

        int count = 0;
        parse(tree, tok_arr.ptr, &tree->root, &count);

        free(tok_arr.ptr);
        include_graph(tree_graph_dump(tree, VAR_INFO(tree)));

        return D_ERR_NO_ERR;
}

////////////////////////////////////////////////////////////////////////////////

// Copies subtree to given destination.
static void
diff_copy(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;

        node_insert(diff, dpos, {DIFF_POISON, {}});

        dn[*dpos].data = en[*epos].data;

        if (en[*epos].left != -1) 
                diff_copy(eq, diff, &en[*epos].left, &dn[*dpos].left);

        if (en[*epos].right != -1)
                diff_copy(eq, diff, &en[*epos].right, &dn[*dpos].right);
}

static bool
diff_find(tree_t *eq, int *epos, diff_obj_type_t type)
{
        bool ret_val = false;

        if (eq->nodes[*epos].data.type == type)
                return true;

        if (eq->nodes[*epos].left != -1) 
                ret_val = diff_find(eq, &eq->nodes[*epos].left, type);

        if (ret_val == true)
                return ret_val;

        if (eq->nodes[*epos].right != -1) 
                ret_val = diff_find(eq, &eq->nodes[*epos].right, type);

        return ret_val;
}

static void
diff_take_mul(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        diff->nodes[*dpos].data.val.op = OP_ADD;

        tmp = {.type = DIFF_OP, .val = {.op = OP_MUL}};
        node_insert(diff, &dn[*dpos].left, tmp);
        node_insert(diff, &dn[*dpos].right, tmp);

        diff_take(eq, diff, &en[*epos].left, &dn[dn[*dpos].left].left);
        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].left].right);

        diff_take(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].left);
        diff_copy(eq, diff, &en[*epos].left, &dn[dn[*dpos].right].right);
}

static void
diff_take_div(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        diff->nodes[*dpos].data.val.op = OP_DIV;

        // Left subtree.
        tmp = {.type = DIFF_OP, .val = {.op = OP_SUB}};
        node_insert(diff, &dn[*dpos].left, tmp);
        diff_take_mul(eq, diff, epos, &dn[*dpos].left);

        // Right subtree.
        tmp = {.type = DIFF_OP, .val = {.op = OP_POW}};
        node_insert(diff, &dn[*dpos].right, tmp);

        tmp = {.type = DIFF_NUM, .val = {.num = 2}};
        node_insert(diff, &dn[dn[*dpos].right].right, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].left);
}

static void
diff_pow(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};
        double tmp_num = 0;

        dn[*dpos].data.val.op = OP_MUL;

        // Derivative of function in power.
        diff_take(eq, diff, &en[*epos].left, &dn[*dpos].left);

        // Derivative of power.
        int *tpos = &dn[*dpos].right;

        tmp = {.type = DIFF_OP, .val = {.op = OP_MUL}};
        node_insert(diff, tpos, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[*tpos].left);

        tmp = {.type = DIFF_OP, .val = {.op = OP_POW}};
        node_insert(diff, &dn[*tpos].right, tmp);

        diff_copy(eq, diff, &en[*epos].left,
                        &dn[dn[*tpos].right].left);

        tmp = {.type = DIFF_OP, .val = {.op = OP_SUB}};
        node_insert(diff, &dn[dn[*tpos].right].right, tmp);

        tmp = {.type = DIFF_NUM, .val = {.num = tmp_num}};
        node_insert(diff, &dn[dn[dn[*tpos].right].right].left, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[dn[*tpos].right].right].left);

        tmp = {.type = DIFF_NUM, .val = {.num = 1}};
        node_insert(diff, &dn[dn[dn[*tpos].right].right].right, tmp);
}

static void
diff_exp(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        dn[*dpos].data.val.op = OP_MUL;

        diff_take(eq, diff, &en[*epos].right, &dn[*dpos].left);

        int *tpos = &dn[*dpos].right;

        tmp = {.type = DIFF_OP, .val = {.op = OP_MUL}};
        node_insert(diff, tpos, tmp);

        diff_copy(eq, diff, epos, &dn[*tpos].right);

        tmp = {.type = DIFF_OP, .val = {.op = OP_LN}};
        node_insert(diff, &dn[*tpos].left, tmp);

        node_insert(diff, &dn[dn[*tpos].left].left, {DIFF_POISON, {}});
        
        diff_copy(eq, diff, &en[*epos].left, &dn[dn[*tpos].left].right);
}

static void
diff_pow_exp(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};
        int *tpos = nullptr;

        dn[*dpos].data.val.op = OP_MUL;

        diff_copy(eq, diff, epos, &dn[*dpos].left);

        tmp = {.type = DIFF_OP, .val = {.op = OP_ADD}};
        node_insert(diff, &dn[*dpos].right, tmp);

        // Left subtree of '+' node.
        tpos = &dn[dn[*dpos].right].left;

        tmp = {.type = DIFF_OP, .val = {.op = OP_MUL}};
        node_insert(diff, tpos, tmp);

        diff_take(eq, diff, &en[*epos].right, &dn[*tpos].left);
        
        tmp = {.type = DIFF_OP, .val = {.op = OP_LN}};
        node_insert(diff, &dn[*tpos].right, tmp);

        node_insert(diff, &dn[dn[*tpos].right].left, {DIFF_POISON, {}});
        diff_copy(eq, diff, &en[*epos].left, &dn[dn[*tpos].right].right);

        // Right subtree of '+' node.
        tpos = &dn[dn[*dpos].right].right;

        tmp = {.type = DIFF_OP, .val = {.op = OP_DIV}};
        node_insert(diff, tpos, tmp);

        tmp = {.type = DIFF_OP, .val = {.op = OP_MUL}};
        node_insert(diff, &dn[*tpos].left, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*tpos].left].right);
        diff_take(eq, diff, &en[*epos].left, &dn[dn[*tpos].left].left);

        diff_copy(eq, diff, &en[*epos].left, &dn[*tpos].right);
}

static void
diff_take_pow(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *en = eq->nodes;

        if (diff_find(eq, &en[*epos].right, DIFF_VAR) == false) {
                diff_pow(eq, diff, epos, dpos);
        } else {
                if (diff_find(eq, &en[*epos].left, DIFF_VAR) == false) {
                        diff_exp(eq, diff, epos, dpos);
                } else {
                        diff_pow_exp(eq, diff, epos, dpos);
                }
        }
}

static void
diff_take_sin(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        diff->nodes[*dpos].data.val.op = OP_MUL;
      
        // Left subtree.
        diff_take(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);

        // Right subtree.
        tmp = {.type = DIFF_OP, .val = {.op = OP_COS}};
        node_insert(diff, &dn[*dpos].right, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].right);

        // No left child for trigonometric functions.
        node_insert(diff, &dn[dn[*dpos].right].left, {DIFF_POISON, {}});
}

static void
diff_take_cos(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        diff->nodes[*dpos].data.val.op = OP_MUL;
      
        // Left subtree.
        diff_take(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);

        // Right subtree.
        tmp = {.type = DIFF_OP, .val = {.op = OP_MUL}};
        node_insert(diff, &dn[*dpos].right, tmp);

        int *tpos = &dn[*dpos].right;

        tmp = {.type = DIFF_NUM, .val = {.num = -1}};
        node_insert(diff, &dn[*tpos].left, tmp);

        tmp = {.type = DIFF_OP, .val = {.op = OP_SIN}};
        node_insert(diff, &dn[*tpos].right, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*tpos].right].right);

        // No left child for trigonometric functions.
        node_insert(diff, &dn[dn[*tpos].right].left, {DIFF_POISON, {}});
}

static void
diff_take_ln(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        diff->nodes[*dpos].data.val.op = OP_MUL;
      
        // Left subtree.
        diff_take(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);

        // Right subtree.
        tmp = {.type = DIFF_OP, .val = {.op = OP_DIV}};
        node_insert(diff, &dn[*dpos].right, tmp);

        tmp = {.type = DIFF_NUM, .val = {.num = 1}};
        node_insert(diff, &dn[dn[*dpos].right].left, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].right);
}

static int
diff_take_op(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        assert(eq);
        assert(diff);
        assert(epos);
        assert(dpos);

        switch(eq->nodes[*epos].data.val.op) {
                case OP_ADD:
                        diff->nodes[*dpos].data.val.op = OP_ADD;

                        diff_take(eq, diff, &eq->nodes[*epos].left,
                                        &diff->nodes[*dpos].left);
                        diff_take(eq, diff, &eq->nodes[*epos].right,
                                        &diff->nodes[*dpos].right);

                        break;
                case OP_SUB:
                        diff->nodes[*dpos].data.val.op = OP_SUB;

                        diff_take(eq, diff, &eq->nodes[*epos].left,
                                        &diff->nodes[*dpos].left);
                        diff_take(eq, diff, &eq->nodes[*epos].right,
                                        &diff->nodes[*dpos].right);

                        break;
                case OP_MUL:
                        diff_take_mul(eq, diff, epos, dpos);

                        break;
                case OP_DIV:
                        diff_take_div(eq, diff, epos, dpos);

                        break;
                case OP_POW:
                        diff_take_pow(eq, diff, epos, dpos);

                        break;
                case OP_SIN:
                        diff_take_sin(eq, diff, epos, dpos);

                        break;
                case OP_COS:
                        diff_take_cos(eq, diff, epos, dpos);

                        break;
                case OP_LN:
                        diff_take_ln(eq, diff, epos, dpos);

                        break;
                default:
                        assert(0 && "Invalid operation type.");
                        break;
        }

        return D_ERR_NO_ERR;
}

int
diff_take(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        assert(eq);
        assert(diff);
        assert(epos);
        assert(dpos);

        // Insert node to derivative tree.
        node_insert(diff, dpos, {DIFF_POISON, {}});

        // Process node from equation tree.
        switch (eq->nodes[*epos].data.type) {
                // The program should never meet poison node.
                case DIFF_POISON:
                        log("Error: empty node encoutered while "
                            "taking the derivative.\n");
                        return D_ERR_EMPTY;
                // Variable; always 1.
                case DIFF_VAR:
                        diff->nodes[*dpos].data.type = DIFF_NUM;
                        diff->nodes[*dpos].data.val.num = 1;
                        break;
                // Constant number; always 0.
                case DIFF_NUM:
                        diff->nodes[*dpos].data.type = DIFF_NUM;
                        diff->nodes[*dpos].data.val.num = 0;
                        break;
                // Math constant; always 0.
                case DIFF_CONST:
                        diff->nodes[*dpos].data.type = DIFF_NUM;
                        diff->nodes[*dpos].data.val.num = 0;
                        break;
                // Operation; needs to be handled specifically.
                case DIFF_OP:
                        diff->nodes[*dpos].data.type = DIFF_OP;
                        diff_take_op(eq, diff, epos, dpos);
                        break;
                // Type of node should never be outside of enum.
                default:
                        assert(0 && "Invalid type of node.");
                        break;
        }

        return D_ERR_NO_ERR;
}

