//
// Created by Joshua Arnold on 1/7/23.
//

#include "newutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "trs.h"


static unsigned int get_file_length(const char *fn) {
    FILE *f;
    int fhandle;
    long long len;

    f = fopen(fn, "rb");
    if (!f) {
        return 0;
    }
    fhandle = fileno(f);
    len = lfilelength(fhandle);
    fclose(f);
    if (len == -1LL) {
        return 0;
    }
    if (len > 0xFFFFFFFFLL) {
        return 0xFFFFFFFFU;
    }
    return (unsigned int) len;
}


//Does nothing if null
void free_mem_block(struct mem_block *p) {
    if (p) free(p);
}

struct mem_block *read_mem_block_from_file(const char *filename) {
    if (!filename) {
        return 0;
    }
    unsigned int len = get_file_length(filename);
    if (len > 0x6FFFFFFF) {
        joshlog("Cannot find length of %s\n", filename);
        return 0;
    }
    struct mem_block *res = malloc(len + sizeof(struct mem_block));
    if (!res) {
        joshlog("..metafile_mem_block allocation failed\n");
        return 0;
    }
    res->size = len;
    FILE *f = fopen(filename, "rb");
    if (!f) {
        joshlog("fopen failed\n");
        free(res);
        return 0;
    }
    size_t read_count = fread(res->data, 1, res->size, f);
    if (read_count != res->size) {
        joshlog("Read count mismatch expect %u got %u\n", res->size, read_count);
        free(res);
        fclose(f);
        return 0;
    }
    fclose(f);
    return res;
}

