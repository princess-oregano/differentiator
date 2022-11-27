#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "lexer.h"
#include "log.h"

static int
lex_alloc(tok_arr_t *tok_arr, int cap)
{
        if (cap < 0) {
                log("Capacity must be larger or equal to 0.\n");
                return LEX_BAD_CAP;
        }

        token_t *tmp = nullptr;
        fprintf(stderr, "cap = %d\n", cap);
        tmp = (token_t *) realloc(tok_arr->ptr, (size_t) cap * sizeof(token_t));

        if (tmp == nullptr) {
                log("Couldn't allocate momry for tokens.\n");
                return LEX_ALLOC;
        }

        tok_arr->ptr = tmp;
        tok_arr->cap = cap;

        return LEX_NO_ERR;
}

static ssize_t
lex_long(char *buffer, token_t *token)
{
        assert(buffer);
        assert(token);

        int i = 0;

        char *str = nullptr;
        str = strpbrk(buffer, "()");
        ssize_t len = str - buffer;

        // Check if number.
        bool number = true;
        while(i < len) {            // Later add support of float.
                if (!isdigit(buffer[i])) {
                        number = false;
                        break;
                }
                i++;
        }

        char *dummy = nullptr;
        if (number) {
                token->type = DIFF_NUM;
                token->val.num  = strtof(buffer, &dummy);
                return len;
        }

        if (strncmp(buffer, "pi", (size_t) len) == 0) {
                token->type = DIFF_CONST;
                token->val.m_const = CONST_PI;
        } else if (strncmp(buffer, "sin", (size_t) len) == 0) {
                token->type = DIFF_OP;
                token->val.op = OP_SIN;
        } else if (strncmp(buffer, "cos", (size_t) len) == 0) {
                token->type = DIFF_OP;
                token->val.op = OP_COS;
        } else if (strncmp(buffer, "ln", (size_t) len) == 0) {
                token->type = DIFF_OP;
                token->val.op = OP_LN;
        } else {
                log("Invalid usage: unknown operation.\n");
                return -1;
        }

        return len;
}

int
lexer(char *buffer, tok_arr_t *tok_arr)
{
        assert(buffer);
        assert(tok_arr);

        lex_alloc(tok_arr, 100);

        int i = 0;
        ssize_t lex_ret = 0;
        int tok_count = 0;
        int len = (int) strlen(buffer) - 1;
        while (i < len) {
                while (isspace(buffer[i]))
                        i++;

                switch (buffer[i]) {
                        case '(':
                                tok_arr->ptr[tok_count].type = DIFF_BRACE;
                                tok_arr->ptr[tok_count].val.closed = 0;
                                break;
                        case ')':
                                tok_arr->ptr[tok_count].type = DIFF_BRACE;
                                tok_arr->ptr[tok_count].val.closed = 1;
                                break;
                        case 'x':
                                tok_arr->ptr[tok_count].type = DIFF_VAR;
                                tok_arr->ptr[tok_count].val.var = 'x';
                                break;
                        case '+':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_ADD;
                                break;
                        case '-':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_SUB;
                                break;
                        case '*':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_MUL;
                                break;
                        case '/':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_DIV;
                                break;
                        case '^':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_POW;
                                break;
                        case 'e':
                                tok_arr->ptr[tok_count].type = DIFF_CONST;
                                tok_arr->ptr[tok_count].val.m_const = CONST_E;
                                break; 
                        default:
                                if ((lex_ret = lex_long(&buffer[i],
                                     &tok_arr->ptr[tok_count])) == -1) {
                                        log("Invalid command.\n");
                                        return LEX_INV_CH;
                                } else {
                                        i += (int) lex_ret- 1;
                                }
                                break;
                }

                tok_count++;
                i++;

                if (tok_arr->cap < tok_count + 1) {
                        lex_alloc(tok_arr, tok_arr->cap * 2);
                }

        }

        return LEX_NO_ERR;
}

