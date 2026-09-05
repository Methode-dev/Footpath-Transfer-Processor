/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#include "kdtree.h"

static void free_nodes(KDNode *node)
{
    if (node == NULL)
        return;

    free_nodes(node->left);
    free_nodes(node->right);
    free(node);
}

void kdtree_free(KDTree *tree)
{
    if (tree == NULL)
        return;

    free_nodes(tree->root);
    free(tree);
}

static int compare_indices(const void *a, const void *b)
{
    unsigned int ia = *(const unsigned int *)a;
    unsigned int ib = *(const unsigned int *)b;
    double va;
    double vb;

    if (sort_lon) {
        va = sort_graph->nodes[ia].lon;
        vb = sort_graph->nodes[ib].lon;
    } else {
        va = sort_graph->nodes[ia].lat;
        vb = sort_graph->nodes[ib].lat;
    }
    return (va > vb) - (va < vb);
}

static double distance_squared(const Node *a, double lat, double lon)
{
    double lat_diff = a->lat - lat;
    double lon_diff = a->lon - lon;

    return lat_diff * lat_diff + lon_diff * lon_diff;
}

static void nearest(const KDNode *node, const Graph *graph, double lat, double lon, int depth, unsigned int *best_node, double *best_distance)
{
    if (node == NULL)
        return;

    const Node *current = &graph->nodes[node->graph_node];
    double distance = distance_squared(current, lat,  lon);
    bool split_lon = depth % 2;
    double difference;
    KDNode *near;
    KDNode *far;

    if (distance < *best_distance) {
        *best_distance = distance;
        *best_node = node->graph_node;
    }
    if (split_lon)
        difference = lon - current->lon;
    else
        difference = lat - current->lat;

    if (difference < 0) {
        near = node->left;
        far = node->right;
    } else {
        near = node->right;
        far = node->left;
    }
    nearest(near, graph, lat, lon, depth + 1, best_node, best_distance);
    if (difference * difference < *best_distance) {
        nearest(far, graph, lat, lon, depth + 1, best_node, best_distance);
    }
}

unsigned int kdtree_nearest(const KDTree *tree, const Graph *graph, double lat, double lon, double *distance)
{
    unsigned int best_node = 0;
    double best_distance = DBL_MAX;

    nearest(tree->root, graph, lat, lon, 0, &best_node, &best_distance);
    *distance = best_distance;
    return best_node;
}

static KDNode *build_tree(const Graph *graph, unsigned int *indices, unsigned int count, int depth)
{
    bool split_lon = depth % 2;
    unsigned int middle = count / 2;
    KDNode *node = malloc(sizeof(KDNode));


    if (node == NULL || count == 0)
        return NULL;
    sort_graph = graph;
    sort_lon = split_lon;
    qsort(indices, count, sizeof(*indices), compare_indices);
    node->graph_node = indices[middle];
    node->left = build_tree(graph, indices, middle, depth + 1);
    node->right = build_tree(graph, indices + middle + 1, count - middle - 1, depth + 1);
    return node;
}

KDTree *kdtree_build(const Graph *graph)
{
    KDTree *tree = malloc(sizeof(KDTree));
    unsigned int *indices = malloc(sizeof(unsigned int) * graph->node_count);

    if (tree == NULL || indices == NULL)
        return NULL;
    for (unsigned int i = 0; i < graph->node_count; i++)
        indices[i] = i;
    tree->root = build_tree(graph, indices, graph->node_count, 0);
    return tree;
}
