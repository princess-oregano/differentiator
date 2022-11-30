#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "simplifier.h"
#include "tree_dump.h"
#include "log.h"

static bool 
are_equal(double value1, double value2)
{
        return (fabs(value1 - value2) < THRESHOLD);
}

void
sim_const(tree_t *eq, int *pos)
{
        tree_data_t tmp = {};
        double num = 0;

        tree_node_t *en = eq->nodes;

        if (en[*pos].left == -1)
                return;
                
        sim_const(eq, &en[*pos].left);

        if (en[*pos].right == -1)
                return;
        
        sim_const(eq, &en[*pos].right);

        if (en[en[*pos].left].data.type == DIFF_NUM &&
            en[en[*pos].right].data.type == DIFF_NUM) {
                double num1 = en[en[*pos].left].data.val.num;
                double num2 = en[en[*pos].right].data.val.num;

                switch (en[*pos].data.val.op) {
                        case OP_ADD:
                                num = num1 + num2;
                                break;
                        case OP_SUB:
                                num = num1 - num2;
                                break;
                        case OP_MUL:
                                num = num1 * num2;
                                break;
                        case OP_DIV:
                                num = num1 / num2;
                                break;
                        case OP_POW:
                                num = pow(num1, num2);
                                break;
                        case OP_SIN:
                        case OP_COS:
                        case OP_LN:
                                return;
                        default:
                                log("Invalid operation type '%d'\n", en[*pos].data.val.op);
                                assert(0 && "Invalid operation type");
                                return;
                }

                tmp = {.type = DIFF_NUM, .val = {.num = num}};
                node_remove(eq, pos);
                node_insert(eq, pos, tmp);
        }
}

static void
sim_add_sub(tree_t *eq, int *pos)
{
        tree_node_t *en = eq->nodes;

        tree_data_t elem1 = en[en[*pos].left].data;
        tree_data_t elem2 = en[en[*pos].right].data;

        if (elem1.type == DIFF_NUM && are_equal(elem1.val.num, 0)) {
                node_remove(eq, &en[*pos].left);
                node_bound(pos, en[*pos].right);
        }

        if (elem2.type == DIFF_NUM && are_equal(elem2.val.num, 0)) {
                node_remove(eq, &en[*pos].right);
                node_bound(pos, en[*pos].left);
        }
}

static void
sim_mul(tree_t *eq, int *pos)
{
        tree_node_t *en = eq->nodes;
        tree_data_t tmp = {};

        tree_data_t elem1 = en[en[*pos].left].data;
        tree_data_t elem2 = en[en[*pos].right].data;

        if ((elem1.type == DIFF_NUM && are_equal(elem1.val.num, 0)) ||
            (elem2.type == DIFF_NUM && are_equal(elem2.val.num, 0))) {
                node_remove(eq, pos);
                tmp = {.type = DIFF_NUM, .val = {.num = 0}};
                node_insert(eq, pos, tmp);
        }

        if (elem1.type == DIFF_NUM && are_equal(elem1.val.num, 1)) {
                node_remove(eq, &en[*pos].left);
                node_bound(pos, en[*pos].right);
        }

        if (elem2.type == DIFF_NUM && are_equal(elem2.val.num, 1)) {
                node_remove(eq, &en[*pos].right);
                node_bound(pos, en[*pos].left);
        }
}

static void
sim_div(tree_t *eq, int *pos)
{
        tree_node_t *en = eq->nodes;
        tree_data_t tmp = {};

        tree_data_t elem1 = en[en[*pos].left].data;
        tree_data_t elem2 = en[en[*pos].right].data;

        if (elem2.type == DIFF_NUM && are_equal(elem2.val.num, 1)) {
                node_remove(eq, &en[*pos].right);
                node_bound(pos, en[*pos].left);
        }

        if ((elem1.type == DIFF_NUM && are_equal(elem1.val.num, 0))) {
                node_remove(eq, pos);
                tmp = {.type = DIFF_NUM, .val = {.num = 0}};
                node_insert(eq, pos, tmp);
        }
}

static void
sim_pow(tree_t *eq, int *pos)
{
        tree_node_t *en = eq->nodes;
        tree_data_t tmp = {};

        tree_data_t elem1 = en[en[*pos].left].data;
        tree_data_t elem2 = en[en[*pos].right].data;

        if (elem1.type == DIFF_NUM && are_equal(elem1.val.num, 0)) {
                node_remove(eq, pos);
                tmp = {.type = DIFF_NUM, .val = {.num = 0}};
                node_insert(eq, pos, tmp);
        }

        if ((elem1.type == DIFF_NUM && are_equal(elem1.val.num, 1))) {
                node_remove(eq, pos);
                tmp = {.type = DIFF_NUM, .val = {.num = 1}};
                node_insert(eq, pos, tmp);
        }
        
        if (elem2.type == DIFF_NUM && are_equal(elem2.val.num, 0)) {
                node_remove(eq, pos);
                tmp = {.type = DIFF_NUM, .val = {.num = 1}};
                node_insert(eq, pos, tmp);
        }

        if (elem2.type == DIFF_NUM && are_equal(elem2.val.num, 1)) {
                node_remove(eq, &en[*pos].right);
                node_bound(pos, en[*pos].left);
        }
}

void
sim_neutral(tree_t *eq, int *pos)
{
        tree_node_t *en = eq->nodes;

        if (en[*pos].left == -1)
                return;
                
        sim_neutral(eq, &en[*pos].left);

        if (en[*pos].right == -1)
                return;
        
        sim_neutral(eq, &en[*pos].right);

        switch (en[*pos].data.val.op) {
                case OP_ADD:
                case OP_SUB:
                        sim_add_sub(eq, pos);
                        break;
                case OP_MUL:
                        sim_mul(eq, pos);
                        break;
                case OP_DIV:
                        sim_div(eq, pos);
                        break;
                case OP_POW:
                        sim_pow(eq, pos);
                        break;
                case OP_SIN:
                case OP_COS:
                case OP_LN:
                        break;
                default:
                        log("Invalid operation type '%d'\n", en[*pos].data.val.op);
                        assert(0 && "Invalid operation type");
                        break;
        }
}

