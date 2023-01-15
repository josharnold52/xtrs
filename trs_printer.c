/*
 * Copyright (C) 1992 Clarendon Hill Software.
 *
 * Permission is granted to any individual or institution to use, copy,
 * or redistribute this software, provided this copyright notice is retained. 
 *
 * This software is provided "as is" without any expressed or implied
 * warranty.  If this software brings on any sort of damage -- physical,
 * monetary, emotional, or brain -- too bad.  You've got no one to blame
 * but yourself. 
 *
 * The software may be modified for your own purposes, but modified versions
 * must retain this notice.
 */

/* $Id$ */

#include <sys/stat.h>
#include <string.h>
#include "z80.h"
#include "trs.h"
#include "newutils.h"

void trs_printer_write(int value)
{
    char buf[2000];
    mkdir(emulator_printer_directory, S_IWUSR);  //S_IWUSR ==> not read only
    safe_strcpy(buf, emulator_printer_directory, sizeof(buf) - 20);
    strcat(buf, "/PRINTER.OUT");

    FILE *f = fopen(buf, "ab");
    if (f) {
        putc(value & 0xFF, f);
        fclose(f);
    }
}

int trs_printer_read()
{
    return 0x30;	/* printer selected, ready, with paper, not busy */
}
