/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <libxml/parser.h>
#include "osm.h"

#define MIN_ARGS 2
#define MAX_ARGS 3

#define OK 0
#define KO 1

#define HELP_FLAG "--help\0"

#define OSM_FLAG "--osm\0"
#define GTFS_FLAG "--gtfs\0"
#define OUT_FLAG "--output\0"

const char* FLAGS[] = {OSM_FLAG, GTFS_FLAG, OUT_FLAG, NULL}; // MANDATORY FLAGS MUST BE PLACED FIRST

typedef struct {
    char **paths;
} paths;

#endif
