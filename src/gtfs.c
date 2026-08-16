/*
 * MÉTHODE SAS
 * https://methode.dev
 */

#include "gtfs.h"
#include <sys/fcntl.h>

static int count_til_sep(char *str, int i, char sep)
{
    for (; str[i] != '\0' && str[i] != '\n' && str[i] != sep; i++);
    return i;
}

static int count_sep(char *str, char sep)
{
    int count = 0;

    for (int i = 0; str[i] != '\0' && str[i] != '\n'; i++)
        if (str[i] == sep)
            count++;
    return count + 1;
}

static char **str_to_tab(char *str, char sep)
{
    int count = count_sep(str, sep);
    int j = 0;
    char **res = malloc(sizeof(char *) * count);

    for (int i = 0; i != count; i++) {
        int size_subarray = count_til_sep(str, j, sep);
        res[i] = malloc(sizeof(char) * size_subarray + 1);
        for (int k = 0; j != size_subarray; j++, k++) {
            if (str[j] != '"' && str[j] != '\t' && str[j] != '\r')
                res[i][k] = str[j];
            else
                k--;
        }
        res[i][j] = '\0';
        j++;
    }
    return res;
}

static char **get_headers(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    char tmp;
    int i;
    char *header = malloc(sizeof(char));

    for (i = 1; read(fd, &tmp, 1); i++) {
        if (tmp == '\n')
            break;
        header = realloc(header, sizeof(char) * i);
        header[i - 1] = tmp;
    }
    header[i - 1] = '\0';
    return str_to_tab(header, ',');
}

static int search_header(char **headers, char *header)
{
    for (int i = 0; headers[i]; i++) {
        if (!(strcmp(headers[i], header)))
            return i;
    }
    return -1;
}

StopsHeader *parse_stops(const char *filename)
{
    char **headers = get_headers(filename);
    StopsHeader *headers_pos = malloc(sizeof(StopsHeader));

    headers_pos->stop_id = search_header(headers, "stop_id");
    headers_pos->stop_lat = search_header(headers, "stop_lat");
    headers_pos->stop_lon = search_header(headers, "stop_lon");
    return headers_pos;
}
