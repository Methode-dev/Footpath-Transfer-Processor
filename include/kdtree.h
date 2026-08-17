/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#ifndef KDTREE_H
#define KDTREE_H

#include <stdbool.h>
#include <float.h>
#include "osm.h"

static const Graph *sort_graph;
static bool sort_lon;

typedef struct KDNode {
    unsigned int graph_node;
    struct KDNode *left;
    struct KDNode *right;
} KDNode;

typedef struct {
    KDNode *root;
} KDTree;

KDTree *kdtree_build(const Graph *);
void kdtree_free(KDTree *);
int kdtree_nearest(const KDTree *, const Graph *, double, double);

#endif
