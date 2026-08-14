/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <libxml/parser.h>


#define MIN_ARGS 2
#define MAX_ARGS 3

#define OK 0
#define KO 1

#define HELP_FLAG "--help\0"

#define OSM_FLAG "--osm\0"
#define GTFS_FLAG "--gtfs\0"
#define OUT_FLAG "--output\0"

#define FLAGS = {OSM_FLAG, GTFS_FLAG, OUT_FLAG, NULL}; // MANDATORY FLAGS MUST BE PLACED FIRST

typedef struct {
    char **paths;
} paths;

int search_flag(int ac, char **av, char *flag)
{
    for (int i = 1; i < ac; i += 1) {
        if (strcmp(flag, av[i]) == 0)
            return i + 1;
    }
    return 1;
}

int args_parser(int ac, char **av, paths *p)
{
    int index;
    int flags_index[MAX_ARGS]; // OSM, GTFS, OUTPUT

    for (int i = 0; i != MAX_ARGS; flags_index[i] = 0, i++);
    if (!(ac % 2) || ac < MIN_ARGS * 2 + 1 || ac > MAX_ARGS * 2 + 1) {
      printf("%s\n", "Wrong arguments. Please look at --help");
      return 1;
    }
    p->paths = malloc(sizeof(char *) * MAX_ARGS + 1);

    for (int i = 0, j = 0; FLAGS[i] != NULL; i++) {
        index = search_flag(ac, av, FLAGS[i]);
        flags_index[i] = index - 1;
        if (i > 0 && flags_index[i] - 1 == flags_index[i - 1]) {
            printf("Flag %s has no value. See --help for more info\n", FLAGS[i - 1]);
            return 1;
        } else if (index == 1 && i < MIN_ARGS) {
            printf("%s flag is missing. See --help for more info\n", FLAGS[i]);
            return 1;
        } else if (open(av[index], O_WRONLY | O_CREAT | O_TRUNC, 0644) < 0) {
            printf("Path for %s flag {%s} cannot be opened nor created.\nRelated error:\n", av[index-1], av[index]);
            perror(av[index]);
            return 1;
        } else if (index != 1) {
            p->paths[j] = malloc(sizeof(char) * strlen(av[index]) + 1);
            strcpy(p->paths[j], av[index]);
            j++;
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
    if (args_parser(ac, av, p) == 1)
        return 1;
    else
     printf("%s\n", p->paths[0]);
    return 0;
}
