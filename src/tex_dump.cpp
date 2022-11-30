#include <stdio.h>
#include <assert.h>
#include "tex_dump.h"
#include "log.h"

static bool
check_leaf(tree_t *tree, int *pos)
{
        fprintf(stderr, "left = %d, right = %d\n", tree->nodes[*pos].left, tree->nodes[*pos].right);

        if (tree->nodes[*pos].left == -1 && tree->nodes[*pos].right == -1)
                return false;
        else
                return true;
}

static bool
check_add_sub(tree_t *tree, int *pos)
{
        if (tree->nodes[*pos].data.type == DIFF_OP &&
           (tree->nodes[*pos].data.val.op == OP_ADD ||
            tree->nodes[*pos].data.val.op == OP_SUB))
                return true;
        else
                return false;
}

static void
tex_const(tree_t *eq, int *pos, FILE *stream)
{
        switch (eq->nodes[*pos].data.val.m_const) {
                case CONST_E:
                        fprintf(stream, "e");
                        break;
                case CONST_PI:
                        fprintf(stream, "\\pi");
                        break;
                default:
                        log("Invalid const type encountered.\n");
                        assert(0 && "Invalid const type encountered.");
                        break;
        }
}

static void
tex_brace(tree_t *eq, int *pos, FILE *stream, bool (*check)(tree_t *, int *))
{
        bool brace = check(eq, pos);

        if (brace)
                fprintf(stream, "(");
        tex_subtree(eq, pos, stream);
        if (brace)
                fprintf(stream, ")");
}

static void
tex_op(tree_t *eq, int *pos, FILE *stream)
{
        tree_node_t *en = eq->nodes;

        switch (eq->nodes[*pos].data.val.op) {
                case OP_ADD:
                        tex_brace(eq, &en[*pos].left, stream, check_add_sub);
                        fprintf(stream, " + ");
                        tex_brace(eq, &en[*pos].right, stream, check_add_sub);
                        break;
                case OP_SUB:
                        tex_brace(eq, &en[*pos].left, stream, check_add_sub);
                        fprintf(stream, " - ");
                        tex_brace(eq, &en[*pos].right, stream, check_add_sub);
                        break;
                case OP_MUL:
                        tex_brace(eq, &en[*pos].left, stream, check_add_sub);
                        fprintf(stream, " \\cdot ");
                        tex_brace(eq, &en[*pos].right, stream, check_add_sub);
                        break;
                case OP_DIV:
                        fprintf(stream, " \\dfrac{");
                        tex_subtree(eq, &en[*pos].left, stream);
                        fprintf(stream, "}{");
                        tex_subtree(eq, &en[*pos].right, stream);
                        fprintf(stream, "} ");
                        break;
                case OP_POW:
                        fprintf(stream, "{");
                        tex_brace(eq, &en[*pos].left, stream, check_leaf);
                        fprintf(stream, "}^{");
                        tex_subtree(eq, &en[*pos].right, stream);
                        fprintf(stream, "}");
                        break;
                case OP_SIN:
                        fprintf(stream, " sin(");
                        tex_subtree(eq, &en[*pos].right, stream);
                        fprintf(stream, ")");
                        break;
                case OP_COS:
                        fprintf(stream, " cos(");
                        tex_subtree(eq, &en[*pos].right, stream);
                        fprintf(stream, ")");
                        break;
                case OP_LN:
                        fprintf(stream, " ln(");
                        tex_subtree(eq, &en[*pos].right, stream);
                        fprintf(stream, ")");
                        break;
                default:
                        log("Invalid operation type encountered.\n");
                        assert(0 && "Invalid operation type encountered.");
                        break;
        }
}

void
tex_subtree(tree_t *eq, int *pos, FILE *stream)
{
        tree_data_t data = eq->nodes[*pos].data;
        switch (data.type) {
                case DIFF_POISON:
                        log("Invalid data type encountered.\n");
                        assert(0 && "Met poison node while tex dumping.");
                        break;
                case DIFF_VAR:
                        fprintf(stream, "%c", data.val.var);
                        break;
                case DIFF_NUM:
                        fprintf(stream, "%lg", data.val.num);
                        break;
                case DIFF_CONST:
                        tex_const(eq, pos, stream);
                        break;
                case DIFF_OP:
                        tex_op(eq, pos, stream);
                        break;
                default:
                        break;
        }
}

void 
tex_tree_dump(tree_t *eq, const char *filename)
{
        file_t file {};
        get_file(filename, &file, "w");

        setvbuf(file.stream, nullptr, _IONBF, 0);

        tex_subtree(eq, &eq->root, file.stream);

        fclose(file.stream);
}

