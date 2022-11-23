#ifndef LEXER_H
#define LEXER_H

#include "types.h"

enum lex_err {
        LEX_NO_ERR  = 0,
        LEX_ALLOC   = 1,
        LEX_BAD_CAP = 2,
        LEX_INV_CH  = 3,
};

struct token_t {
        diff_obj_type_t type = DIFF_POISON;
        value_t val = {};
};

struct tok_arr_t {
        token_t *ptr = nullptr;
        int cap = 0;
};

// Builds an array of tokens.
int
lexer(char *buffer, tok_arr_t *tok_arr);

#endif // LEXER_H

