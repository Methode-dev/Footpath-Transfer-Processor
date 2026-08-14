#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <libxml/xmlreader.h>

#define EARTH_RADIUS_M 6371000.0
#define INITIAL_CAPACITY 1024


/* ============================================================
 * NODE
 * ============================================================ */

typedef struct {
    int64_t id;
    double lat;
    double lon;
} Node;


/* ============================================================
 * NODE HASH MAP
 * ============================================================ */

typedef struct {
    int64_t key;
    Node value;
    int used;
} NodeEntry;

typedef struct {
    NodeEntry *entries;
    size_t capacity;
    size_t count;
} NodeMap;


static size_t hash_int64(int64_t x)
{
    uint64_t h = (uint64_t)x;

    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;

    return (size_t)h;
}


static NodeMap *node_map_create(size_t capacity)
{
    NodeMap *map = calloc(1, sizeof(*map));

    if (!map)
        return NULL;

    map->capacity = capacity;

    map->entries =
        calloc(
            capacity,
            sizeof(*map->entries)
        );

    if (!map->entries) {
        free(map);
        return NULL;
    }

    return map;
}


static int node_map_resize(NodeMap *map)
{
    size_t old_capacity = map->capacity;
    NodeEntry *old_entries = map->entries;

    map->capacity *= 2;

    map->entries =
        calloc(
            map->capacity,
            sizeof(*map->entries)
        );

    if (!map->entries) {
        map->entries = old_entries;
        map->capacity = old_capacity;
        return 0;
    }

    map->count = 0;

    for (size_t i = 0; i < old_capacity; i++) {

        if (!old_entries[i].used)
            continue;

        size_t index =
            hash_int64(old_entries[i].key)
            % map->capacity;

        while (map->entries[index].used)
            index = (index + 1) % map->capacity;

        map->entries[index] =
            old_entries[i];

        map->count++;
    }

    free(old_entries);

    return 1;
}


static void node_map_insert(
    NodeMap *map,
    int64_t id,
    double lat,
    double lon)
{
    /*
     * Resize when load factor reaches ~70%.
     */
    if (map->count * 10 >= map->capacity * 7) {

        if (!node_map_resize(map))
            return;
    }

    size_t index =
        hash_int64(id) %
        map->capacity;

    while (map->entries[index].used) {

        /*
         * Node already exists.
         */
        if (map->entries[index].key == id) {

            map->entries[index].value.lat = lat;
            map->entries[index].value.lon = lon;

            return;
        }

        index =
            (index + 1) %
            map->capacity;
    }

    map->entries[index].used = 1;

    map->entries[index].key = id;

    map->entries[index].value.id = id;
    map->entries[index].value.lat = lat;
    map->entries[index].value.lon = lon;

    map->count++;
}


static Node *node_map_get(
    NodeMap *map,
    int64_t id)
{
    size_t index =
        hash_int64(id) %
        map->capacity;

    size_t start = index;

    while (map->entries[index].used) {

        if (map->entries[index].key == id)
            return &map->entries[index].value;

        index =
            (index + 1) %
            map->capacity;

        if (index == start)
            break;
    }

    return NULL;
}


static void node_map_destroy(NodeMap *map)
{
    if (!map)
        return;

    free(map->entries);
    free(map);
}


/* ============================================================
 * GRAPH
 * ============================================================ */

typedef struct Edge {
    int64_t to;
    double weight;

    struct Edge *next;
} Edge;


typedef struct Vertex {
    int64_t id;

    Edge *edges;

    struct Vertex *next;
} Vertex;


typedef struct {
    Vertex **buckets;

    size_t capacity;
    size_t vertex_count;
    size_t edge_count;
} Graph;


static Graph *graph_create(size_t capacity)
{
    Graph *graph =
        calloc(1, sizeof(*graph));

    if (!graph)
        return NULL;

    graph->capacity = capacity;

    graph->buckets =
        calloc(
            capacity,
            sizeof(*graph->buckets)
        );

    if (!graph->buckets) {
        free(graph);
        return NULL;
    }

    return graph;
}


static Vertex *graph_get_vertex(
    Graph *graph,
    int64_t id)
{
    size_t index =
        hash_int64(id) %
        graph->capacity;

    Vertex *vertex =
        graph->buckets[index];

    while (vertex) {

        if (vertex->id == id)
            return vertex;

        vertex = vertex->next;
    }

    return NULL;
}


static Vertex *graph_get_or_create_vertex(
    Graph *graph,
    int64_t id)
{
    Vertex *vertex =
        graph_get_vertex(graph, id);

    if (vertex)
        return vertex;

    size_t index =
        hash_int64(id) %
        graph->capacity;

    vertex =
        calloc(1, sizeof(*vertex));

    if (!vertex)
        return NULL;

    vertex->id = id;

    vertex->next =
        graph->buckets[index];

    graph->buckets[index] =
        vertex;

    graph->vertex_count++;

    return vertex;
}


static void graph_add_edge(
    Graph *graph,
    int64_t from,
    int64_t to,
    double weight)
{
    Vertex *vertex =
        graph_get_or_create_vertex(
            graph,
            from
        );

    if (!vertex)
        return;

    Edge *edge =
        malloc(sizeof(*edge));

    if (!edge)
        return;

    edge->to = to;
    edge->weight = weight;

    edge->next =
        vertex->edges;

    vertex->edges =
        edge;

    graph->edge_count++;
}


static void graph_destroy(Graph *graph)
{
    if (!graph)
        return;

    for (size_t i = 0;
         i < graph->capacity;
         i++) {

        Vertex *vertex =
            graph->buckets[i];

        while (vertex) {

            Vertex *next_vertex =
                vertex->next;

            Edge *edge =
                vertex->edges;

            while (edge) {

                Edge *next_edge =
                    edge->next;

                free(edge);

                edge = next_edge;
            }

            free(vertex);

            vertex = next_vertex;
        }
    }

    free(graph->buckets);
    free(graph);
}


/* ============================================================
 * DISTANCE
 *
 * Haversine distance.
 * Returns meters.
 * ============================================================ */

static double distance_m(
    double lat1,
    double lon1,
    double lat2,
    double lon2)
{
    double phi1 =
        lat1 * M_PI / 180.0;

    double phi2 =
        lat2 * M_PI / 180.0;

    double dphi =
        (lat2 - lat1) *
        M_PI / 180.0;

    double dlambda =
        (lon2 - lon1) *
        M_PI / 180.0;

    double a =
        sin(dphi / 2.0) *
        sin(dphi / 2.0) +

        cos(phi1) *
        cos(phi2) *
        sin(dlambda / 2.0) *
        sin(dlambda / 2.0);

    double c =
        2.0 *
        atan2(
            sqrt(a),
            sqrt(1.0 - a)
        );

    return EARTH_RADIUS_M * c;
}


/* ============================================================
 * WALKABLE HIGHWAY
 * ============================================================ */

static int is_walkable_highway(
    const xmlChar *value)
{
    if (!value)
        return 0;

    return
        xmlStrEqual(
            value,
            BAD_CAST "footway"
        ) ||

        xmlStrEqual(
            value,
            BAD_CAST "pedestrian"
        ) ||

        xmlStrEqual(
            value,
            BAD_CAST "path"
        ) ||

        xmlStrEqual(
            value,
            BAD_CAST "steps"
        ) ||

        xmlStrEqual(
            value,
            BAD_CAST "living_street"
        ) ||

        xmlStrEqual(
            value,
            BAD_CAST "residential"
        ) ||

        xmlStrEqual(
            value,
            BAD_CAST "service"
        ) ||

        xmlStrEqual(
            value,
            BAD_CAST "track"
        ) ||

        xmlStrEqual(
            value,
            BAD_CAST "unclassified"
        );
}


/* ============================================================
 * NODE PARSER
 * ============================================================ */

static void parse_node(
    xmlTextReaderPtr reader,
    NodeMap *nodes)
{
    xmlChar *id =
        xmlTextReaderGetAttribute(
            reader,
            BAD_CAST "id"
        );

    xmlChar *lat =
        xmlTextReaderGetAttribute(
            reader,
            BAD_CAST "lat"
        );

    xmlChar *lon =
        xmlTextReaderGetAttribute(
            reader,
            BAD_CAST "lon"
        );

    if (!id || !lat || !lon) {

        xmlFree(id);
        xmlFree(lat);
        xmlFree(lon);

        return;
    }

    int64_t node_id =
        strtoll(
            (char *)id,
            NULL,
            10
        );

    double latitude =
        strtod(
            (char *)lat,
            NULL
        );

    double longitude =
        strtod(
            (char *)lon,
            NULL
        );

    node_map_insert(
        nodes,
        node_id,
        latitude,
        longitude
    );

    xmlFree(id);
    xmlFree(lat);
    xmlFree(lon);
}


/* ============================================================
 * WAY PARSER
 * ============================================================ */

static void parse_way(
    xmlTextReaderPtr reader,
    NodeMap *nodes,
    Graph *graph)
{
    int64_t *refs = NULL;

    size_t ref_count = 0;
    size_t ref_capacity = 16;

    int walkable = 0;

    refs =
        malloc(
            ref_capacity *
            sizeof(*refs)
        );

    if (!refs)
        return;

    while (xmlTextReaderRead(reader) == 1) {

        int type =
            xmlTextReaderNodeType(reader);

        /*
         * </way>
         */
        if (type ==
            XML_READER_TYPE_END_ELEMENT) {

            const xmlChar *name =
                xmlTextReaderConstName(
                    reader
                );

            if (xmlStrEqual(
                    name,
                    BAD_CAST "way"
                )) {

                break;
            }

            continue;
        }

        if (type !=
            XML_READER_TYPE_ELEMENT) {

            continue;
        }

        const xmlChar *name =
            xmlTextReaderConstName(
                reader
            );


        /*
         * <nd ref="123"/>
         */
        if (xmlStrEqual(
                name,
                BAD_CAST "nd"
            )) {

            xmlChar *ref =
                xmlTextReaderGetAttribute(
                    reader,
                    BAD_CAST "ref"
                );

            if (!ref)
                continue;


            if (ref_count ==
                ref_capacity) {

                ref_capacity *= 2;

                int64_t *tmp =
                    realloc(
                        refs,
                        ref_capacity *
                        sizeof(*refs)
                    );

                if (!tmp) {

                    xmlFree(ref);
                    free(refs);

                    return;
                }

                refs = tmp;
            }


            refs[ref_count++] =
                strtoll(
                    (char *)ref,
                    NULL,
                    10
                );

            xmlFree(ref);
        }


        /*
         * <tag k="highway" v="footway"/>
         */
        else if (xmlStrEqual(
                     name,
                     BAD_CAST "tag"
                 )) {

            xmlChar *key =
                xmlTextReaderGetAttribute(
                    reader,
                    BAD_CAST "k"
                );

            xmlChar *value =
                xmlTextReaderGetAttribute(
                    reader,
                    BAD_CAST "v"
                );

            if (key &&
                value &&
                xmlStrEqual(
                    key,
                    BAD_CAST "highway"
                )) {

                walkable =
                    is_walkable_highway(
                        value
                    );
            }

            xmlFree(key);
            xmlFree(value);
        }
    }


    /*
     * Not a pedestrian way.
     */
    if (!walkable) {

        free(refs);

        return;
    }


    /*
     * Build an UNDIRECTED graph.
     *
     * Way:
     *
     * A -> B -> C -> D
     *
     * becomes:
     *
     * A <-> B
     * B <-> C
     * C <-> D
     */
    for (size_t i = 0;
         i + 1 < ref_count;
         i++) {

        Node *a =
            node_map_get(
                nodes,
                refs[i]
            );

        Node *b =
            node_map_get(
                nodes,
                refs[i + 1]
            );

        /*
         * Referenced node wasn't found.
         */
        if (!a || !b)
            continue;


        double weight =
            distance_m(
                a->lat,
                a->lon,
                b->lat,
                b->lon
            );


        /*
         * A -> B
         */
        graph_add_edge(
            graph,
            a->id,
            b->id,
            weight
        );


        /*
         * B -> A
         */
        graph_add_edge(
            graph,
            b->id,
            a->id,
            weight
        );
    }

    free(refs);
}


/* ============================================================
 * MAIN
 * ============================================================ */

int main(int argc, char **argv)
{
    if (argc != 2) {

        fprintf(
            stderr,
            "Usage: %s walking.osm\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }


    /*
     * Streaming XML reader.
     *
     * The complete OSM XML file is never
     * loaded into memory.
     */
    xmlTextReaderPtr reader =
        xmlReaderForFile(
            argv[1],
            NULL,
            XML_PARSE_HUGE
        );

    if (!reader) {

        fprintf(
            stderr,
            "Failed to open: %s\n",
            argv[1]
        );

        return EXIT_FAILURE;
    }


    /*
     * Node ID -> coordinates.
     */
    NodeMap *nodes =
        node_map_create(
            INITIAL_CAPACITY
        );

    if (!nodes) {

        fprintf(
            stderr,
            "Failed to allocate node map\n"
        );

        xmlFreeTextReader(reader);

        return EXIT_FAILURE;
    }


    /*
     * Weighted undirected graph.
     */
    Graph *graph =
        graph_create(
            INITIAL_CAPACITY
        );

    if (!graph) {

        fprintf(
            stderr,
            "Failed to allocate graph\n"
        );

        node_map_destroy(nodes);
        xmlFreeTextReader(reader);

        return EXIT_FAILURE;
    }


    /*
     * Read the XML sequentially.
     */
    while (xmlTextReaderRead(reader) == 1) {

        if (xmlTextReaderNodeType(reader) !=
            XML_READER_TYPE_ELEMENT) {

            continue;
        }

        const xmlChar *name =
            xmlTextReaderConstName(
                reader
            );


        /*
         * Store coordinates.
         */
        if (xmlStrEqual(
                name,
                BAD_CAST "node"
            )) {

            parse_node(
                reader,
                nodes
            );
        }


        /*
         * Process ways.
         */
        else if (xmlStrEqual(
                     name,
                     BAD_CAST "way"
                 )) {

            parse_way(
                reader,
                nodes,
                graph
            );
        }
    }


    xmlFreeTextReader(reader);


    /*
     * Graph is now ready.
     */
    printf(
        "Graph constructed\n"
        "Vertices: %zu\n"
        "Edges:    %zu\n",
        graph->vertex_count,
        graph->edge_count
    );


    /*
     * TODO:
     *
     * Run Dijkstra / A* here.
     *
     * Example:
     *
     * shortest_path(
     *     graph,
     *     start_node_id,
     *     end_node_id
     * );
     */


    graph_destroy(graph);
    node_map_destroy(nodes);

    return EXIT_SUCCESS;
}
