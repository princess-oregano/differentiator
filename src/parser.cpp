#include <stdio.h>
#include "log.h"
#include "parser.h"

int
parse(tree_t *tree, token_t *tokens, int *pos)
{
        static int count = 0;
        if (tokens[count].type == DIFF_BRACE) {
                if (tokens[count].val.closed == true) {
                        log("Expected opening brace.\n");
                        return PAR_BRACE;
                }

                // Create node if meet opening brace.
                node_insert(tree, pos, {DIFF_POISON, 0});

                // Proceed to next token.
                count++;

                // If the next token is not brace, then initialize node.
                if (tokens[count].type != DIFF_BRACE) {
                        // Check if the token is the only one in braces.
                        if (tokens[count + 1].type != DIFF_BRACE &&
                            tokens[count + 1].val.closed != true) {
                                log("Expected closing brace.\n");
                                return PAR_BRACE;
                        }

                        // Initialize token.
                        tree->nodes[*pos].data.type = tokens[count].type;
                        tree->nodes[*pos].data.val = tokens[count].val;

                        // Proceed to the next after closing brace token.
                        count += 2;
                        
                        return PAR_NO_ERR;
                } else {
                        // Check if brace after opening brace is not closing.
                        if (tokens[count].val.closed == true) {
                                log("Cannot initialize empty object.\n");
                                return PAR_BRACE;
                        }

                        // Init left child.
                        parse(tree, tokens, &tree->nodes[*pos].left);

                        // Init the parent node.
                        tree->nodes[*pos].data.type = tokens[count].type;
                        tree->nodes[*pos].data.val = tokens[count].val;

                        // Proceed to next token.
                        count++;

                        // Init the right child.
                        // TODO: add trigonometry support.
                        parse(tree, tokens, &tree->nodes[*pos].right);
                }
                
        }

        return PAR_NO_ERR;
}

