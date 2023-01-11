//
// Created by Joshua Arnold on 1/7/23.
//
#include <string.h>
#include <assert.h>
#include "trs.h"
#include "newutils.h"
#include "trs_metafile.h"

static size_t min(size_t i, size_t i1);

static void log_fragment(const char *name, const char *data, size_t start, size_t end);

static size_t find_next_line_offset(const char *data, size_t size, size_t offset) {
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
static size_t rtrim_offset(const char *data, size_t start, size_t end) {
    for (; end > start; end--) {
        char c = data[end - 1];
        if (c > 32) {
            break;
        }
    }
    return end;
}

//returns the new end if we trim whitespace from the left
static size_t ltrim_offset(const char *data, size_t start, size_t end) {
    for (; end > start; start++) {
        char c = data[start];
        if (c > 32) {
            break;
        }
    }
    return start;
}


void meta_reader_init(const struct mem_block *source_block, struct meta_reader_cursor *cursor) {
    memset(cursor, 0, sizeof(*cursor));
    cursor->data_start = cursor->label_start = cursor->file_start = source_block->data;
    cursor->file_length = source_block->size;
    cursor->state = META_READER_CURSOR_STATE_INIT;
}


//Tests if the fragment is a label and if so, updates start and end to just the label (with no prefix)
static int trim_to_label(const char *data, size_t *start, size_t *end) {
    size_t x = ltrim_offset(data, *start, *end);
    if (x >= *end || data[x] != META_LABEL_PREFIX) {
        return 0;
    }
    *start = x + 1;
    *end = rtrim_offset(data, *start, *end);
    return 1;
}


//Tests if the fragment is a label - does not return position
static int is_label(const char *data, size_t start, size_t end) {
    return trim_to_label(data, &start, &end);
}

//Return non zero if good, zero if eof
int meta_reader_next(struct meta_reader_cursor *cursor) {

    if (!cursor ||
        (cursor->state != META_READER_CURSOR_STATE_INIT && cursor->state != META_READER_CURSOR_STATE_VALID)) {
        return 0;
    }
    joshlog("JHERE+\n");
    size_t p;
    p = cursor->data_length + cursor->data_offset;
    for (;;) {
        if (p >= cursor->file_length) {
            cursor->state = META_READER_CURSOR_STATE_EOF;
            cursor->file_start = cursor->data_start = cursor->label_start = 0;
            cursor->file_length = cursor->label_length = cursor->data_length = 0;
            cursor->label_offset = cursor->data_offset = 0;
            return 0;
        }
        size_t p2 = find_next_line_offset(cursor->file_start, cursor->file_length, p);
        assert(p2 > p);
        log_fragment("LN", cursor->file_start, p, p2);
        size_t s = p, e = p2; //Remember start and end
        p = p2; // Prepare for next iteration
        //If this was a label, trim it, set it in structure and exit.
        if (trim_to_label(cursor->file_start, &s, &e)) {
            cursor->label_offset = s;
            cursor->label_start = cursor->file_start + s;
            cursor->label_length = e - s;
            break;
        }
    }
    const size_t untrimmed_label_start = p;
    for (;;) {
        if (p >= cursor->file_length) {
            break;
        }
        size_t p2 = find_next_line_offset(cursor->file_start, cursor->file_length, p);
        if (is_label(cursor->file_start, p, p2)) {
            break;
        }
        p = p2;
    }
    const size_t untrimmed_label_end = p;

    cursor->data_offset = ltrim_offset(cursor->file_start, untrimmed_label_start, untrimmed_label_end);
    cursor->data_length = rtrim_offset(cursor->file_start, cursor->data_offset, untrimmed_label_end)
                          - cursor->data_offset;
    cursor->data_start = cursor->file_start + cursor->data_offset;
    cursor->state = META_READER_CURSOR_STATE_VALID;
    return 1;
}

static size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}


static void log_fragment(const char *name, const char *data, size_t start, size_t end) {
    char cpy[1024];
    size_t len = min(end - start, sizeof(cpy) - 1);
    memcpy(cpy, data + start, len);
    cpy[len] = 0;
    joshlog("META FRAGMENT %s : <%s>\n", name, cpy);
}


boundary find_meta_data(const struct mem_block *source_block, const char *label) {
    struct meta_reader_cursor cursor;
    boundary res;

    meta_reader_init(source_block, &cursor);
    while (meta_reader_next(&cursor)) {
        //TODO
        log_fragment("NEXT LABEL", cursor.file_start, cursor.label_offset,
                     cursor.label_offset + cursor.label_length);
        //TODO
        log_fragment("NEXT DATA", cursor.file_start, cursor.data_offset,
                     cursor.data_offset + cursor.data_length);

        if (strnicmp(label, cursor.label_start, cursor.label_length) == 0) {
            res.start = cursor.data_offset;
            res.end = cursor.data_offset + cursor.data_length;
            return res;
        }
    }
    res.start = res.end = 0;
    return res;
}
