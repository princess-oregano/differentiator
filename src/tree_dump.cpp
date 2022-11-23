#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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

        system_wformat("%s %s %s %s", "dot -Tpng",
                        DOT_FILENAME, ">", PNG_FILENAME);

        return PNG_FILENAME;
}

static void
const_node_graph_dump(tree_t *tree, int curr, int node_count)
{
        switch (tree->nodes[curr].data.val.m_const) {
                case CONST_E:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\ne\", shape = rect]\n",
                                node_count, curr);
                        break;
                case CONST_PI:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\nπ\", shape = rect]\n",
                                node_count, curr);
                        break;
                default:
                        log("Invalid type encountered.\n");
                        assert(0 && "Invalid const type encountered.");
                        break;
        }
}

static void
op_node_graph_dump(tree_t *tree, int curr, int node_count)
{
        switch (tree->nodes[curr].data.val.op) {
                case OP_ADD:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n+\", shape = rect]\n",
                                node_count, curr);
                        break;
                case OP_SUB:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n-\", shape = rect]\n",
                                node_count, curr);
                        break;
                case OP_MUL:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n*\", shape = rect]\n",
                                node_count, curr);
                        break;
                case OP_DIV:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n/\", shape = rect]\n",
                                node_count, curr);
                        break;
                case OP_SIN:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\nsin\", shape = rect]\n",
                                node_count, curr);
                        break;
                case OP_COS:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\ncos\", shape = rect]\n",
                                node_count, curr);
                        break;
                default:
                        log("Invalid type encountered.\n");
                        assert(0 && "Invalid operation type encountered.");
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
                case DIFF_POISON:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\nVoid.\", shape = rect]\n",
                                node_count, curr);
                        break;
                case DIFF_VAR:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%c\", shape = rect]\n",
                                node_count, curr, tree->nodes[curr].data.val.var);
                        break;
                case DIFF_NUM:
                        fprintf(DMP_STREAM,
                                "node%d [label = \"%d\\n%lg\", shape = rect]\n",
                                node_count, curr, tree->nodes[curr].data.val.num);
                        break;
                case DIFF_CONST:
                        const_node_graph_dump(tree, curr, node_count);
                        break;
                case DIFF_OP:
                        op_node_graph_dump(tree, curr, node_count);
                        break;
                default:
                        log("Invalid type encountered.\n");
                        assert(0 && "Invalid data type encountered.");
                        break;
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

