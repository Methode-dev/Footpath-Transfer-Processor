/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#include "osm.h"

/* Graph */
static void graph_init(Graph *graph, unsigned int node_count, Node *nodes);
static void graph_add_edge(Graph *graph, unsigned int from, unsigned int to, double weight);

/* Misc */
static unsigned int count_nodes(const char *file);
static size_t hash_id(long long id, size_t capacity);
static double weight_distance_calculation(const Node *a, const Node *b);
static void node_map_init(NodeMap *map, size_t node_count);
static void node_map_insert(NodeMap *map, long long id, size_t index);
static Node *get_node(NodeMap *map, Node *nodes, long long id);

/* Parse & process */
static void parse_node(xmlTextReaderPtr reader, Node *nodes, unsigned int *count);
static void parse_way(xmlTextReaderPtr reader, NodeMap *map, Node *nodes, Graph *graph);
static void process_nodes(xmlTextReaderPtr reader, Node *nodes, unsigned int *parsed_nodes);
static void process_ways(xmlTextReaderPtr reader, Node *nodes, NodeMap *map, Graph *graph);

/*
 * Graph
 */

static void graph_init(Graph *graph, unsigned int node_count, Node *nodes)
{
    graph->node_count = node_count;
    graph->nodes = nodes;
    graph->adj = calloc(
        node_count,
        sizeof *graph->adj
    );
}

static void graph_add_edge(Graph *graph, unsigned int from, unsigned int to, double weight)
{
    AdjList *list = &graph->adj[from];

    if (list->count == list->capacity) {
        list->capacity = ((list->capacity == 0) ? 4 : list->capacity * 2);
        list->edges = realloc(list->edges, sizeof(*list->edges) * list->capacity);
    }
    list->edges[list->count].to = to;
    list->edges[list->count].weight = weight;
    list->count++;
}

void graph_free(Graph *graph)
{
    for (unsigned int i = 0; i < graph->node_count; i++)
        free(graph->adj[i].edges);
    free(graph->adj);
    free(graph->nodes);
    free(graph);
}

/*
 * Misc & Nodes
 */

xmlTextReaderPtr get_reader(const char *file)
{
    xmlTextReaderPtr reader = xmlReaderForFile(file, NULL, XML_PARSE_HUGE);
    if (!reader) {
        printf("Failed to open %s\n", file);
        exit(1);
    }
    return reader;
}

static unsigned int count_nodes(const char *file)
{
    xmlTextReaderPtr reader = get_reader(file);
    unsigned int count = 0;

    while (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) != XML_READER_TYPE_ELEMENT)
            continue;
        else if (xmlStrEqual(xmlTextReaderConstName(reader), BAD_CAST "node"))
            count++;
    }
    xmlFreeTextReader(reader);
    if (count == 0)
        printf("WARNING: Total node count in OSM file == 0.\nProceeding...\n");
    return count;
}

static size_t hash_id(long long id, size_t capacity) // +/- fmix64 mixing function
{
    unsigned long long x = (unsigned long long)id;

    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;

    return (size_t)(x % capacity);
}

static double weight_distance_calculation(const Node *a, const Node *b)
{
    const double earth_radius = 6371000.0;
    double lat1 = a->lat * M_PI / 180.0;
    double lat2 = b->lat * M_PI / 180.0;
    double dlat = (b->lat - a->lat) * M_PI / 180.0;
    double dlon = (b->lon - a->lon) * M_PI / 180.0;
    double h = sin(dlat / 2.0) * sin(dlat / 2.0) + cos(lat1) * cos(lat2) * sin(dlon / 2.0) * sin(dlon / 2.0);

    return 2.0 * earth_radius * asin(sqrt(h));
}

static void node_map_init(NodeMap *map, size_t node_count)
{
    map->capacity = (node_count >= 16) ? node_count * 2 : 16; // To avoid collisions as much as possible
    map->entries = calloc(map->capacity, sizeof(*map->entries));
}

static void node_map_insert(NodeMap *map, long long id, size_t index)
{
    size_t position = hash_id(id, map->capacity);

    while (map->entries[position].used) {
        if (map->entries[position].id == id) {
            map->entries[position].index = index;
            return;
        }
        position = (position + 1) % (map->capacity - 1);
    }
    map->entries[position].id = id;
    map->entries[position].index = index;
    map->entries[position].used = true;
}

static Node *get_node(NodeMap *map, Node *nodes, long long id)
{
    size_t position = hash_id(id, map->capacity);

    while (map->entries[position].used) {
        if (map->entries[position].id == id)
            return &nodes[map->entries[position].index];
        position = (position + 1) % map->capacity;
    }
    return NULL;
}

/*
 * Parse & process
 */

static void parse_node(xmlTextReaderPtr reader, Node *nodes, unsigned int *count)
{
    xmlChar *id = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
    xmlChar *lat = xmlTextReaderGetAttribute(reader, BAD_CAST "lat");
    xmlChar *lon = xmlTextReaderGetAttribute(reader, BAD_CAST "lon");

    nodes[*count].id = strtoll((char *)id, NULL, 10);
    nodes[*count].lat = strtod((char *)lat, NULL);
    nodes[*count].lon = strtod((char *)lon, NULL);
    (*count)++;
    xmlFree(id);
    xmlFree(lat);
    xmlFree(lon);
}

static void parse_way(xmlTextReaderPtr reader, NodeMap *map, Node *nodes, Graph *graph)
{
    const xmlChar *name;
    size_t ref_capacity = 16;
    long long *refs = malloc(sizeof(long long *) * ref_capacity);
    size_t ref_count = 0;
    int walkable = 0;

    while (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
            if (xmlStrEqual(xmlTextReaderConstName(reader), BAD_CAST "way"))
                break;
            continue;
        } else if (xmlTextReaderNodeType(reader) != XML_READER_TYPE_ELEMENT) {
            continue;
        }
        name = xmlTextReaderConstName(reader);
        if (xmlStrEqual(name, BAD_CAST "nd")) {
            xmlChar *ref = xmlTextReaderGetAttribute(reader, BAD_CAST "ref");
            if (ref_count == ref_capacity) {
                ref_capacity *= 2;
                long long *tmp = realloc(refs, sizeof(*refs) * ref_capacity);
                refs = tmp;

            }
            refs[ref_count++] = strtoll((char *)ref, NULL, 10);
            xmlFree(ref);
        } else if (xmlStrEqual(name, BAD_CAST "tag")) {
            xmlChar *key = xmlTextReaderGetAttribute(reader, BAD_CAST "k");
            xmlChar *value = xmlTextReaderGetAttribute(reader, BAD_CAST "v");
            if (key && value && xmlStrEqual(key, BAD_CAST "highway")) {
                if (
                    xmlStrEqual(value, BAD_CAST "footway") ||
                    xmlStrEqual(value, BAD_CAST "pedestrian") ||
                    xmlStrEqual(value, BAD_CAST "path") ||
                    xmlStrEqual(value, BAD_CAST "steps") ||
                    xmlStrEqual(value, BAD_CAST "living_street") ||
                    xmlStrEqual(value, BAD_CAST "residential") ||
                    xmlStrEqual(value, BAD_CAST "service") ||
                    xmlStrEqual(value, BAD_CAST "track") ||
                    xmlStrEqual(value, BAD_CAST "unclassified")
                )
                    walkable = 1;
            }
            xmlFree(key);
            xmlFree(value);
        }
    }
    if (walkable) {
        for (size_t i = 0; i + 1 < ref_count; i++) {
            Node *a = get_node(map, nodes, refs[i]);
            Node *b = get_node(map, nodes, refs[i + 1]);
            if (!a || !b)
                continue;
            double weight = weight_distance_calculation(a, b);
            graph_add_edge(graph, a - nodes, b - nodes, weight);
            graph_add_edge(graph, b - nodes, a - nodes, weight);
        }
    }
    free(refs);
}

static void process_nodes(xmlTextReaderPtr reader, Node *nodes, unsigned int *parsed_nodes)
{
    const xmlChar *name;

    while (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) != XML_READER_TYPE_ELEMENT)
            continue;
        name = xmlTextReaderConstName(reader);
        if (xmlStrEqual(name, BAD_CAST "node"))
            parse_node(reader, nodes, parsed_nodes);
    }
}

static void process_ways(xmlTextReaderPtr reader, Node *nodes, NodeMap *map, Graph *graph)
{
    const xmlChar *name;

    while (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) != XML_READER_TYPE_ELEMENT)
            continue;
        name = xmlTextReaderConstName(reader);
        if (xmlStrEqual(name, BAD_CAST "way"))
            parse_way(reader, map, nodes, graph);
    }
}

Graph *parse_osm(const char *filename)
{
    const unsigned int node_count = count_nodes(filename);
    xmlTextReaderPtr reader = get_reader(filename);
    Node *nodes = malloc(sizeof(Node) * (node_count + 1));
    NodeMap map;
    Graph *graph = malloc(sizeof(Graph *));
    unsigned int parsed_nodes = 0;

    process_nodes(reader, nodes, &parsed_nodes);
    node_map_init(&map, parsed_nodes);
    for (unsigned int i = 0; i < parsed_nodes; i++) {
        node_map_insert(&map, nodes[i].id, i);
    }
    graph_init(graph, parsed_nodes, nodes);
    reader = get_reader(filename);
    process_ways(reader, nodes, &map, graph);
    xmlFreeTextReader(reader);
    free(map.entries);
    return graph;
}
