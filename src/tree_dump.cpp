#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "differentiator.h"
#include "tree_dump.h"
#include "system.h"
#include "log.h"

// Stream of dump output.
static FILE *DMP_STREAM = nullptr;
static char DOT_FILENAME[FILENAME_MAX] = {};
static char PNG_FILENAME[FILENAME_MAX] = {};

// Opens a graphviz file to write dump to.
static void
open_graph_dump()
{
        DIR* dir = opendir("dmp");
        if (dir) {
                closedir(dir);
        } else if (ENOENT == errno) {
                mkdir("dmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        } else {
                fprintf(stderr, "Couldn't make a directory for dumps.\n");
        }

        char filename[] = "dmp/DUMP_XXXXXX";

        DMP_STREAM = fdopen(mkstemp(filename), "w");
        setvbuf(DMP_STREAM, nullptr, _IONBF, 0);

        memcpy(DOT_FILENAME, filename, strlen(filename) + 1);
}

// Genetares .png image from given dot code.
static char *
generate_graph()
{
        memcpy(PNG_FILENAME, DOT_FILENAME, strlen(DOT_FILENAME));
        strcat(PNG_FILENAME, ".png");

        system_wformat("%s %s %s %s", "dot -Tpng", DOT_FILENAME, ">", PNG_FILENAME);

        return PNG_FILENAME;
}

static void
op_node_graph_dump(tree_t *tree, int curr, int prev, int node_count)
{
        switch (tree->nodes[curr].data.val.op) {
                case OP_ADD:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%s\", shape = rect]\n",
                                node_count, curr, MATH_ADD);
                        break;
                case OP_SUB:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%s\", shape = rect]\n",
                                node_count, curr, MATH_SUB);
                        break;
                case OP_MUL:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%s\", shape = rect]\n",
                                node_count, curr, MATH_MUL);
                        break;
                case OP_DIV:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%s\", shape = rect]\n",
                                node_count, curr, MATH_DIV);
                        break;
        }
}

// Creates a graphviz dump of node and all nide's children.
static void
node_graph_dump(tree_t *tree, int curr, int prev)
{
        static int node_count = 1;

        // Need to add switch to print data correctly.
        switch (tree->nodes[curr].data.type) {
                case VAL_POISON: 
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\nNothing.\", shape = rect]\n",
                                node_count, curr);
                        break;
                        break;
                case VAL_VAR:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%d\", shape = rect]\n",
                                node_count, curr, tree->nodes[curr].data.val.var);
                        break;
                case VAL_NUM:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%lg\", shape = rect]\n",
                                node_count, curr, tree->nodes[curr].data.val.num);
                        break;
                case VAL_CONST:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%s\", shape = rect]\n",
                                node_count, curr, tree->nodes[curr].data.val.m_const);
                        break;
                case VAL_OP:
                        op_node_graph_dump(tree, curr, prev, node_count);
        }

        fprintf(DMP_STREAM,
        "node%d -> node%d [arrowhead = none]\n", prev, node_count++);

        prev = node_count - 1;

        if (tree->nodes[curr].left != -1)
                node_graph_dump(tree, tree->nodes[curr].left, prev);

        if (tree->nodes[curr].right != -1)
                node_graph_dump(tree, tree->nodes[curr].right, prev);
}

char *
tree_graph_dump(tree_t *tree)
{
        open_graph_dump();

        fprintf(DMP_STREAM,
        "digraph G {\n"
        "ranksep = 1.5\n"
        "graph [dpi = 100]\n"
        "splines = ortho\n");

        fprintf(DMP_STREAM,
        "node0[label = \"root\", shape = rect]\n");

        node_graph_dump(tree, tree->root, 0);

        fprintf(DMP_STREAM, "}\n");

        fclose(DMP_STREAM);

        return generate_graph();
}

