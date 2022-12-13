#include <stdio.h>
#include "log.h"
#include "parser.h"
#include "tree_dump.h"

static int
parse_subtree(tree_t *tree, token_t *tokens, int *pos, int *count)
{
        int err = 0;

        // Check if brace after opening brace is not closing.
        if (tokens[*count].type == DIFF_BRACE && 
            tokens[*count].val.closed == true) {
                log("Cannot initialize empty object.\n");
                return PAR_BRACE;
        }

        // Some operations have only one arg, so their
        // left child is initialized with void.
        if (tokens[*count].type == DIFF_OP &&
           (tokens[*count].val.op == OP_SIN ||
            tokens[*count].val.op == OP_COS ||
            tokens[*count].val.op == OP_LN)) {
                if (node_insert(tree, &tree->nodes[*pos].left, 
                                        {DIFF_POISON, 0}) != ERR_NO_ERR)
                        return PAR_INSERT;
        } else {
                if ((err = parse(tree, tokens, &tree->nodes[*pos].left, 
                                                count)) != PAR_NO_ERR)
                        return err;
                // Move from closing brace token.
                // It IS here, oVerwiZe, error would occur.
                //
                // Though this move forward is not needed in operations with 
                // one arg: they don't have closing brace for first arg as
                // they don't have one.
                (*count)++;
        }

        // Init the parent node.
        tree->nodes[*pos].data.type = tokens[*count].type;
        tree->nodes[*pos].data.val = tokens[*count].val;

        // Proceed to next token.
        (*count)++;

        // Init the right child.
        if ((err = parse(tree, tokens, &tree->nodes[*pos].right, 
                                          count)) != PAR_NO_ERR)
                        return err;
        // Move from closing brace token.
        // It IS here, overwise, error would occur.
        (*count)++;

        return PAR_NO_ERR;
}

static int
parse_node(tree_t *tree, token_t *tokens, int *pos, int *count)
{
        // Check if the token is the only one in braces.
        if (tokens[*count + 1].type != DIFF_BRACE &&
            tokens[*count + 1].val.closed != true) {
                log("Expected closing brace.\n");
                return PAR_BRACE;
        }
 
        if (tokens[*count].type == DIFF_OP) {
                log("Cannot perform operations with operation.\n");
                return PAR_OP;
        }
 
        // Initialize token.
        tree->nodes[*pos].data.type = tokens[*count].type;
        tree->nodes[*pos].data.val = tokens[*count].val;
 
        // Proceed to the next token.
        (*count)++;
 
        return PAR_NO_ERR;
}

int
parse(tree_t *tree, token_t *tokens, int *pos, int *count)
{
        int err = 0;

        if (tokens[*count].type == DIFF_BRACE) {
                if (tokens[*count].val.closed == true) {
                        log("Expected opening brace.\n");
                        return PAR_BRACE;
                }

                // Create node if meet opening brace.
                if (node_insert(tree, pos, {DIFF_POISON, 0}) != ERR_NO_ERR)
                        return PAR_INSERT;

                // Proceed to next token.
                (*count)++;

                if (tokens[*count].type == DIFF_OP &&
                   (tokens[*count].val.op == OP_SIN ||
                    tokens[*count].val.op == OP_COS ||
                    tokens[*count].val.op == OP_LN)) {
                        if ((err = parse_subtree(tree, tokens, pos, 
                                                        count)) != PAR_NO_ERR)
                                return err;
                        return PAR_NO_ERR;
                }
                // If the next token is not brace, then initialize node.
                if (tokens[*count].type != DIFF_BRACE) {
                        if ((err = parse_node(tree, tokens, pos, 
                                                        count)) != PAR_NO_ERR)
                                return err;
                } else {
                        if ((err = parse_subtree(tree, tokens, pos, 
                                                        count)) != PAR_NO_ERR)
                                return err;
                }
        } else {
                log("Expected opening brace.\n");
                return PAR_BRACE;
        }

        return PAR_NO_ERR;
}

