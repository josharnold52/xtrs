//
// Created by arnold on 1/21/25.
//



#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstddef>
#include "dpmutil_ini.h"
#include "../dpmhw/dpmhw.h" //TODO - Maybe this stuff should just be in a "dpm" space

using namespace dpmutil;

typedef IniSettings::_bufDesc bufDesc;

/** Advance to a non-whitespace (inclusive) and return its position or -1 if not found */
static int findLineStart(const bufDesc & buffer, int searchFrom) {
    searchFrom = searchFrom < 0 ? 0 : searchFrom;
    for(int p = searchFrom; p < buffer.size; p++) {
        if (buffer.pBuf[p] > 32) {
            return p;
        }
    }
    return -1;
}

/** Advance to a line terminator (inclusive) and return its position or -1 if not found */
static int findLineEnd(const bufDesc & buffer, int searchFrom) {
    searchFrom = searchFrom < 0 ? 0 : searchFrom;
    for(int p = searchFrom; p < buffer.size; p++) {
        if (buffer.pBuf[p] == '\r' || buffer.pBuf[p] == '\n') {
            return p;
        }
    }
    return -1;
}

/**
 * On entry, pos should point to the start of the section text (the '[').  If it matches,
 * the return is the position just after the trailing ']'.  Otherwise, returns -1;
 */
static int matchSectionHeader(const bufDesc & buffer, int pos, const char *sectionName) {
    if (pos < 0 || pos >= buffer.size) {
        return -1;
    }
    if (buffer.pBuf[pos++] != '[') {
        return -1;
    }
    for(char c = *sectionName; c ; c = *(++sectionName)) {
        if (pos >= buffer.size || c != buffer.pBuf[pos++]) {
            return -1;
        }
    }
    if (pos >= buffer.size || ']' != buffer.pBuf[pos++]) {
        return -1;
    }
    return pos;
}
/**
 * On entry, pos should point to the start of the ket.  If it matches,
 * the return is the position of the value (after the = and whitespace). The return
 * value could be the length of the buffer
 * Otherwise, returns -1;
 */
static int matchKey(const bufDesc & buffer, int pos, const char *keyName) {
    if (pos < 0 || pos > buffer.size) {
        return -1;
    }
    for(char c = *keyName; c ; c = *(++keyName)) {
        if (pos >= buffer.size || c != buffer.pBuf[pos++]) {
            return -1;
        }
    }
    while(pos < buffer.size && (buffer.pBuf[pos] == ' ' || buffer.pBuf[pos] == '\t')) {
        pos++;
    }
    if(pos >= buffer.size || buffer.pBuf[pos++] != '=') {
        return -1;
    }
    while(pos < buffer.size && (buffer.pBuf[pos] == ' ' || buffer.pBuf[pos] == '\t')) {
        pos++;
    }

    return pos;
}

/**
 * Returns the # of characters from pos up to and including the last non-whitespace character on the line.
 */
static int getRestOfLineLength(const bufDesc & buffer, const int pos) {
    // Note: The IDE emits a warning that pos can never be less than zero so isn't needed in the condition.
    //  I suppose this is because it is a static method and the IDE can reason about all callers.  However,
    //  I am leaving the condition in because I don't want to accidentally add a caller where this is
    //  violated and have it break.  (And disabling the warning for one line requires several lines of pragmas
    //  which I find too annoying)
    if (pos < 0 || pos >= buffer.size) {
        return 0;
    }
    //We know here that pos is a readable position
    int x = pos;
    for(; x < buffer.size; x++) {
        char c = buffer.pBuf[x];
        if (c == '\r' || c=='\n') {
            break;
        }
    }
    //Back up to just after the last non-space or tab.
    while(x > pos) {
        //This is a valid read since x-1 is less than length and greater than or equal to pos
        char c = buffer.pBuf[x - 1];
        if (c != ' ' && c != '\t') {
            break;
        }
        x --;
    }
    return x - pos;
}


static bool findValue(const bufDesc & buffer, const char *section, const char *key, int &startPos, int & length) {
    if (!buffer.pBuf) {
        return false;
    }
    int pos = 0;
    bool in_section = false;
    while(pos < buffer.size) {
        dpmhw::dpmhw_debug("start loop %d\n", pos);
        if ((pos = findLineStart(buffer, pos)) < 0) {
            return false;
        }
        dpmhw::dpmhw_debug("line start %d\n", pos);
        if (buffer.pBuf[pos] == '[') {
            dpmhw::dpmhw_debug("start section %d\n", pos);
            int chk = matchSectionHeader(buffer, pos, section);
            dpmhw::dpmhw_debug("  -- section res = %d\n", chk);
            in_section = chk >= 0;
        } else if (in_section) {
            dpmhw::dpmhw_debug("start key %d\n", pos);
            int chk = matchKey(buffer, pos, key);
            dpmhw::dpmhw_debug("  -- key res = %d\n", chk);
            if (chk >= 0) {
                dpmhw::dpmhw_debug("match at %d\n", chk);
                startPos = chk;
                length = getRestOfLineLength(buffer, chk);
                dpmhw::dpmhw_debug("length is %d\n", length);
                return true;
            }
        }
        if ((pos = findLineEnd(buffer, pos)) < 0) {
            return false;
        }
        dpmhw::dpmhw_debug("line end %d\n", pos);
    }
    return false;
}


int IniSettings::getStringLength(const char *section, const char *key) const {
    if (!section || !key) {
        return -1;
    }
    int startPos, length;
    if (!findValue(buffer, section, key, startPos, length)) {
        return -1;
    }
    return length;
}

bool IniSettings::getString(const char *section, const char *key, char *valueBuf, size_t valueBufSize, int &valueLength) const {
    if (!section || !key || !valueBuf || valueBufSize == 0) {
        return -1;
    }
    int startPos, length;
    if (!findValue(buffer, section, key, startPos, length)) {
        return false;
    }
    dpmhw::dpmhw_debug("fetched line length=%d vbsz=%u\n", length, valueBufSize);
    //Need length + room for a null
    if (valueBufSize <= length) {
        return false;
    }
    dpmhw::dpmhw_debug("fetched line 2 length=%d\n", length);
    memcpy(valueBuf, buffer.pBuf + startPos, length);
    valueBuf[length] = 0;
    valueLength = length;
    return true;
}


bool IniSettings::getBoolean(const char *section, const char *key, bool &value) const {
    char buf[8];
    if (!getString(section, key, buf, sizeof(buf))) {
        return false;
    }
    //Note that stricmp isn't defined in ANSI but djgpp has it.
    if (stricmp(buf, "true") == 0) {
        value = true;
        return true;
    }
    if (stricmp(buf, "false") == 0) {
        value = false;
        return true;
    }
    return false;
}
bool IniSettings::getInt(const char *section, const char *key, int &value) const {
    char buf[128];
    int valueLength = 0;
    if (!getString(section, key, buf, sizeof(buf), valueLength)) {
        return false;
    }
    if (valueLength == 0) {
        return false;
    }
    char *endp;
    long v = strtol(buf, &endp, 0);
    if (endp != (buf + valueLength)) {
        return false;
    }
    static_assert(sizeof(long) == sizeof(int));
    value = v;
    return true;
}

bool IniSettings::getDouble(const char *section, const char *key, double &value) const {
    char buf[128];
    int valueLength = 0;
    if (!getString(section, key, buf, sizeof(buf), valueLength)) {
        return false;
    }
    if (valueLength == 0) {
        return false;
    }
    char *endp;
    double v = strtod(buf, &endp);
    if (endp != (buf + valueLength)) {
        return false;
    }
    value = v;
    return true;
}


static const int MAX_INI_FILE_SIZE = 0x10000;

static bufDesc readFile(const char *fileName) {
    if (!fileName) {
        return {nullptr,0};
    }
    struct stat finfo{};

    if (stat(fileName, &finfo) != 0) {
//        dpmhw::dpmhw_log("File not found: %s\n", fileName);
        return {nullptr,0};
    }

    if (finfo.st_size < 0 || finfo.st_size > MAX_INI_FILE_SIZE) {
        dpmhw::dpmhw_log("Invalid size for %s : %d\n", fileName, finfo.st_size);
        return {nullptr,0};
    }

    const bufDesc buffer{static_cast<char *>(malloc(finfo.st_size)), finfo.st_size};
    if (!buffer.pBuf) {
        dpmhw::dpmhw_log("Allocation failed while reading ini file %s : %d\n", fileName, finfo.st_size);
        return {nullptr,0};
    }

    int fd = open(fileName, O_RDONLY | O_BINARY, 0);
    const bool success = fd >= 0 && read(fd, const_cast<char *>(buffer.pBuf), buffer.size) == buffer.size;
    if (fd) {
        close(fd);
    }
    if (!success) {
        free(const_cast<char*>(buffer.pBuf));
        dpmhw::dpmhw_log("Error while reading ini file %s : %d\n", fileName, finfo.st_size);
        return {nullptr,0};
    }

    return buffer;
}

IniSettings::IniSettings(const char *filePath)
: buffer(readFile(filePath)) {

}

IniSettings::~IniSettings() {
    if (buffer.pBuf) {
        free(const_cast<char*>(buffer.pBuf));
    }
}


