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


#endif //XTRS_NEWUTILS_H
