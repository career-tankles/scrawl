#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "datrie.h"

#define LINE_LEN_MAX    1023

#define USAGE   "Usage: \n"              \
    "  Build: -b <txt_file> <dat_file>\n"        \
    "  Test:  -t <txt_file> <dat_file>\n"        \
    "  Query: -q <key> <dat_file>\n"             

int main(int argc, char **argv)
{
    if (argc != 4 && argc != 5)
    {
        fprintf(stderr, USAGE);
        return 1;
    }

    if (!strcmp(argv[1], "-b"))
    {
        const char *txt_file = argv[2];
        const char *dat_file = argv[3];

        FILE *src = fopen(txt_file, "r");
        if (!src)
        {
            fprintf(stderr, "Error: open %s, %m\n", txt_file);
            return 2;
        }

        dat_t *p = dat_new();

        char line[LINE_LEN_MAX + 1];
        char key[DAT_KSIZE_MAX + 1];
        unsigned value, i = 0;

        while (fgets(line, LINE_LEN_MAX, src))
        {
            i ++;
            if (sscanf(line, "%[^\t]\t%u\n", key, &value) != 2)
            {
                fprintf(stderr, "Error: [%u] %s\n", i, line);
                return 4;
            }
//          printf("%s -> %u\n", key, value);
            dat_add(p, key, value);
        }

        int ret = dat_dump(p, dat_file);
        if (ret < 0)
        {
            fprintf(stderr, "Error: build internal, ret=%d\n", ret);
            return 5;
        }

        printf("%u words done\n", i);
        fclose(src);
    }
    else if (!strcmp(argv[1], "-q"))
    {
        const char *key = argv[2];
        const char *dat_file = argv[3];

        dat_t *p = dat_new();

        int ret = dat_load(p, dat_file);
        if (ret < 0)
        {
            fprintf(stderr, "Error: build internal, ret=%d\n", ret);
            return 6;
        }

        unsigned value;
        ret = dat_get(p, key, &value);
        if (ret < 0)
        {
            printf("%s not exist\n", key);
            return 0;
        }

        printf("%s -> %u\n", key, value);

        dat_free(p);
    }
    else if (!strcmp(argv[1], "-t"))
    {
        const char *txt_file = argv[2];
        const char *dat_file = argv[3];

        FILE *src = fopen(txt_file, "r");
        if (!src)
        {
            fprintf(stderr, "Error: open %s, %m\n", txt_file);
            return 7;
        }

        dat_t *p = dat_new();

        int ret = dat_load(p, dat_file);
        if (ret < 0)
        {
            fprintf(stderr, "Error: build internal, ret=%d\n", ret);
            return 8;
        }
        char line[LINE_LEN_MAX + 1];
        char key[DAT_KSIZE_MAX + 1];
        char k[DAT_KSIZE_MAX + 1] = {0};
        unsigned value, v, i = 0;

        while (fgets(line, LINE_LEN_MAX, src))
        {
            i ++;

            if (!strcmp(key, k))
                continue;

            if (sscanf(line, "%[^\t]\t%u\n", key, &value) != 2)
            {
                fprintf(stderr, "Error: [%u] %s\n", i, line);
                return 9;
            }

            ret = dat_get(p, key, &v);
            assert(ret == 0 && v == value);

            strcpy(k, key);

//          printf("%s -> %u\n", key, value);
        }

        printf("%u words done\n", i);
        fclose(src);

        dat_free(p);
    }
    return 0;
}
