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
#include "args.h"

// Left/right subtrees.
#define DL(ARG) diff->nodes[ARG].left
#define DR(ARG) diff->nodes[ARG].right
#define EL(ARG) eq->nodes[ARG].left
#define ER(ARG) eq->nodes[ARG].right
// Data.
#define DD(ARG) diff->nodes[ARG].data
#define ED(ARG) eq->nodes[ARG].data
// Copy.
#define CPY(EQ_SUBTREE, DIFF_SUBTREE) diff_copy(eq, diff, &EQ_SUBTREE, &DIFF_SUBTREE)
// Take derivative.
#define TKE(EQ_SUBTREE, DIFF_SUBTREE) diff_take(eq, diff, &EQ_SUBTREE, &DIFF_SUBTREE)
// Number in tree.
#define NUM(N) {.type = DIFF_NUM, .val = {.num = N}}
// Operation in tree.
#define OP(OP_NAME) {.type = DIFF_OP, .val = {.op = OP_##OP_NAME}}

int
diff_parse(tree_t *tree, char *buffer)
{
        tok_arr_t tok_arr {};

        int err = 0;

        if ((err = lexer(buffer, &tok_arr)) != LEX_NO_ERR) {
                return err;
        }

        int count = 0;
        if ((err = parse(tree, tok_arr.ptr, &tree->root, &count)) != 
                                                                PAR_NO_ERR) {
                return err;
        }

        free(tok_arr.ptr);
        include_graph(tree_graph_dump(tree, VAR_INFO(tree)));

        return D_ERR_NO_ERR;
}

// Copies subtree to given destination.
static int
diff_copy(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        if (node_insert(diff, dpos, {DIFF_POISON, {}}) != ERR_NO_ERR)
                return D_ERR_INSERT;

        DD(*dpos) = ED(*epos);

        if (EL(*epos) != -1) 
                CPY(EL(*epos), DL(*dpos));

        if (ER(*epos) != -1)
                CPY(ER(*epos), DR(*dpos));

        return D_ERR_NO_ERR;
}

static bool
diff_find(tree_t *eq, int *epos, diff_obj_type_t type)
{
        bool ret_val = false;

        if (ED(*epos).type == type)
                return true;

        if (EL(*epos) != -1) 
                ret_val = diff_find(eq, &EL(*epos), type);

        if (ret_val == true)
                return ret_val;

        if (ER(*epos) != -1) 
                ret_val = diff_find(eq, &ER(*epos), type);

        return ret_val;
}

static void
diff_take_add_sub(tree_t *eq, tree_t *diff, int *epos, int *dpos, bool plus)
{
        if (plus)
                DD(*dpos).val.op = OP_ADD;
        else
                DD(*dpos).val.op = OP_SUB;

        CPY(EL(*epos), DL(*dpos));
        DD(DL(*dpos)).copy = true;
        CPY(ER(*epos), DR(*dpos));
        DD(DR(*dpos)).copy = true;

        TKE(EL(*epos), DL(*dpos));
        TKE(ER(*epos), DR(*dpos));
}

static void
diff_take_mul(tree_t *eq, tree_t *diff, int *epos, int *dpos, bool div)
{
        if (!div)
                DD(*dpos).val.op = OP_ADD;
        else 
                DD(*dpos).val.op = OP_SUB;

        node_insert(diff, &DL(*dpos), OP(MUL));
        node_insert(diff, &DR(*dpos), OP(MUL));

        CPY(ER(*epos), DR(DL(*dpos)));
        CPY(EL(*epos), DR(DR(*dpos)));

        CPY(ER(*epos), DL(DR(*dpos)));
        DD(DL(DR(*dpos))).copy = true;
        CPY(EL(*epos), DL(DL(*dpos)));
        DD(DL(DL(*dpos))).copy = true;

        TKE(ER(*epos), DL(DR(*dpos)));
        TKE(EL(*epos), DL(DL(*dpos)));
}

static void
diff_take_div(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        DD(*dpos).val.op = OP_DIV;

        // Left subtree.
        node_insert(diff, &DL(*dpos), OP(SUB));

        // Right subtree.
        node_insert(diff, &DR(*dpos), OP(POW));

        node_insert(diff, &DR(DR(*dpos)), NUM(2));

        CPY(ER(*epos), DL(DR(*dpos)));

        diff_take_mul(eq, diff, epos, &DL(*dpos), true);
}

static void
diff_pow(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        DD(*dpos).val.op = OP_MUL;

        // Derivative of power.
        node_insert(diff, &DR(*dpos), OP(MUL));

        CPY(ER(*epos), DL(DR(*dpos)));

        node_insert(diff, &DR(DR(*dpos)), OP(POW));

        CPY(EL(*epos), DL(DR(DR(*dpos))));

        node_insert(diff, &DR(DR(DR(*dpos))), OP(SUB));

        CPY(ER(*epos), DL(DR(DR(DR(*dpos)))));

        node_insert(diff, &DR(DR(DR(DR(*dpos)))), NUM(1));

        // Derivative of function in power.
        CPY(EL(*epos), DL(*dpos));
        DD(DL(*dpos)).copy = true;

        TKE(EL(*epos), DL(*dpos));
}

static void
diff_exp(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        DD(*dpos).val.op = OP_MUL;

        node_insert(diff, &DR(*dpos), OP(MUL));

        CPY(*epos, DR(DR(*dpos)));

        node_insert(diff, &DL(DR(*dpos)), OP(LN));

        node_insert(diff, &DL(DL(DR(*dpos))), {DIFF_POISON, {}});
        
        CPY(EL(*epos), DR(DL(DR(*dpos))));

        CPY(ER(*epos), DL(*dpos));
        DD(DL(*dpos)).copy = true;

        TKE(ER(*epos), DL(*dpos));
}

static void
diff_pow_exp(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        DD(*dpos).val.op = OP_MUL;

        CPY(*epos, DL(*dpos));

        node_insert(diff, &DR(*dpos), OP(ADD));

        // Left subtree of '+' node.
        node_insert(diff, &DL(DR(*dpos)), OP(MUL));

        node_insert(diff, &DR(DL(DR(*dpos))), OP(LN));

        node_insert(diff, &DL(DR(DL(DR(*dpos)))), {DIFF_POISON, {}});
        CPY(EL(*epos), DR(DR(DL(DR(*dpos)))));

        // Right subtree of '+' node.
        node_insert(diff, &DR(DR(*dpos)), OP(DIV));

        node_insert(diff, &DL(DR(DR(*dpos))), OP(MUL));

        CPY(EL(*epos), DR(DL(DR(DR(*dpos)))));
        CPY(EL(*epos), DR(DR(DR(*dpos))));

        CPY(ER(*epos), DL(DL(DR(*dpos))));
        DD(DL(DL(DR(*dpos)))).copy = true;
        CPY(EL(*epos), DL(DL(DR(DR((*dpos))))));
        DD(DL(DL(DR(DR((*dpos)))))).copy = true;

        TKE(ER(*epos), DL(DL(DR(*dpos))));
        TKE(EL(*epos), DL(DL(DR(DR((*dpos))))));
}

static void
diff_take_pow(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        if (diff_find(eq, &ER(*epos), DIFF_VAR) == false) {
                diff_pow(eq, diff, epos, dpos);
        } else {
                if (diff_find(eq, &EL(*epos), DIFF_VAR) == false) {
                        diff_exp(eq, diff, epos, dpos);
                } else {
                        diff_pow_exp(eq, diff, epos, dpos);
                }
        }
}

static void
diff_take_sin(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        DD(*dpos).val.op = OP_MUL;
      
        // Right subtree.
        node_insert(diff, &DR(*dpos), OP(COS));

        CPY(ER(*epos), DR(DR(*dpos)));

        // No left child for trigonometric functions.
        node_insert(diff, &DL(DR(*dpos)), {DIFF_POISON, {}});
        
        // Left subtree.
        CPY(ER(*epos), DL(*dpos));
        DD(DL(*dpos)).copy = true;

        TKE(ER(*epos), DL(*dpos));
}

static void
diff_take_cos(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        DD(*dpos).val.op = OP_MUL;
      
        // Right subtree.
        node_insert(diff, &DR(*dpos), OP(MUL));

        node_insert(diff, &DL(DR(*dpos)), NUM(-1));

        node_insert(diff, &DR(DR(*dpos)), OP(SIN));

        CPY(ER(*epos), DR(DR(DR(*dpos))));

        // No left child for trigonometric functions.
        node_insert(diff, &DL(DR(DR(*dpos))), {DIFF_POISON, {}});

        // Left subtree.
        CPY(ER(*epos), DL(*dpos));
        DD(DL(*dpos)).copy = true;

        TKE(ER(*epos), DL(*dpos));
}

static void
diff_take_ln(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        DD(*dpos).val.op = OP_MUL;
      
        // Right subtree.
        node_insert(diff, &DR(*dpos), OP(DIV));

        node_insert(diff, &DL(DR(*dpos)), NUM(1));

        CPY(ER(*epos), DR(DR(*dpos)));

        // Left subtree.
        CPY(ER(*epos), DL(*dpos));
        DD(DL(*dpos)).copy = true;

        TKE(ER(*epos), DL(*dpos));
}

static int
diff_take_op(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        assert(eq);
        assert(diff);
        assert(epos);
        assert(dpos);

        switch(ED(*epos).val.op) {
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
        switch (ED(*epos).type) {
                // The program should never meet poison node.
                case DIFF_POISON:
                        log("Error: empty node encoutered while "
                            "taking the derivative.\n");
                        return D_ERR_EMPTY;
                // Variable; always 1.
                case DIFF_VAR:
                        DD(*dpos).type = DIFF_NUM;
                        DD(*dpos).val.num = 1;
                        break;
                // Constant number; always 0.
                case DIFF_NUM:
                        DD(*dpos).type = DIFF_NUM;
                        DD(*dpos).val.num = 0;
                        break;
                // Math constant; always 0.
                case DIFF_CONST:
                        DD(*dpos).type = DIFF_NUM;
                        DD(*dpos).val.num = 0;
                        break;
                // Operation; needs to be handled specifically.
                case DIFF_OP:
                        DD(*dpos).type = DIFF_OP;
                        diff_take_op(eq, diff, epos, dpos);
                        break;
                // Type of node should never be outside of enum.
                default:
                        assert(0 && "Invalid type of node.");
                        break;
        }

        return D_ERR_NO_ERR;
}

#undef DL
#undef DR
#undef EL
#undef ER
#undef DD
#undef ED
#undef CPY
#undef TKE
#undef NUM
#undef OP

