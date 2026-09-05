/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#ifndef MERGE_H
#define MERGE_H

#include "gtfs.h"
#include "kdtree.h"
#include "osm.h"

#define MAX_STOP_DISTANCE (0.005 * 0.005)

void merge_stops(Graph *, BusStop *, unsigned int, const KDTree *);

#endif
