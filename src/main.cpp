#include <stdio.h>
#include "tree_dump.h"
#include "log.h"
#include "differentiator.h"

int
main()
{
        open_log("log.html");
        tree_t tree {};
        tree_ctor(&tree, 100);

        diff_parse(&tree);
        fprintf(stderr, "left = %lg, center = %d, right = %lg", 
                tree.nodes[tree.nodes[tree.root].left].data.val.num, 
                tree.nodes[tree.root].data.val.op,
                tree.nodes[tree.nodes[tree.root].right].data.val.num);

        include_graph(tree_graph_dump(&tree));

        tree_dtor(&tree);

        return 0;
}

