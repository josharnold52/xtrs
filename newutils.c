//
// Created by Joshua Arnold on 1/7/23.
//

#include "newutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include "trs.h"


size_t find_next_line_offset(const char *data, size_t size, size_t offset) {
    if (offset >= size) {
        return size;
    }
    size_t p = offset;
    //Line terminators ar 0A or 0D or 0A0D or 0D0A
    for (;;) {
        char c = data[p];
        if ((++p) >= size) {
            break;
        }
        if (c == 0xa || c == 0xd) {
            char c2 = data[p];
            if (c != c2 && (c2 == 0xa || c2 == 0xd)) {
                p++;
            }
            break;
        }
    }
    return p;
}

//returns the new end if we trim whitespace from the right
size_t rtrim_offset(const char *data, size_t start, size_t end) {
    for (; end > start; end--) {
        char c = data[end - 1];
        if (c > 32) {
            break;
        }
    }
    return end;
}

//returns the new end if we trim whitespace from the left
size_t ltrim_offset(const char *data, size_t start, size_t end) {
    for (; end > start; start++) {
        char c = data[start];
        if (c > 32) {
            break;
        }
    }
    return start;
}



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

int find_cas_tracks2(const char *baseDir, const char *file, int *trackArray, int maxTracks) {
    if (!baseDir || !baseDir[0])
        return find_cas_tracks(file, trackArray, maxTracks);
    if (!file || !file[0])
        return find_cas_tracks(baseDir, trackArray, maxTracks);
    size_t dirLen = strlen(baseDir); //we know this is > 0
    size_t fileLen = strlen(file);   //we know this is > 0
    char *buf = malloc(dirLen + fileLen + 3);
    if (!buf)
        return 0;
    strcpy(buf, baseDir);
    if (buf[dirLen-1] != '/' && buf[dirLen-1] != '\\') {
        buf[dirLen] = '/';
        buf[dirLen+1] = 0;
    }
    strcat(buf, file);
    int res = find_cas_tracks(buf, trackArray, maxTracks);
    free(buf);
    return res;
}

int find_cas_tracks(const char *file, int *trackArray, int maxTracks) {
    struct mem_block *block = read_mem_block_from_file(file);
    if (!block) {
        return 0;
    }
    int trackCount = 0;
    int zeroRunSize = 0;
    for(size_t pos=0;pos<block->size;pos++) {
        if (block->data[pos] != 0) {
            zeroRunSize = 0;
        } else {
            zeroRunSize++;
            if (zeroRunSize == 120) {
                int trackPos = ((int)pos) - 119;
                if (trackCount < maxTracks && trackArray) {
                    trackArray[trackCount] = trackPos;
                }
                trackCount++;
            }
        }
    }

    free_mem_block(block);
    return trackCount;
}

size_t safe_strcpy(char *dest, const char *src, size_t buf_size_include_null) {
    if (!buf_size_include_null || !dest)
        return 0;
    size_t len = strlen(src);
    size_t toCopy = len < buf_size_include_null ? len : buf_size_include_null - 1;
    if (toCopy)
        memcpy(dest, src, toCopy);
    dest[toCopy] = 0;
    return toCopy;
}