#include <stdio.h>
#include "simplifier.h"
#include "tree_dump.h"
#include "log.h"
#include "differentiator.h"
#include "tex_dump.h"

int
main()
{
        open_log("log.html");
        tex_begin("dump.tex");

        tree_t eq {};
        tree_t diff {};
        tree_ctor(&eq, 100);
        tree_ctor(&diff, 100);

        diff_parse(&eq);

        include_graph(tree_graph_dump(&eq, VAR_INFO(eq)));
        tex_eq_dump(&eq);

        diff_take(&eq, &diff, &eq.root, &diff.root);
        include_graph(tree_graph_dump(&diff, VAR_INFO(diff)));
        tex_diff_dump(&diff);

        sim_eq(&diff, &diff.root);
        include_graph(tree_graph_dump(&diff, VAR_INFO(diff)));
        tex_sim_dump(&diff);

        tree_dtor(&eq);
        tree_dtor(&diff);

        tex_end();

        return 0;
}

