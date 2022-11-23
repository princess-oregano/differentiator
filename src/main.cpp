#include <stdio.h>
#include "tree_dump.h"
#include "log.h"
#include "differentiator.h"

int
main()
{
        open_log("log.html");

        tree_t eq {};
        tree_t diff {};
        tree_ctor(&eq, 100);
        tree_ctor(&diff, 100);

        diff_parse(&eq);

        include_graph(tree_graph_dump(&eq));

        diff_take(&eq, &diff, &eq.root, &diff.root);

        include_graph(tree_graph_dump(&eq));
        include_graph(tree_graph_dump(&diff));

        tree_dtor(&eq);
        tree_dtor(&diff);

        return 0;
}

