#include <stdio.h>
#include "simplifier.h"
#include "tree_dump.h"
#include "log.h"
#include "differentiator.h"
#include "tex_dump.h"
#include "args.h"

int
main(int argc, char *argv[])
{
        open_log("log.html");

        params_t params {};
        process_args(argc, argv, &params);

        tree_t eq {};
        tree_t diff {};
        tree_ctor(&eq, 100);
        tree_ctor(&diff, 200);

        file_t file {};
        char *buffer = nullptr;
        get_file(params.src_filename, &file, "r");
        if (read_file(&buffer, &file) == ERR_ALLOC)
                return ERR_ALLOC;

        diff_parse(&eq, buffer);

        tex_begin("dump.tex");

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
        fclose(file.stream);
        tex_end();

        return 0;
}

