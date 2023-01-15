//
// Created by Joshua Arnold on 1/7/23.
//

#ifndef XTRS_NEWUTILS_H
#define XTRS_NEWUTILS_H

#include <stddef.h>

struct mem_block {
    size_t size;
    char data[];
};

void free_mem_block(struct mem_block *p);
struct mem_block *read_mem_block_from_file(const char *filename);

//Return size when at end
size_t find_next_line_offset(const char *data, size_t size, size_t offset);

//returns the new end if we trim whitespace from the right
size_t rtrim_offset(const char *data, size_t start, size_t end);

//returns the new end if we trim whitespace from the left
size_t ltrim_offset(const char *data, size_t start, size_t end);


int find_cas_tracks(const char *file, int *trackArray, int maxTracks);
int find_cas_tracks2(const char *baseDir, const char *file, int *trackArray, int maxTracks);

/** Copies the string, truncating if need be, ensures always null terminated. Returns length of copied string (not including null term) */
size_t safe_strcpy(char *dest, const char *src, size_t buf_size_include_null);

#endif //XTRS_NEWUTILS_H
