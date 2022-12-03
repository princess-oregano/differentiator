#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "differentiator.h"
#include "file.h"
#include "parser.h"
#include "tree.h"
#include "tree_dump.h"
#include "tex_dump.h"
#include "log.h"
#include "types.h"

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
diff_take_add_sub(tree_t *eq, tree_t *diff, int *epos, int *dpos, bool plus)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;

        if (plus)
                diff->nodes[*dpos].data.val.op = OP_ADD;
        else
                diff->nodes[*dpos].data.val.op = OP_SUB;


        diff_copy(eq, diff, &en[*epos].left, &dn[*dpos].left);
        dn[dn[*dpos].left].data.copy = true;
        diff_copy(eq, diff, &en[*epos].right, &dn[*dpos].right);
        dn[dn[*dpos].right].data.copy = true;

        diff_take(eq, diff, &en[*epos].left, &dn[*dpos].left);
        diff_take(eq, diff, &en[*epos].right, &dn[*dpos].right);
}

static void
diff_take_mul(tree_t *eq, tree_t *diff, int *epos, int *dpos, bool div)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        if (!div)
                diff->nodes[*dpos].data.val.op = OP_ADD;
        else 
                diff->nodes[*dpos].data.val.op = OP_SUB;

        tmp = {.type = DIFF_OP, .val = {.op = OP_MUL}};
        node_insert(diff, &dn[*dpos].left, tmp);
        node_insert(diff, &dn[*dpos].right, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].left].right);
        diff_copy(eq, diff, &en[*epos].left, &dn[dn[*dpos].right].right);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].left);
        dn[dn[dn[*dpos].right].left].data.copy = true;
        diff_copy(eq, diff, &en[*epos].left, &dn[dn[*dpos].left].left);
        dn[dn[dn[*dpos].left].left].data.copy = true;

        diff_take(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].left);
        diff_take(eq, diff, &en[*epos].left, &dn[dn[*dpos].left].left);
}

static void
diff_take_div(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        diff->nodes[*dpos].data.val.op = OP_DIV;

        // Left subtree.
        int *tpos = &dn[*dpos].left;
        tmp = {.type = DIFF_OP, .val = {.op = OP_SUB}};
        node_insert(diff, tpos, tmp);

        // Right subtree.
        tmp = {.type = DIFF_OP, .val = {.op = OP_POW}};
        node_insert(diff, &dn[*dpos].right, tmp);

        tmp = {.type = DIFF_NUM, .val = {.num = 2}};
        node_insert(diff, &dn[dn[*dpos].right].right, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].left);

        diff_take_mul(eq, diff, epos, tpos, true);
}

static void
diff_pow(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};
        double tmp_num = 0;

        dn[*dpos].data.val.op = OP_MUL;

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

        // Derivative of function in power.
        diff_copy(eq, diff, &en[*epos].left, &dn[*dpos].left);
        dn[dn[*dpos].left].data.copy = true;

        diff_take(eq, diff, &en[*epos].left, &dn[*dpos].left);
}

static void
diff_exp(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        dn[*dpos].data.val.op = OP_MUL;

        int *tpos = &dn[*dpos].right;

        tmp = {.type = DIFF_OP, .val = {.op = OP_MUL}};
        node_insert(diff, tpos, tmp);

        diff_copy(eq, diff, epos, &dn[*tpos].right);

        tmp = {.type = DIFF_OP, .val = {.op = OP_LN}};
        node_insert(diff, &dn[*tpos].left, tmp);

        node_insert(diff, &dn[dn[*tpos].left].left, {DIFF_POISON, {}});
        
        diff_copy(eq, diff, &en[*epos].left, &dn[dn[*tpos].left].right);

        diff_copy(eq, diff, &en[*epos].right, &dn[*dpos].left);
        dn[dn[*dpos].left].data.copy = true;

        diff_take(eq, diff, &en[*epos].right, &dn[*dpos].left);
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
        diff_copy(eq, diff, &en[*epos].left, &dn[*tpos].right);

        tpos = &dn[dn[*dpos].right].left;
        diff_copy(eq, diff, &en[*epos].right, &dn[*tpos].left);
        dn[dn[*tpos].left].data.copy = true;
        tpos = &dn[dn[*dpos].right].right;
        diff_copy(eq, diff, &en[*epos].left, &dn[dn[*tpos].left].left);
        dn[dn[dn[*tpos].left].left].data.copy = true;

        tpos = &dn[dn[*dpos].right].left;
        diff_take(eq, diff, &en[*epos].right, &dn[*tpos].left);
        tpos = &dn[dn[*dpos].right].right;
        diff_take(eq, diff, &en[*epos].left, &dn[dn[*tpos].left].left);
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
      
        // Right subtree.
        tmp = {.type = DIFF_OP, .val = {.op = OP_COS}};
        node_insert(diff, &dn[*dpos].right, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].right);

        // No left child for trigonometric functions.
        node_insert(diff, &dn[dn[*dpos].right].left, {DIFF_POISON, {}});
        
        // Left subtree.
        diff_copy(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);
        dn[dn[*dpos].left].data.copy = true;

        diff_take(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);
}

static void
diff_take_cos(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        diff->nodes[*dpos].data.val.op = OP_MUL;
      
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

        // Left subtree.
        diff_copy(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);
        dn[dn[*dpos].left].data.copy = true;

        diff_take(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);
}

static void
diff_take_ln(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        tree_node_t *dn = diff->nodes;
        tree_node_t *en = eq->nodes;
        tree_data_t tmp {};

        diff->nodes[*dpos].data.val.op = OP_MUL;
      
        // Right subtree.
        tmp = {.type = DIFF_OP, .val = {.op = OP_DIV}};
        node_insert(diff, &dn[*dpos].right, tmp);

        tmp = {.type = DIFF_NUM, .val = {.num = 1}};
        node_insert(diff, &dn[dn[*dpos].right].left, tmp);

        diff_copy(eq, diff, &en[*epos].right, &dn[dn[*dpos].right].right);

        // Left subtree.
        diff_copy(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);
        dn[dn[*dpos].left].data.copy = true;

        diff_take(eq, diff, &eq->nodes[*epos].right, &dn[*dpos].left);
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
                        diff_take_add_sub(eq, diff, epos, dpos, true);
                        break;
                case OP_SUB:
                        diff_take_add_sub(eq, diff, epos, dpos, false);
                        break;
                case OP_MUL:
                        diff_take_mul(eq, diff, epos, dpos, false);

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

        log("epos = %d, dpos = %d\n", *epos, *dpos);
        if (*dpos == diff->root) {
                diff_copy(eq, diff, &eq->root, &diff->root);
                diff->nodes[diff->root].data.copy = true;
        }
        include_graph(tree_graph_dump(diff, VAR_INFO(diff)));

        tex_diff_dump(diff);
        node_remove(diff, dpos);
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

