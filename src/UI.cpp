#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "akinator.h"
#include "UI.h"

void
print_hello()
{
        printf(
        "                                       ;\\                        \n"
        "                                      _' \\_                      \n"
        " Приветствую.                       ,' '  '`.                     \n"
        " Меня зовут Баст, мудрейшая        ;,)       \\                   \n"
        " из кошек. Загадай мне            /          :                    \n"
        " любой oбъект и я его отгадаю,    (_         :                    \n"
        " или, может быть, чему-то          `--.       \\                  \n"
        " научусь.                             /        `.                 \n"
        "                                     ;           `.               \n"
        "                                    /              `.             \n"
        "                                    :                 `.          \n"
        "                                    :                   \\        \n"
        "                                     \\                   \\      \n"
        "                                      ::                 :        \n"
        "                                      || |               |        \n"
        "                                      || |`._            ;        \n"
        "                                     _;; ; __`._,       (________ \n"
        "                                   ((__/(_____(______,'______(___)\n");

        printf("Возможно, мы раньше играли в такую игру.\n"
               "Как мы ее тогда назвали?\n");
}

int
open_save(tree_t *tree, game_save_t *save)
{
        char *filename = nullptr;
        int ans = 0;

        while ((ans = ask(&filename)) != ANS_LONG) {
                switch (ans) {
                        case ANS_TRUE:
                                free(filename);
                                fprintf(stdout, "Да?..\n");
                                break;
                        case ANS_FALSE:
                                free(filename);
                                fprintf(stdout, "Нет?..\n");
                                break;
                        case ANS_EXIT:
                                fprintf(stdout, "Я обиделась!\n");
                                free(filename);
                                return AK_EXIT;
                        case ANS_LONG:
                                break;
                        default:
                                assert(0 && "Invalid ans value.");
                }
        }

        FILE *stream = fopen(filename, "r");
        if (stream == nullptr) {
                printf("Я не помню такой игры.\n");
                printf("Давай попробуем ее начать.\n");
                stream = fopen(filename, "w");
                if (stream == nullptr) {
                        printf("Не получилось...\n");
                        save->stream = stream;
                        save->filename = filename;

                        return AK_ERROR;
                }
                int start_ret = ak_start(tree, stdout);
                if (start_ret != AK_NORMAL) {
                        fclose(stream);
                        free(filename);

                        return start_ret;
                }
        } else {
                ak_restore(tree, filename);
                printf("Да, я вспомнила.\n");
                printf("Начнем игру.\n");
        }

        printf("Я мудрая кошка, поэтому занимаюсь только умными делами.\n"
               "Я могу угадать что-то, описать или сравнить.\n");

        save->stream = stream;
        save->filename = filename;

        return AK_NORMAL;
}

void
close_save(tree_t *tree, game_save_t *save)
{
        assert(tree);
        assert(save);

        free(save->filename);
        fclose(save->stream);
}

int
get_choice()
{
        fprintf(stdout, "Что мне сделать?\n");
        char *answer = nullptr;
        int ans = 0;
        while (ans != ANS_EXIT) {
                ans = ask(&answer);
                switch(ans) {
                        case ANS_TRUE:
                                fprintf(stdout, "Да?..\n");
                                free(answer);
                                break;
                        case ANS_FALSE:
                                fprintf(stdout, "Нет?..\n");
                                free(answer);
                                break;
                        case ANS_EXIT:
                                fprintf(stdout, "Мне и самой не очень-то "
                                                "хотелось играть...\n");
                                free(answer);
                                break;
                        case ANS_LONG:
                                if (strcmp(answer, GUESS) == 0) {
                                        free(answer);
                                        return MODE_GUESS; 
                                } else if (strcmp(answer, DEFINE) == 0) {
                                        free(answer);
                                        return MODE_DEFINE;
                                } else if (strcmp(answer, COMPARE) == 0) {
                                        free(answer);
                                        return MODE_COMPARE;
                                } else {
                                        free(answer);
                                        fprintf(stdout, "Не поняла.\n");
                                }
                                break;
                        default:
                                assert(0 && "Invalid ans value.");
                }
        }

        return MODE_EXIT;
}

static ssize_t 
getline_f(char **lineptr, size_t *n, FILE *stream)
{
        char *ptr_ch = NULL;
        char *max_ptr_ch = NULL;
        size_t buf_size = *n;
        size_t max_buf_size = 0;
        int i = 0;

        if (*lineptr == NULL || *n == 0) {
                buf_size = 3;
                max_buf_size = 2 * buf_size;
                if((*lineptr = (char *) calloc(buf_size, sizeof(char))) == NULL)
                        return -1;
        }

        int ch = 0;
        while (isspace(ch = fgetc(stream)))
                ;

        for (ptr_ch = *lineptr, max_ptr_ch = *lineptr + buf_size; ; i++) {
                if (ch == EOF)
                        return -1;

                *ptr_ch++ = (char) ch;

                if (ch == '\n') {
                        *ptr_ch = '\0';
                        return ptr_ch - *lineptr;
                }
 
                if (ptr_ch + 2 >= max_ptr_ch) {
                        max_buf_size = 2 * buf_size;
                        ssize_t delta = ptr_ch - *lineptr;
                        char *new_lineptr = NULL;

                        if ((new_lineptr = (char *) realloc(*lineptr, (max_buf_size * sizeof(char)))) == NULL)
                                return -1;

                        buf_size = max_buf_size;
                        *lineptr = new_lineptr;
                        ptr_ch = new_lineptr + delta;
                        max_ptr_ch = new_lineptr + max_buf_size;
                }

                ch = fgetc(stream);
        }

        // Unreachable.
        return 0;
}

int
ask(char **buf)
{
        assert(buf);

        fprintf(stdout, " $ ");

        char *data = nullptr;
        size_t buf_size = 0;

        getline_f(&data, &buf_size, stdin);
        buf_size = strlen(data);

        size_t i = 0;
        for (i = buf_size - 1; i > 0 && isspace(data[i]); i--)
                ;
        data[i + 1] = '\0';
        data = (char *) realloc(data, i + 2 * sizeof(char));

        *buf = data;

        if (strcmp(*buf, AGREE) == 0) 
                return ANS_TRUE;
        else if (strcmp(*buf, DISAGREE) == 0)
                return ANS_FALSE;
        else if (strcmp(*buf, EXIT) == 0)
                return ANS_EXIT;
        else 
                return ANS_LONG;
}

