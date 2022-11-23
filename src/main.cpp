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

        include_graph(tree_graph_dump(&tree));

        tree_dtor(&tree);

        return 0;
}

