/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#include "merge.h"
#include "gtfs.h"
#include "kdtree.h"


void merge_stops(Graph *graph, BusStop *stops, unsigned int stop_count, const KDTree *tree)
{
    double distance;

    for (unsigned int i = 0; i < stop_count; i++) {
        stops[i].graph_node = kdtree_nearest(tree, graph, stops[i].lat, stops[i].lon, &distance);
        if (distance > MAX_STOP_DISTANCE)
            stops[i].graph_node = UINT_MAX;
        if (stops[i].graph_node == UINT_MAX)
            printf("%s -> outside OSM\n", stops[i].id);
    }
}
