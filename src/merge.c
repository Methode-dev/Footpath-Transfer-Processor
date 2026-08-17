/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#include "merge.h"
#include "gtfs.h"

static unsigned int find_closest_node(Graph *graph, BusStop *stop)
{
    if (graph && stop) {
        return 1;
    }
    return 0;
}

void merge_stops(Graph *graph, BusStop *stops, unsigned int stop_count)
{
    for (unsigned int i = 0; i < stop_count; i++) {
        printf("%s: %f, %f -- %d\n", stops[i].id, stops[i].lat, stops[i].lon, graph->node_count);
    }
    find_closest_node(graph, &stops[0]);
}
