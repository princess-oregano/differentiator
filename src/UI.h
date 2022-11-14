#ifndef UI_H
#define UI_H

#include <stdio.h>
#include "akinator.h"

const char *const EXIT = "я ухожу";
const char *const AGREE = "да";
const char *const DISAGREE = "нет";
const char *const GUESS = "угадать"; 
const char *const DEFINE = "описать"; 
const char *const COMPARE = "сравнить"; 

enum ak_mode_t {
        MODE_GUESS = 0,
        MODE_DEFINE = 1,
        MODE_COMPARE = 2,
        MODE_EXIT = 3,
};

enum ans_t {
        ANS_FALSE = 0,
        ANS_TRUE = 1,
        ANS_LONG = 2,
        ANS_EXIT = 3,
};

struct game_save_t {
        char *filename = nullptr;
        FILE* stream = nullptr;
};

// Prints hello message.
void
print_hello();
// Opens save or creates a new one.
int
open_save(tree_t *tree, game_save_t *save);
// Closes save.
void
close_save(tree_t *tree, game_save_t *save);
// Gets choice.
int
get_choice();
// Asks user a quastion.
int
ask(char **buf);

#endif // UI_H

