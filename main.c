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

int search_flag(int ac, char **av, char *flag)
{
    for (int i = 1; i < ac; i += 1) {
        if (strcmp(flag, av[i]) == 0)
            return i + 1;
    }
    return -1;
}

int args_parser(int ac, char **av, paths *p)
{
    int index;
    int count = 0;
    int flags_index[3] = {0, 0, 0}; // OSM, GTFS, OUTPUT
    char *flags[10] = {OSM_FLAG, GTFS_FLAG, OUT_FLAG, NULL}; // MANDATORY FLAGS MUST BE PLACED FIRST

    if (!(ac % 2) || ac < MIN_ARGS * 2 + 1 || ac > MAX_ARGS * 2 + 1) {
      printf("%s\n", "Wrong arguments. Please look at --help");
      return 1;
    }
    for (int i = 0; flags[i] != NULL; i++) {
        index = search_flag(ac, av, flags[i]);
        count += 2;
        flags_index[i] = index - 1;
        if (i > 0 && flags_index[i] - 1 == flags_index[i - 1]) {
            printf("Flag %s has no value. See --help for more info\n", flags[i - 1]);
            return -1;
        }
        if (index == -1 && i < MIN_ARGS) {
            printf("%s flag is missing. See --help for more info\n", flags[i]);
            return -1;
        }
    }
    return 0;
}

void print_helper(void)
{
    printf("USAGE: footpaths_processor [command] [arg]\n\n");
    printf("COMMANDS:\n");
    printf("\t--osm\n\t\tFeeds the path of the osm file.\n\n");
    printf("\t--gtfs\n\t\tFeeds the path of the GTFS stops file.\n\t\tMust be the 'stops.txt' file or equivalent, the folder is not accepted.\n\n");
    printf("\t[--output]\n\t\tFeeds the path of the file that will be created.\n");
}

int main(int ac, char **av)
{
    paths *p = malloc(sizeof(paths));

    if (ac == 2 && strcmp(av[1], HELP_FLAG) == 0) {
        print_helper();
        return 0;
    }
    return args_parser(ac, av, p);
}
