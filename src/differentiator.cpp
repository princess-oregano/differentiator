#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include "differentiator.h"
#include "lexer.h"
#include "parser.h"
#include "tree_dump.h"
#include "log.h"

// Makes a structure with info about file.
static int
get_file(const char *filename, file_t *file, const char *mode)
{
        log("Entering %s.\n", __PRETTY_FUNCTION__);

        if ((file->stream = fopen(filename, mode)) == nullptr) {
                log("Error: Couldn't open %s.\n", filename);

                return D_ERR_OPEN;
        }

        if (stat(filename, &file->stats) != 0) {
                log("Error: Coudn't get stats of %s.\n", filename);
                return D_ERR_STATS;
        }

        log("Exiting %s.\n", __PRETTY_FUNCTION__);

        return D_ERR_STATS;
}

// Reads file and puts its contents to a buffer.
static int
read_file(char **buffer, file_t *file)
{
        log("Entering %s.\n", __PRETTY_FUNCTION__);

        assert(file);
        assert(buffer);

        *buffer = (char *) mmap(NULL, (size_t) file->stats.st_size, PROT_READ,
                               MAP_PRIVATE, fileno(file->stream), 0);

        if (*buffer == MAP_FAILED) {
                log("Error: Couldn't allocate memory.\n");
                log("Exiting %s.\n", __PRETTY_FUNCTION__);
                return D_ERR_MAP;
        }


        log("Exiting %s.\n", __PRETTY_FUNCTION__);

        return D_ERR_NO_ERR;
}

int
diff_parse(tree_t *tree)
{
        file_t file {};
        char *buffer = nullptr;
        tok_arr_t tok_arr {};

        get_file("test", &file, "r");
        read_file(&buffer, &file);

        lexer(buffer, &tok_arr);

        parse(tree, tok_arr.ptr, &tree->root);

        free(tok_arr.ptr);
        include_graph(tree_graph_dump(tree)); 

        return D_ERR_NO_ERR;
}

