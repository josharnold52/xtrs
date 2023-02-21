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

size_t safe_strlen(const char *s, size_t buf_size_include_null) {
    if (!s) {
        return 0;
    }
    size_t sz;
    for(sz =0;sz < buf_size_include_null && *s;s++,sz++) {
        /** loop */
    }
    return sz;

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

/** Appends tp the destination string, truncating if need be, ensures always null terminated.
 * Returns length of copied string (not including null term)
 * @param dest the string to append to
 * @param src the string to append
 * @param buf_size_include_null the _total_ size of the destination buffer (not just the space remainign)
 * @param returns the resulting string length
 * */
size_t safe_strcat(char *dest, const char *src, size_t buf_size_include_null) {
    if (!dest)
        return 0;
    size_t dlen = safe_strlen(dest, buf_size_include_null);
    if (!src || (dlen >= buf_size_include_null))
        return dlen;
    size_t room_left_inc_null = buf_size_include_null - dlen;
    if (room_left_inc_null == 1)
        return dlen;
    size_t x = safe_strcpy(dest+dlen, src, room_left_inc_null);
    return dlen + x;
}



size_t join_path(char *buf, size_t buf_size_include_null, const char *p1, const char *p2) {
    if (!buf_size_include_null)
        return 0;
    if (!p1 || !p1[0])
        return safe_strcpy(buf, p2 ? p2 : "", buf_size_include_null);
    size_t x = safe_strcpy(buf, p1, buf_size_include_null);
    if (!p2 || !p2[0])
        return x;

    if (buf[x-1] != '/' && buf[x-1] != '\\') {
        if ((x+1)>=buf_size_include_null)
            return x;
        buf[x++] = '/';
        buf[x] = 0;
    }
    while (p2[0] == '/')
        p2++;
    size_t x2 = safe_strcpy(buf+x,  p2, buf_size_include_null - x);
    return x + x2;
}

unsigned int get_file_length(const char *fn) {
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
        return 0xFFFFFFFF;
    }
    return (unsigned int) len;
}

size_t find_char(const char *data, size_t start, size_t end, char c) {
    if (end <= start)
        return end;
    for(size_t x = start; x < end; x ++) {
        if (data[x] == c)
            return x;
    }
    return end;
}

size_t find_char_from_right(const char *data, size_t start, size_t end, char c) {
    if (end <= start)
        return start;
    for(size_t x = end-1; x >= start; x --) {
        if (data[x] == c)
            return x;
    }
    return start;
}

/** Returns end if not found */
size_t find_dirsep(const char *data, size_t start, size_t end) {
    if (end <= start)
        return end;
    for(size_t x = start; x < end; x ++) {
        if (data[x] == '/' || data[x] == '\\')
            return x;
    }
    return end;
}

/** Returns start if not found */
size_t find_dirsep_from_right(const char *data, size_t start, size_t end) {
    if (end <= start)
        return start;
    for(size_t x = end-1; x >= start; x --) {
        if (data[x] == '/' || data[x] == '\\')
            return x;
    }
    return start;
}


/** Return position just after delim or end if not found */
size_t extract_next_token(const char *data, size_t start, size_t end, char delim, char *dest, size_t dest_buf_size) {
    if (dest && dest_buf_size)
        dest[0] = 0;
    if(start >= end)
        return end;
    size_t delim_pos = find_char(data, start, end, delim);
    if (dest && dest_buf_size) {
        size_t s = ltrim_offset(data, start, delim_pos);
        size_t e = rtrim_offset(data, s, delim_pos);
        size_t len = e - s;
        if (len >= dest_buf_size) {
            len = dest_buf_size - 1;
        }
        if (len)
            memcpy(dest, data + s, len);
        dest[len] = 0;
    }
    return delim_pos < end ? delim_pos + 1 : end;
}

int starts_with(const char *str, const char *prefix_to_test) {
    if (!str || !prefix_to_test)
        return 0;
    const char *s, *p;
    for(s=str,p=prefix_to_test;*p;s++,p++) {
        if (*p != *s) {
            return 0;
        }
    }
    return 1;
}

int is_dirsep(char c) {
    return c == '/' || c == '\\';
}