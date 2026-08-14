/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

static void parse_node(xmlTextReaderPtr reader, Node *nodes, unsigned int *count)
{
    xmlChar *id = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
    xmlChar *lat = xmlTextReaderGetAttribute(reader, BAD_CAST "lat");
    xmlChar *lon = xmlTextReaderGetAttribute(reader, BAD_CAST "lon");

    // if (id && lat && lon)
    //     printf("id: %s\nlat: %s | lon: %s\n\n\n", id, lat, lon);
    nodes[*count].id = strtoll((char *)id, NULL, 10);
    nodes[*count].lat = strtod((char *)lat, NULL);
    nodes[*count].lon = strtod((char *)lon, NULL);
    (*count)++;
    xmlFree(id);
    xmlFree(lat);
    xmlFree(lon);
}

int main(int argc, char **av)
{
    const unsigned int node_count = count_nodes(av[1]);
    xmlTextReaderPtr reader = get_reader(av[1]);
    Node *nodes = malloc(sizeof(Node) * (node_count + 1));
    unsigned int parsed_nodes = 0;

    while (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) != XML_READER_TYPE_ELEMENT)
            continue;
        const xmlChar *name = xmlTextReaderConstName(reader);

        if (xmlStrEqual(name, BAD_CAST "node"))
            parse_node(reader, nodes, &parsed_nodes);
    }
    xmlFreeTextReader(reader);
    free(nodes);
    return 0;
}
