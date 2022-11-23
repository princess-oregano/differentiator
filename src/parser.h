#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "tree.h"

enum par_err {
        PAR_NO_ERR = 0,
        PAR_BRACE  = 1,
};

// Builds tree from tokens.
int
parse(tree_t *tree, token_t *tokens, int *pos);

#endif // PARSER_H
