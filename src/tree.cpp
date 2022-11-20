#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tree.h"
#include "log.h"

static int
tree_resize(tree_t *tree, int new_cap)
{
        log("Entered %s.\n", __PRETTY_FUNCTION__);

        if (new_cap <= 0) {
                log("Error: Invalid capacity.\n"
                    "Exiting %s.\n", __PRETTY_FUNCTION__);

                return ERR_BAD_CAP;
        }

        tree_node_t *tmp_nodes = (tree_node_t *) realloc(tree->nodes,
                                 (size_t) new_cap * sizeof(tree_node_t));

        if (tmp_nodes == nullptr) {
                log("Error: Couldn't allocate memory for nodes.\n"
                    "Exiting %s.\n", __PRETTY_FUNCTION__);

                return ERR_ALLOC;
        }

        tree->nodes = tmp_nodes;

        for (int i = tree->cap; i < new_cap; i++) {
                tree->nodes[i].data = {};
                tree->nodes[i].left = -1;
                tree->nodes[i].right = -1;
                tree->nodes[i].next_free = i + 1;
        }

        tree->nodes[tree->cap].next_free = tree->cap + 1;
        tree->cap = new_cap;

        log("Exiting %s.\n", __PRETTY_FUNCTION__);

        return ERR_NO_ERR;
}

int
tree_ctor(tree_t *tree, int cap)
{
        log("Entered %s.\n", __PRETTY_FUNCTION__);

        int err = ERR_NO_ERR;

        if ((err = tree_resize(tree, cap)) != ERR_NO_ERR)
                return err;

        tree->free = 0;

        log("Exiting %s.\n", __PRETTY_FUNCTION__);

        return ERR_NO_ERR;
}

int
node_insert(tree_t *tree, int *parent, tree_data_t data)
{
        log("Entered %s.\n", __PRETTY_FUNCTION__);

        assert(tree); 
        assert(parent);
        assert(*parent >= -1);

        tree->nodes[tree->free].data = data;
        node_bound(parent, tree->free);
        tree->free = tree->nodes[tree->free].next_free;

        if (tree->free >= tree->cap) {
                log("Free = %d, capacity = %d\n", tree->free, tree->cap);
                log("Resizing...\n");
                int err = ERR_NO_ERR;
                if ((err = tree_resize(tree, tree->cap * 2)) != ERR_NO_ERR)
                        return err;
        }

        log("Exiting %s.\n", __PRETTY_FUNCTION__);

        return ERR_NO_ERR;
}

void
node_bound(int *parent, int node)
{
        log("Entered %s.\n", __PRETTY_FUNCTION__);

        assert(parent);

        if (*parent != -1 && node != 0) {
                log("Warning: pointer is already initialized.\n"
                    "Bounding may lead to loss of data.n");
        }

        *parent = node;

        log("Exiting %s.\n", __PRETTY_FUNCTION__);
}

int
node_remove(tree_t *tree, int *pos)
{
        log("Entered %s.\n", __PRETTY_FUNCTION__);

        assert(tree);
        if (*pos < 0) {
                log("Invalid position.\n");
                return ERR_BAD_POS;
        }

        tree->nodes[*pos].data = {};

        if (tree->nodes[*pos].left != -1)
                node_remove(tree, &tree->nodes[*pos].left);

        if (tree->nodes[*pos].right != -1)
                node_remove(tree, &tree->nodes[*pos].right);

        tree->nodes[*pos].left = -1;
        tree->nodes[*pos].right = -1;

        tree->nodes[*pos].next_free = tree->free;
        tree->free = *pos;

        *pos = -1;

        log("Exiting %s.\n", __PRETTY_FUNCTION__);

        return ERR_NO_ERR;
}

int
tree_dtor(tree_t *tree)
{
        int err = ERR_NO_ERR;
        if ((err = node_remove(tree, &tree->root)) != ERR_NO_ERR)
                return err;
        free(tree->nodes);

        tree->root = -1;
        tree->cap = -1;
        tree->free = -1;

        return ERR_NO_ERR;
}

