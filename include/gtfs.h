/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#ifndef GTFS_H
#define GTFS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    unsigned int stop_id;
    unsigned int stop_lat;
    unsigned int stop_lon;
} StopsHeader;

typedef struct {
    char *id;
    double lat;
    double lon;
    unsigned int graph_node;
} BusStop;

StopsHeader *parse_stops(const char *);

#endif
