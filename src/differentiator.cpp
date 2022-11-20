#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/mman.h>
#include "tree_dump.h"
#include "log.h"
#include "differentiator.h"

static int
get_2delim_buf(char **line, int delim1, int delim2, char *buffer)
{
        log("Entering %s.\n", __PRETTY_FUNCTION__);

        size_t count = 0;
        for ( ; buffer[count] != delim1 && buffer[count] != delim2; count++)
                ;

        *line = (char *) calloc (count + 1, sizeof(char));
        if (*line == nullptr) {
                log("Couldn't allocate memory.\n");
                return 0;
        }

        memcpy(*line, buffer, count);

        log("Exiting %s.\n", __PRETTY_FUNCTION__);
        
        return (int) count;
}

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

static void
data_init(char *str_data, tree_data_t *data)
{

}

// Restores node from description from the buffer(inorder traversal).
static void
build_node(tree_t *tree, char *buffer, int *pos)
{
        log("Entering %s.\n", __PRETTY_FUNCTION__);

        include_graph(tree_graph_dump(tree));

        assert(tree);
        assert(buffer);

        tree_data_t data = {};
        char *str_data = nullptr;
        char ch = '\0';
        static int i = 0;

        for ( ; isspace(buffer[i]); i++)
                ;

        if (buffer[i] == '(') {
                i++;
                node_insert(tree, pos, {VAL_POISON, 0});
                node_insert(tree, &tree->nodes[*pos].left, {VAL_POISON, 0});
                log("Left\n");
                build_node(tree, buffer, &tree->nodes[*pos].left);
                i += get_2delim_buf(&str_data, '(', ')', &buffer[i]) + 1;
                fprintf(stderr, "'%s'\n", str_data);
                fprintf(stderr, "*pos = %d\n", *pos);
                log("Center\n");
                tree->nodes[*pos].data.type = VAL_OP;
                tree->nodes[*pos].data.val.op = OP_ADD;
                node_insert(tree, &tree->nodes[*pos].right, {VAL_POISON, 0});
                log("Right\n");
                build_node(tree, buffer, &tree->nodes[*pos].right);
        } else {
                fprintf(stderr, "*pos = %d\n", *pos);
                i += get_2delim_buf(&str_data, '(', ')', &buffer[i]) + 1;
                fprintf(stderr, "'%s' = %d\n", str_data, atoi(str_data));
                tree->nodes[*pos].data.type = VAL_NUM;
                tree->nodes[*pos].data.val.num = atoi(str_data);
                return;
        }

        log("Exiting %s.\n", __PRETTY_FUNCTION__);

        return;
}

int
diff_parse(tree_t *tree)
{
        file_t file {};
        char *buffer = nullptr;

        get_file("test", &file, "r");
        read_file(&buffer, &file);

        build_node(tree, buffer, &tree->root);

        include_graph(tree_graph_dump(tree)); 

        return D_ERR_NO_ERR;
}

// Prints node data in specific format.
static void
print_node(tree_t *tree, int pos, FILE *stream, int level)
{
        log("Entering %s.\n", __PRETTY_FUNCTION__);

        assert(tree);
        assert(stream);

        if (pos < 0)
                return;

        level++;

        fprintf(stream, "(\'%d\'", tree->nodes[pos].data.val);

        if (tree->nodes[pos].left  == -1 &&
            tree->nodes[pos].right == -1) {
                fprintf(stream, ")\n");
        } else {
                print_node(tree, tree->nodes[pos].left,  stream, level);
                print_node(tree, tree->nodes[pos].right, stream, level);
                fprintf(stream, ")\n");
        }

        log("Exiting %s.\n", __PRETTY_FUNCTION__);
}

int
diff_save(tree_t *tree, const char *filename)
{
        log("Entering %s.\n", __PRETTY_FUNCTION__);

        file_t file;
        int ret = 0;
        if ((ret = get_file(filename, &file, "w")) != D_ERR_NO_ERR)
                return ret;

        setvbuf(file.stream, nullptr, _IOFBF, (size_t) file.stats.st_blksize);

        print_node(tree, tree->root, file.stream, 0);

        fclose(file.stream);

        log("Exiting %s.\n", __PRETTY_FUNCTION__);

        return D_ERR_NO_ERR;
}

