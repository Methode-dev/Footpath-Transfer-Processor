/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#include <stdio.h>
#include <stdlib.h>
#include <libxml/xmlreader.h>

typedef struct {
    long long id;
    double lat;
    double lon;
} Node;

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
    return count;
}

static void parse_node(xmlTextReaderPtr reader, Node **nodes, unsigned int *count)
{
    xmlChar *id = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
    xmlChar *lat = xmlTextReaderGetAttribute(reader, BAD_CAST "lat");
    xmlChar *lon = xmlTextReaderGetAttribute(reader, BAD_CAST "lon");

    if (id && lat && lon)
        printf("id: %s\nlat: %s | lon: %s\n\n\n", id, lat, lon);
    nodes[*count]->id = strtoll((char *)id, NULL, 10);
    nodes[*count]->lat = strtod((char *)lat, NULL);
    nodes[*count]->lon = strtod((char *)lon, NULL);
    (*count)++;
    xmlFree(id);
    xmlFree(lat);
    xmlFree(lon);
}

int main(int argc, char **av)
{
    xmlTextReaderPtr reader = get_reader(av[1]);
    Node *nodes = NULL;
    unsigned int node_count = 0;
    unsigned int node_capacity = 0;

    while (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) != XML_READER_TYPE_ELEMENT)
            continue;
        const xmlChar *name = xmlTextReaderConstName(reader);

        if (xmlStrEqual(name, BAD_CAST "node"))
            parse_node(reader);
    }
    xmlFreeTextReader(reader);
    return 0;
}
