#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "tex_dump.h"
#include "system.h"
#include "log.h"

static replace_t REPLACE {};
const int REPL_WEIGHT = 20;
static FILE *TEX_STREAM = nullptr;
static const char *TEX_FILENAME = nullptr;
static const char *PHRASE[] = {
        "Чурбанов сказал, что",
        "Бог умер, и на его надгробии написано следующее",
        "Интересный факт: если Вы достаточно забавный, то Яворский съест Вас \
последним, и у Вас будет время кровью погибших кОлЛеГ записать, что",
        "Тут, очевидно, у любого возникли бы затруднения. Мы постараемся их \
измежать применив хитроумный ход, который, к сожалению, почти никогда не \
применяется семинаристами. На данном этапе взятия производной нам необходимо \
собраться силами. После данного приема возможны: тремор в руках, ощущение \
сильного алгольного опьянения, исключение из физтеха для студентов и \
немедленное увольнение с кафедры высшей математики для семинаристов. Сделаем \
следующее: перерыв на семинаре",
        "На первый взгляд, задача тривиальная, но есть нюанс. Впрочем, \
это не помешает нам вызвать к доске рандомного учащегося, который с легкостью \
запишет следующее",
        "Если на данном этапе кажется, что можно пропустить несколько шагов \
то не следует поддаваться соблазну. Следующее преобразование далеко не такое \
очевидно, как может показаться, и к нему следует отнестись со всей \
серьезностью",
        "Пусть нас не волнует Чурбанов, но переживать за его моральное \
состояние после такого все же стоит",
        "Иногда авторка мечтает, чтобы ее семинарист хоть решал задачи хоть \
немного подробнее, чем просто \"Ну... В общем, да, оно так будет. Ответ \
правильный, садитес.\"",
        "Саша Таранов готовит вкусные блинчики. Пока авторка их ела, \
производная взялась сама собой",
        "На данном этапе следует сказать учащимся, что они все молодцы, и \
вообще они солнышки. К сожалению, не каждому даются такие сложности, \
поэтому не стоит себя перенапрягать. Можно просто перейти к следующему \
действию",
        "Чурбанов...",
        "Интересный факт: в данной программе заготовлено около $20$ разных \
фраз, тогда вероятность выпадения этой конкретной фразы составляет порядка \
$5\\%$. Для сравнения, $3\\%$ жителей России на данный момент болеют раком",
        "Вас никогда не интересовало, почему нельзя переводиться у \
семинаристов по матану? Меня вот приговорили к году Чурбанова и я хочу \
аппелировать. Впрочем, не об этом",
        "Чтобы следующий этап показался более, чем легким, достаточно успеть \
к началу семинара, в крайнем случае, опаздать не более, чем на полчаса. Вообще \
достаточно в принципе проводить семинар, да, Дмитрий Владимирович?",
        "Прикол: мой семер потерял работы с семестровой",
        "Дай бог здоровья каждому, кто это читает...",
        "Однажды Чурбанов ехал со своей женой в маршрутке, и жена его \
спросила: <<Дорогой, а почему никогда не снимаешь кольцо, даже когда \
спишь?>>. Чурбанов ответил: <<Ну... Вот так вот... Да... Кто следующий \
по списку?>>. С женой плакала половина маршрутки",
        "Регулярная п-п-п-п-прецессия",
};
static const char *SIM_PHRASE = 
        "Наконец, мы подошли к упрощению выращения. Очевидно, что самая \
сложная часть заключается в аккуратном подведении студентов к правильному \
ответу, так как велика вероятность глупо ошибиться: пропустить минус, \
поставить лишнюю двойку, приравнять $3^3$ к $9$ и др. Поэтому сейчас мы \
воспользуемся техникой решения задач Дедиса Денкова, который всем \
известен за свою порой даже излишнюю подробность. В связи с этим \
последующие вычисления авторка не видит смысла даже комментировать: ну очев \
все понятно, где непонятно-то? Все видно же ну че ты ноеш халява жеж";
static const char *EQ_PHRASE = 
        "Для примера рассмотрим следующую функцию, которая потребует нашего \
тщательнейшего рассмотрения:";

static bool
check_leaf(tree_t *tree, int *pos)
{
        return (tree->nodes[*pos].left != -1 && tree->nodes[*pos].right != -1);
}

static bool
check_add_sub(tree_t *tree, int *pos)
{
        return (tree->nodes[*pos].data.type == DIFF_OP &&
               (tree->nodes[*pos].data.val.op == OP_ADD ||
                tree->nodes[*pos].data.val.op == OP_SUB));
}

#define IS_OP(NAME) (eq->nodes[*pos].data.type == DIFF_OP && \
                        eq->nodes[*pos].data.val.op == OP_##NAME)

static int
tex_find_weight(tree_t *eq, int *pos)
{
        int len1 = 0; 
        int len2 = 0;

        if (*pos == -1)
                return 1;
        if (eq->nodes[*pos].data.replace == true) 
                return 1;

        len1 += tex_find_weight(eq, &eq->nodes[*pos].left);
        len2 += tex_find_weight(eq, &eq->nodes[*pos].right);

        if (IS_OP(DIV)) {
                if (len1 >= len2)
                        len2 = 0;
                else 
                        len1 = 0;
        }
        if (IS_OP(POW)){
                len2 *= 0.3;
        }

        return len1 + len2;
}

static void
tex_const(tree_t *eq, int *pos, FILE *stream)
{
        switch (eq->nodes[*pos].data.val.m_const) {
                case CONST_E:
                        fprintf(stream, "e");
                        break;
                case CONST_PI:
                        fprintf(stream, "\\pi");
                        break;
                default:
                        log("Invalid const type encountered.\n");
                        assert(0 && "Invalid const type encountered.");
                        break;
        }
}

static void
tex_brace(tree_t *eq, int *pos, bool repl, 
          FILE *stream, bool (*check)(tree_t *, int *))
{
        bool brace = check(eq, pos);

        if (brace)
                fprintf(stream, " \\left(");
        tex_subtree(eq, pos, repl, stream);
        if (brace)
                fprintf(stream, " \\right)");
}

static void
tex_op(tree_t *eq, int *pos, bool repl, FILE *stream)
{
        tree_node_t *en = eq->nodes;

        switch (eq->nodes[*pos].data.val.op) {
                case OP_ADD:
                        tex_brace(eq, &en[*pos].left, repl, stream, check_add_sub);
                        fprintf(stream, " + ");
                        tex_brace(eq, &en[*pos].right, repl, stream, check_add_sub);
                        break;
                case OP_SUB:
                        tex_brace(eq, &en[*pos].left, repl, stream, check_add_sub);
                        fprintf(stream, " - ");
                        tex_brace(eq, &en[*pos].right, repl, stream, check_add_sub);
                        break;
                case OP_MUL:
                        tex_brace(eq, &en[*pos].left, repl, stream, check_add_sub);
                        fprintf(stream, " \\cdot ");
                        tex_brace(eq, &en[*pos].right, repl, stream, check_add_sub);
                        break;
                case OP_DIV:
                        fprintf(stream, " \\dfrac{");
                        tex_subtree(eq, &en[*pos].left, repl, stream);
                        fprintf(stream, "}{");
                        tex_subtree(eq, &en[*pos].right, repl, stream);
                        fprintf(stream, "} ");
                        break;
                case OP_POW:
                        fprintf(stream, "{");
                        tex_brace(eq, &en[*pos].left, repl, stream, check_leaf);
                        fprintf(stream, "}^{");
                        tex_subtree(eq, &en[*pos].right, repl, stream);
                        fprintf(stream, "}");
                        break;
                case OP_SIN:
                        fprintf(stream, " \\sin \\left(");
                        tex_subtree(eq, &en[*pos].right, repl, stream);
                        fprintf(stream, " \\right)");
                        break;
                case OP_COS:
                        fprintf(stream, " \\cos \\left(");
                        tex_subtree(eq, &en[*pos].right, repl, stream);
                        fprintf(stream, " \\right)");
                        break;
                case OP_LN:
                        fprintf(stream, " \\ln \\left(");
                        tex_subtree(eq, &en[*pos].right, repl, stream);
                        fprintf(stream, " \\right)");
                        break;
                default:
                        log("Invalid operation type encountered.\n");
                        assert(0 && "Invalid operation type encountered.");
                        break;
        }
}

static int
find_replace(int pos)
{
        int i = 0;
        for ( ; REPLACE.sub[i].subtree == pos; i++)
                ;

        return i;
}

static void
make_replace(int *pos)
{
        REPLACE.sub[REPLACE.size].letter = (char) REPLACE.size + 65;
        REPLACE.sub[REPLACE.size].subtree = *pos;
        REPLACE.size += 1;
}

void
tex_subtree(tree_t *eq, int *pos, bool repl, FILE *stream)
{
        tree_data_t data = eq->nodes[*pos].data;

        int len = 0;

        if (!repl) {
                len = tex_find_weight(eq, pos);
                if (len > REPL_WEIGHT && *pos != eq->root) {
                        make_replace(pos);
                        eq->nodes[*pos].data.replace = true;
                }

                if (eq->nodes[*pos].data.replace) {
                        fprintf(stream, " %c ", REPLACE.sub[find_replace(*pos)].letter);
                        return;
                }
        }

        if (data.copy == true)
                fprintf(stream, " {\\left(");
        switch (data.type) {
                case DIFF_POISON:
                        log("Invalid data type encountered.\n");
                        assert(0 && "Met poison node while tex dumping.");
                        break;
                case DIFF_VAR:
                        fprintf(stream, "%c", data.val.var);
                        break;
                case DIFF_NUM:
                        if (eq->nodes[*pos].data.val.num < 0)
                                fprintf(stream, " \\left(");
                        fprintf(stream, "%lg\n", data.val.num);
                        if (eq->nodes[*pos].data.val.num < 0)
                                fprintf(stream, " \\right)");
                        break;
                case DIFF_CONST:
                        tex_const(eq, pos, stream);
                        break;
                case DIFF_OP:
                        tex_op(eq, pos, repl, stream);
                        break;
                default:
                        break;
        }

        if (data.copy == true)
                fprintf(stream, " \\right)'}_{\\!x}");
}

static void
tex_intro()
{
        fprintf(TEX_STREAM, 
                "\\begin{titlepage}\n"
                "	\\centering\n"
                "	\\vspace*{5 cm}\n"
                "	\n"
                "	\\huge\\bfseries\n"
                "	Подробное решение задач на семинарах по дисциплине <<Математический анализ>>\n"
                "	\n"
                "	\\vspace*{0.5cm}\n"
                "	\n"
                "	\\large Кулевич Анастасия \\\\ РТРТРТРТРТРТРТ\n"
                "	\n"
                "	\\vspace*{0.5cm}\n"
                "	\\large 31.02.2077\n"
                "	\n"
                "	\\vspace*{\\fill}\n"
                "\\end{titlepage}\n"
                "\\section{Введение}\n"
                "\n"
                "Каждый читатель, наверное, не раз встречался со следующей \
проблемой: классический семинар по математическому анализу представляет из \
себя краткое теор. введение и несколько часов сладкого бота задач о героиновых \
шлюхах. И к сожалению, очень часто семинарист по каким-либо(разумеется, \
уважительным) причинам (напр. нехватка времени, желание собственноручно \
расчленить студентов, фамилия Чурбанов и др.) опускают многие важные моменты \
при решении задач.\n\n"

"Данная статья призывает каждого семинариста по мат. анализу не опускать \
никаких деталей в решении на примере взятия производной. По скромному мнению \
авторки, именно так должна находиться любая производная сложной функции, \
рассматриваемая на семинаре.\n\n"

"Посвящается лучшему семинаристу на свете, Чурбанову Дмитрию Владимировичу.\n\n"

"\\section{Взятие производной}\n\n");

}

void
tex_begin(const char *filename)
{
        srand((unsigned int) time(NULL));

        TEX_FILENAME = filename;
        TEX_STREAM = fopen(TEX_FILENAME, "w");

        setvbuf(TEX_STREAM, nullptr, _IONBF, 0);

        fprintf(TEX_STREAM, 
                "\\documentclass[12pt,a4paper]{article}\n"
                "\\usepackage[a4paper,top=1.5cm, bottom=1.5cm, left=1.5cm, right=1.5cm]{geometry}\n"
                "\\usepackage[T2A]{fontenc}\n"
                "\\usepackage[utf8]{inputenc}\n"
                "\\usepackage[russian]{babel}\n"
                "\\usepackage{amsmath}\n"
                "\\usepackage{amssymb}\n\n");

        fprintf(TEX_STREAM, "\\begin{document}\n\n");

        tex_intro();
}

void
tex_end()
{
        fprintf(TEX_STREAM, "\\end{document}\n");
        
        system_wformat("pdflatex -synctex=1 -interaction=nonstopmode "
                       "-interaction=batchmode %s", TEX_FILENAME);

        fclose(TEX_STREAM);
}

static void
tex_replace(tree_t *eq)
{
        fprintf(TEX_STREAM, " где ");

        for (int i = 0; i < REPLACE.size; i++) {
                fprintf(TEX_STREAM, "$%c = ", REPLACE.sub[i].letter);
                tex_subtree(eq, &REPLACE.sub[i].subtree, true, TEX_STREAM);
                fprintf(TEX_STREAM, "$,");
                fprintf(TEX_STREAM, "\n\n");
        }
}

void
tex_eq_dump(tree_t *eq)
{
        fprintf(TEX_STREAM, "%s\n", EQ_PHRASE);
        fprintf(TEX_STREAM, "\\[ \n f(x) = ");
        tex_subtree(eq, &eq->root, false, TEX_STREAM);
        fprintf(TEX_STREAM, "\\]\n");

        if (REPLACE.size != 0) {
                tex_replace(eq);
        }
}

void
tex_sim_dump(tree_t *eq)
{
        fprintf(TEX_STREAM, "%s\n", SIM_PHRASE);
        fprintf(TEX_STREAM, "\\[ \n f'(x) = ");
        tex_subtree(eq, &eq->root, false, TEX_STREAM);
        fprintf(TEX_STREAM, "\\]\n");

        if (REPLACE.size != 0) {
                tex_replace(eq);
        }
}

void 
tex_diff_dump(tree_t *eq)
{
        size_t curr = (size_t) rand() % (sizeof(PHRASE) / sizeof(char *));
        fprintf(TEX_STREAM, "%s\n", PHRASE[curr]);
        fprintf(TEX_STREAM, "\\[ \n f'(x) = ");
        tex_subtree(eq, &eq->root, false, TEX_STREAM);
        fprintf(TEX_STREAM, "\\]\n");
        
        if (REPLACE.size != 0) {
                tex_replace(eq);
        }
}

