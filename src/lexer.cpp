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
        tmp = (token_t *) realloc(tok_arr->ptr, (size_t) cap * sizeof(token_t));

        if (tmp == nullptr) {
                log("Couldn't allocate momry for tokens.\n");
                return LEX_ALLOC;
        }

        tok_arr->ptr = tmp;
        tok_arr->cap = cap;

        return LEX_NO_ERR;
}

static int
get_2delim_buf(char **line, int delim1, int delim2, char *buffer)
{
        assert(line);
        assert(buffer);

        size_t count = 0;
        for ( ; buffer[count] != delim1 && buffer[count] != delim2; count++)
                ;

        *line = (char *) calloc (count + 1, sizeof(char));
        if (*line == nullptr) {
                log("Couldn't allocate memory.\n");
                return 0;
        }

        memcpy(*line, buffer, count);

        return (int) count;
}

static int
lex_long(char *buffer, token_t *token)
{
        assert(buffer);
        assert(token);

        int i = 0;

        char *str = nullptr;
        int count = get_2delim_buf(&str, '(', ')', buffer);

        // Check if number.
        bool number = true;
        int len = (int) strlen(str);
        while(i < len) {            // Later add support of float.
                if (!isdigit(str[i])) {
                        number = false;
                        break;
                }
                i++;
        }

        if (number) {
                token->type = DIFF_NUM;
                token->val.num  = atof(str);
                free(str);
                return count;
        }

        if (strcmp(str, "pi") == 0) {
                token->type = DIFF_CONST;
                token->val.m_const = CONST_PI;
        } else if (strcmp(str, "sin") == 0) {
                token->type = DIFF_OP;
                token->val.op = OP_SIN;
        } else if (strcmp(str, "cos") == 0) {
                token->type = DIFF_OP;
                token->val.op = OP_COS;
        } else if (strcmp(str, "ln") == 0) {
                token->type = DIFF_OP;
                token->val.op = OP_LN;
        } else {
                log("Invalid usage: unknown operation '%s'.\n", str);
                free(str);
                return -1;
        }

        free(str);

        return count;
}

int
lexer(char *buffer, tok_arr_t *tok_arr)
{
        assert(buffer);
        assert(tok_arr);

        lex_alloc(tok_arr, 100);

        int i = 0;
        int lex_ret = 0;
        int tok_count = 0;
        int len = (int) strlen(buffer) - 1;
        while (i < len) {
                while (isspace(buffer[i]))
                        i++;

                switch (buffer[i]) {
                        case '(':
                                tok_arr->ptr[tok_count].type = DIFF_BRACE;
                                tok_arr->ptr[tok_count].val.closed = 0;
                                tok_count++;
                                break;
                        case ')':
                                tok_arr->ptr[tok_count].type = DIFF_BRACE;
                                tok_arr->ptr[tok_count].val.closed = 1;
                                tok_count++;
                                break;
                        case 'x':
                                tok_arr->ptr[tok_count].type = DIFF_VAR;
                                tok_arr->ptr[tok_count].val.var = 'x';
                                tok_count++;
                                break;
                        case '+':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_ADD;
                                tok_count++;
                                break;
                        case '-':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_SUB;
                                tok_count++;
                                break;
                        case '*':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_MUL;
                                tok_count++;
                                break;
                        case '/':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_DIV;
                                tok_count++;
                                break;
                        case '^':
                                tok_arr->ptr[tok_count].type = DIFF_OP;
                                tok_arr->ptr[tok_count].val.op = OP_POW;
                                tok_count++;
                                break;
                        case 'e':
                                tok_arr->ptr[tok_count].type = DIFF_CONST;
                                tok_arr->ptr[tok_count].val.m_const = CONST_E;
                                tok_count++;
                                break;
                        default:
                                if ((lex_ret = lex_long(&buffer[i],
                                     &tok_arr->ptr[tok_count])) == -1) {
                                        log("Invalid command.\n");
                                        return LEX_INV_CH;
                                } else {
                                        i += lex_ret- 1;
                                        tok_count++;
                                }
                                break;
                }

                i++;

                if (tok_arr->cap > tok_count + 1) {
                        lex_alloc(tok_arr, tok_arr->cap * 2);
                }

        }

        return LEX_NO_ERR;
}

