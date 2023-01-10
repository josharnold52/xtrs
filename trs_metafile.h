//
// Created by Joshua Arnold on 1/7/23.
//

#include "newutils.h"

/***
 * METAFILE FORMAT:
 * ~<header>
 * <data line>
 * <data line>
 * <data line>
 * ~header
 * ...
 *
 */

#ifndef XTRS_TRS_METAFILE_H
#define XTRS_TRS_METAFILE_H

#define META_READER_CURSOR_STATE_INIT (-1)
#define META_READER_CURSOR_STATE_VALID (1)
#define META_READER_CURSOR_STATE_EOF (0)

#define META_LABEL_PREFIX ('~')

struct meta_reader_cursor {
    /**
     * If META_READER_CURSOR_STATE_INIT, pointers point to the start of block, offsets and lengths are 0
     * If META_READER_CURSOR_STATE_VALID, everything is what you expect
     * If META_READER_CURSOR_STATE_EOF, pointers are null, offsets and lengths are 0
     *
     * Note that META_READER_CURSOR_STATE_EOF is 0 and others are non-zero (for convenient looping)
     */
    int state;

    unsigned int label_offset;
    unsigned int label_length;
    const char *label_start;

    unsigned int data_offset;
    unsigned int data_length;
    const char *data_start;

    const char *file_start;
    size_t file_length;
};

void meta_reader_init(const struct mem_block *source_block, struct meta_reader_cursor *cursor);
//Return non zero if good, zero if eof
int meta_reader_next(struct meta_reader_cursor *cursor);

struct boundary {
    size_t start;
    size_t end;
};
typedef struct boundary boundary;

/**
 * Finds the data for the given label in the given metafile.  If not found, returns
 * a boundary with start and end == 0 (either check for this directly, or it can be
 * considered an empty string)
 */
boundary find_meta_data(const struct mem_block *source_block, const char *label);

#endif //XTRS_TRS_METAFILE_H
