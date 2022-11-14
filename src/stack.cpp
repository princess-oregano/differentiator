#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "error.h"
#include "stack.h"

#ifdef HASH
static unsigned int
crc8(unsigned int crc, void *data, size_t len)
{
        unsigned char const *data_ptr = (unsigned char const *)data;

        if (data == NULL)
                return 0;

        crc &= 0xff;

        unsigned char const *end = data_ptr + len;
        while (data_ptr < end)
                crc = crc8_table[crc ^ *data_ptr++];

        return crc;
}
#endif

static long long
data_resize(stack_t *stack, size_t capacity)
{
        err_u err {};

        char *data_ptr = (char *)stack->data ON_CANARY(- sizeof(unsigned long long));

        size_t data_size = capacity * sizeof(elem_t)
                 ON_CANARY(+ 2 * sizeof(unsigned long long));
        char *stk_data_ptr_tmp = (char *) realloc(data_ptr, data_size);

        if (stk_data_ptr_tmp == nullptr) {
                err.type.ERR_ALLOC = true;
                return err.val;
        }

#ifdef CANARY
        memcpy(stk_data_ptr_tmp, &CANARY_VAL, sizeof(CANARY_VAL));
        memcpy(stk_data_ptr_tmp + data_size - sizeof(unsigned long long),
               &CANARY_VAL, sizeof(CANARY_VAL));
#endif

        stack->data = (elem_t *) (stk_data_ptr_tmp ON_CANARY(+ sizeof(unsigned long long)));

        for (size_t i = stack->capacity; i < capacity; i++)
                memset(&stack->data[i], DATA_POISON, sizeof(elem_t));

        stack->capacity = capacity;

#ifdef HASH
        stack->crc_hash = 0;
        stack->crc_hash = crc8(stack->crc_hash, stack, sizeof(stack_t));
#endif

        return err.val;
}

long long
stack_ctor(stack_t *stack, unsigned int capacity, var_info_t var_info)
{
        assert(stack);

        err_u err {};
        char *tmp_data_ptr = nullptr;

        size_t data_size = capacity * sizeof(elem_t)
                 ON_CANARY(+ 2 * sizeof(unsigned long long));
        tmp_data_ptr = (char *) calloc(data_size, 1);

        if (tmp_data_ptr == nullptr) {
                err.type.ERR_ALLOC = 1;
                return err.val;
        }

#ifdef CANARY
        memcpy(tmp_data_ptr, &CANARY_VAL, sizeof(CANARY_VAL));
        memcpy(tmp_data_ptr + data_size - sizeof(unsigned long long),
               &CANARY_VAL, sizeof(CANARY_VAL));
#endif

        stack->data = (elem_t *)(tmp_data_ptr ON_CANARY(+ sizeof(unsigned long long)));

        for (size_t i = 0; i < capacity; i++)
                memset(&stack->data[i], DATA_POISON, sizeof(elem_t));
#ifdef HASH

        stack->crc_hash = crc8(0, stack, sizeof(stack_t));
#endif
        stack->var_info = var_info;
        stack->capacity = capacity;

        return err.val;
}

long long
stack_push(stack_t *stack, elem_t elem)
{
        assert(stack);

        err_u err {};

        if (stack->size >= stack->capacity) {
                if ((err.val |= data_resize(stack, stack->capacity*2)) != 0)
                        return err.val;
        }

        stack->data[stack->size++] = elem;

#ifdef HASH
        stack->crc_hash = 0;
        stack->crc_hash = crc8(stack->crc_hash, stack, sizeof(stack_t));
#endif

        return err.val;
}

long long
stack_pop(stack_t *stack, elem_t *ret_val)
{
        assert(stack);
        verify_stack(stack);

        err_u err {};

        stack->size--;
        *ret_val = stack->data[stack->size];
        memset(&stack->data[stack->size], DATA_POISON, sizeof(elem_t));

        if (2 * stack->size < stack->capacity && stack->capacity > 10) {
                err.val |= data_resize(stack, stack->capacity/2);
        }

#ifdef HASH
        stack->crc_hash = 0;
        stack->crc_hash = crc8(stack->crc_hash, stack, sizeof(stack_t));
#endif

        return err.val;
}

long long
stack_dtor(stack_t *stack)
{
        assert(stack);
        err_u err {};

        for (size_t i = 0; i < stack->capacity; i++)
                stack->data[i] = 0;
        stack->size = (unsigned int) -1;
        stack->capacity = (unsigned int) -1;

        stack->data = (elem_t *)((char *)stack->data ON_CANARY(- sizeof(unsigned long long)));
        if (stack->data != nullptr)
                free(stack->data);

        stack->data = (elem_t *) 0xDEADDEADDEADDEAD;

        return err.val;
}

static void
data_dump(stack_t *stack)
{
        size_t i = 0;

        for (i = 0; i < stack->capacity; i++) {
                char *ch = (char *) &stack->data[i];
                for (size_t j = 0; j < sizeof(elem_t); j++) {
                        fprintf(stderr, "%02hhx ", *(ch + j));
                }
                fprintf(stderr, "\n");
        }
}

void
stack_dump(stack_t *stack, var_info_t cur_var_info)
{
        fprintf(stderr,
                "%s at %s(%d):\n"
                "%s[%p] \"%s\" at %s at %s(%d)\n"
        ON_HASH("       hash = %u\n")
                "       size = %zu\n"
                "       capacity = %zu\n"
                "       data[%p]:\n",
                cur_var_info.func_name, cur_var_info.file_name,
                cur_var_info.line, cur_var_info.init_var_name,
                stack, stack->var_info.init_var_name,
                stack->var_info.func_name, stack->var_info.file_name,
                stack->var_info.line, ON_HASH(stack->crc_hash,) stack->size,
                stack->capacity, stack->data);

        data_dump(stack);
}

long long
verify_stack(stack_t *stack)
{
        assert(stack);

        err_u err {};
#ifdef CANARY
        if (stack-> left_canary != CANARY_VAL ||
            stack->right_canary != CANARY_VAL)
               err.type.ERR_STACK_CANARY = true;

        if ((*(unsigned long long *)((char *) stack->data
             - sizeof(unsigned long long))) != CANARY_VAL ||
            (*(unsigned long long *)((char *) stack->data
             + sizeof(elem_t)*stack->capacity)) != CANARY_VAL)
                return err.type.ERR_DATA_CANARY = true;
#endif

#ifdef HASH
        unsigned int hash = stack->crc_hash;

        stack->crc_hash = 0;
        stack->crc_hash = crc8(stack->crc_hash, stack, sizeof(stack_t));

        if (hash != stack->crc_hash) {
                err.type.ERR_STACK_HASH = true;
                stack->crc_hash = hash;
        }
#endif

        return err.val;
}

