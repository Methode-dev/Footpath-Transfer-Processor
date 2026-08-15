/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#ifndef OSM_H
#define OSM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <libxml/xmlstring.h>
#include <libxml/xmlreader.h>

typedef struct {
    long long id;
    double lat;
    double lon;
} Node;

typedef struct {
    long long id;
    size_t index;
    bool used;
} NodeIndex;

typedef struct {
    NodeIndex *entries;
    size_t capacity;
} NodeMap;

typedef struct {
    unsigned int to;
    double weight;
} Edge;

typedef struct {
    Edge *edges;
    unsigned int count;
    unsigned int capacity;
} AdjList;

typedef struct {
    AdjList *adj;
    Node *nodes;
    unsigned int node_count;
} Graph;

xmlTextReaderPtr get_reader(const char *file);
Graph *parse_osm(const char *filename);
void graph_free(Graph *graph);

#endif
