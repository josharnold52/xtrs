//
// Created by arnold on 12/22/24.
//

#include <cstdarg>
#include "dpmhw.h"
#include "dpmhw_impl.h"

extern "C" {
#include "../z80.h"
}

void dpmhw::dpmhw_debug(const char *msg, ...){}

void dpmhw::dpmhw_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    joshlogv(fmt, args);
    va_end(args);
}








