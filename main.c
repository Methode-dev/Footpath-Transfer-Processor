/*
 * '--osm' -> '{path}'
 * '--gtfs' -> '{path}'
 * ['--output'] -> '{path}'
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int MIN_ARGS = 2;
int MAX_ARGS = 3;

char HELP_FLAG[7] = "--help\0";

char OSM_FLAG[6]  = "--osm\0";
char GTFS_FLAG[7] = "--gtfs\0";
char OUT_FLAG[10] = "--output\0";

typedef struct {
    char *output;
    char *osm;
    char *gtfs;
} paths;

int args_parser(int ac, char **av, paths *p)
{
    int flags_count[3] = {0, 0, 0}; // OSM, GTFS, OUTPUT
    char *flags[10] = {OSM_FLAG, GTFS_FLAG, OUT_FLAG, NULL};

    if (!(ac % 2) || ac < MIN_ARGS * 2 + 1 || ac > MAX_ARGS * 2 + 1) {
      printf("%s\n", "Wrong arguments. Please look at --help");
      return 1;
    }
    for (int i = 0; i < ; i += 2) {
        printf("%s\n", flags[i-1]);
    }
    return 0;
}

int main(int ac, char **av)
{
    paths *p = malloc(sizeof(paths));

    if (ac == 2 && strcmp(av[1], HELP_FLAG) == 0) {
      printf("HELP\n");
      return 0;
    }
    return args_parser(ac, av, p);
}
