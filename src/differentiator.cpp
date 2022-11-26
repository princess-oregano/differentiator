#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include "differentiator.h"
#include "parser.h"
#include "tree.h"
#include "tree_dump.h"
#include "log.h"

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

        parse(tree, tok_arr.ptr, &tree->root);

        free(tok_arr.ptr);
        include_graph(tree_graph_dump(tree, VAR_INFO(tree)));

        return D_ERR_NO_ERR;
}

////////////////////////////////////////////////////////////////////////////////

static void
diff_copy(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        node_insert(diff, dpos, {DIFF_POISON, {}});

        diff->nodes[*dpos].data = eq->nodes[*epos].data;
       
        if (eq->nodes[*epos].left != -1) {
                diff_copy(eq, diff, &eq->nodes[*epos].left, 
                                &diff->nodes[*dpos].left);
        }
        if (eq->nodes[*epos].right != -1) {
                diff_copy(eq, diff, &eq->nodes[*epos].right, 
                                &diff->nodes[*dpos].right);
        }
}

static void
diff_take_mul(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        diff->nodes[*dpos].data.val.op = OP_ADD;

        node_insert(diff, &diff->nodes[*dpos].left, {DIFF_POISON, {}}); 
        diff->nodes[diff->nodes[*dpos].left].data.type = DIFF_OP;
        diff->nodes[diff->nodes[*dpos].left].data.val.op = OP_MUL;

        node_insert(diff, &diff->nodes[*dpos].right, {DIFF_POISON, {}}); 
        diff->nodes[diff->nodes[*dpos].right].data.type = DIFF_OP;
        diff->nodes[diff->nodes[*dpos].right].data.val.op = OP_MUL;

        diff_take(eq, diff, &eq->nodes[*epos].left, &diff->nodes[diff->nodes[*dpos].left].left);
        diff_copy(eq, diff, &eq->nodes[*epos].right, &diff->nodes[diff->nodes[*dpos].left].right);
        
        diff_take(eq, diff, &eq->nodes[*epos].right, &diff->nodes[diff->nodes[*dpos].right].left);
        diff_copy(eq, diff, &eq->nodes[*epos].left, &diff->nodes[diff->nodes[*dpos].right].right);
}

static void
diff_take_div(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        diff->nodes[*dpos].data.val.op = OP_DIV;

        // Left subtree.
        node_insert(diff, &diff->nodes[*dpos].left, {DIFF_POISON, {}}); 
        diff_take_mul(eq, diff, epos, &diff->nodes[*dpos].left); 
        diff->nodes[diff->nodes[*dpos].left].data.type = DIFF_OP;
        diff->nodes[diff->nodes[*dpos].left].data.val.op = OP_SUB;

        // Right subtree.
        node_insert(diff, &diff->nodes[*dpos].right, {DIFF_POISON, {}}); 
        diff->nodes[diff->nodes[*dpos].right].data.type = DIFF_OP;
        diff->nodes[diff->nodes[*dpos].right].data.val.op = OP_POW;

        node_insert(diff, &diff->nodes[diff->nodes[*dpos].right].right, {DIFF_POISON, {}});
        diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].data.type = DIFF_NUM;
        diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].data.val.num = 2;

        diff_copy(eq, diff, &eq->nodes[*epos].right, &diff->nodes[diff->nodes[*dpos].right].left);
}

static void
diff_take_pow(tree_t *eq, tree_t *diff, int *epos, int *dpos)
{
        diff->nodes[*dpos].data.val.op = OP_MUL;
  
        node_insert(diff, &diff->nodes[*dpos].left, {DIFF_POISON, {}}); 
        diff->nodes[diff->nodes[*dpos].left].data.type = DIFF_NUM;
        diff->nodes[diff->nodes[*dpos].left].data.val.num =
                eq->nodes[eq->nodes[*epos].right].data.val.num;
  
        // Power (x)^(a-1).
        node_insert(diff, &diff->nodes[*dpos].right, {DIFF_POISON, {}}); 
        diff->nodes[diff->nodes[*dpos].right].data.type = DIFF_OP;
        diff->nodes[diff->nodes[*dpos].right].data.val.op = OP_POW;
  
        // (x)
        diff_copy(eq, diff, &eq->nodes[*epos].left, 
                        &diff->nodes[diff->nodes[*dpos].right].left);
  
        // (a-1).
        node_insert(diff, &diff->nodes[diff->nodes[*dpos].right].right, {DIFF_POISON, {}}); 
        diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].data.type = DIFF_OP;
        diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].data.val.op = OP_SUB;
  
        node_insert(diff, &diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].left, {DIFF_POISON, {}}); 
        diff->nodes[diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].left].data.type = DIFF_NUM;
        diff->nodes[diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].left].data.val.num =
                eq->nodes[eq->nodes[*epos].right].data.val.num;
        
        node_insert(diff, &diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].right, {DIFF_POISON, {}}); 
        diff->nodes[diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].right].data.type = DIFF_NUM;
        diff->nodes[diff->nodes[diff->nodes[diff->nodes[*dpos].right].right].right].data.val.num = 1;
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
                case OP_COS:
                        assert(0 && "Operation is not yet handled.\n");
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

