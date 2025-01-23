//
// Created by arnold on 12/22/24.
//

#include <cstdarg>
#include <cstdint>
#include "dpmhw.h"
#include "dpmhw_impl.h"

extern "C" {
#include "../z80.h"
}

 bool dpmhw::dpmhw_debug_enabled = false;

void dpmhw::dpmhw_debug(const char *fmt, ...){
    if (dpmhw_debug_enabled) {
        va_list args;
        va_start(args, fmt);
        joshlogv(fmt, args);
        va_end(args);
    }
}

void dpmhw::dpmhw_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    joshlogv(fmt, args);
    va_end(args);
}

bool dpmhw::isPowerOfTwo(uint32_t ai) {
    if (ai == 0) {
        return false;
    }
    while( !(ai & 1)) {
        ai >>= 1;
    }
    return ai == 1;
}

static_assert(sizeof(int) == 4, "The DPM code assumes int is 4 bytes");
static_assert(sizeof(long) == 4, "The DPM code assumes long is 4 bytes");
static_assert(sizeof(long long) == 8, "The DPM code assumes long long is 8 bytes");







