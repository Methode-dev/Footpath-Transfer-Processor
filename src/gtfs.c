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
    int j = 0, k = 0, i = 0;
    int size_subarray;
    char **res = malloc(sizeof(char *) * count);

    for (i = 0; i != count; i++) {
        size_subarray = count_til_sep(str, j, sep);
        res[i] = malloc(sizeof(char) * (size_subarray - j + 1));
        for (k = 0; j != size_subarray; j++, k++) {
            if (str[j] != '"' && str[j] != '\t' && str[j] != '\r')
                res[i][k] = str[j];
            else
                k--;
        }
        res[i][k] = '\0';
        j++;
    }
    res[i] = NULL;
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

static StopsHeader *parse_headers(char **headers)
{
    StopsHeader *headers_pos = malloc(sizeof(StopsHeader));

    headers_pos->stop_id = search_header(headers, "stop_id");
    headers_pos->stop_lat = search_header(headers, "stop_lat");
    headers_pos->stop_lon = search_header(headers, "stop_lon");
    for (int i = 0; headers[i]; i++)
        free(headers[i]);
    free(headers);
    return headers_pos;
}

static char *get_line(FILE *file)
{
    char *line = NULL;
    size_t size = 0;

    if (getline(&line, &size, file) == -1) {
        free(line);
        return NULL;
    }
    return line;
}

/* We're getting an estimate of the length of the first 1000 lines, then extrapolating from file size. */
static unsigned int estimate_stops(const char *filename)
{
    FILE *file = fopen(filename, "r");
    unsigned long sample_size = 0;
    unsigned int sample_count = 0;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    char *line = get_line(file);
    free(line);

    while (sample_count < 1000 && (line = get_line(file)) != NULL) {
        sample_size += strlen(line);
        sample_count++;
        free(line);
    }
    fclose(file);

    if (sample_count == 0)
        return 0;
    return file_size / (sample_size / sample_count);
}

void stops_free(BusStop *stops, unsigned int stop_count)
{
    for (unsigned int i = 0; i < stop_count; i++)
        free(stops[i].id);
    free(stops);
}

BusStop *parse_stops(const char *filename, unsigned int *stop_count)
{
    StopsHeader *headers_pos = parse_headers(get_headers(filename));
    FILE *file = fopen(filename, "r");
    char *line = get_line(file);
    unsigned int estimate = estimate_stops(filename);
    BusStop *stops = malloc(sizeof(BusStop) * estimate);
    char **fields;

    free(line);
    for (*stop_count = 0; (line = get_line(file)) != NULL; (*stop_count)++) {
        fields = str_to_tab(line, ',');
        if (*stop_count == estimate) {
            estimate += estimate / 2; // COULD crash but should be good enough
            stops = realloc(stops, sizeof(BusStop) * estimate);
        }
        stops[*stop_count].id = fields[headers_pos->stop_id];
        stops[*stop_count].lat = atof(fields[headers_pos->stop_lat]);
        stops[*stop_count].lon = atof(fields[headers_pos->stop_lon]);
        free(line);
    }
    fclose(file);
    return stops;
}
