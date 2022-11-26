#ifndef DIFF_TYPES_H
#define DIFF_TYPES_H

// Enum of available operations with numbers.
enum op_t {
        OP_ADD = 1,
        OP_SUB = 2,
        OP_MUL = 3,
        OP_DIV = 4,
        OP_POW = 5,
        OP_SIN = 6,
        OP_COS = 7,
};

enum math_const_t {
        CONST_E  = 1,
        CONST_PI = 2,
};

// Enum of available objects that can be differentiated.
enum diff_obj_type_t {
        DIFF_POISON = 0,
        DIFF_VAR    = 1,
        DIFF_NUM    = 2,
        DIFF_CONST  = 3,
        DIFF_OP     = 4,
#ifdef LEXER_H
        DIFF_BRACE  = 5,
#endif
};

// Union with object
union value_t {
        bool closed;           // brace
        char var;              // 'x'
        double num;            // value of number
        op_t op;               // op_t enum
        math_const_t m_const;  // const enum
};

#endif // DIFF_TYPES_H

