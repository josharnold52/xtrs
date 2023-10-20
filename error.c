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

/*
   Modified by Timothy Mann, 1996 and later
   $Id$
*/

#include "z80.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

int joshlog_echo_to_stdout = 0;

extern char *program_name;

void debug(const char *fmt, ...)
{
  va_list args;
  char xfmt[2064];
  FILE *f;

  strcpy(xfmt, "XTRS debug: ");
  strcat(xfmt, fmt);
  /*strcat(xfmt, "\n");*/
  va_start(args, fmt);
  f = fopen("josh.log", "a");
  vfprintf(f, xfmt, args);
  fflush(f);
  fclose(f);
  va_end(args);
}

void error(const char *fmt, ...)
{
  va_list args;
  char xfmt[2064];
  FILE *f;

  strcpy(xfmt, program_name);
  strcat(xfmt, "XTRS error: ");
  strcat(xfmt, fmt);
  strcat(xfmt, "\n");
  va_start(args, fmt);
  f = fopen("josh.log", "a");
  vfprintf(f, xfmt, args);
  fflush(f);
  fclose(f);
  va_end(args);
}

void fatal(const char *fmt, ...)
{
  va_list args;
  char xfmt[2064];
  FILE *f;

  strcpy(xfmt, program_name);
  strcat(xfmt, "XTRS fatal error: ");
  strcat(xfmt, fmt);
  strcat(xfmt, "\n");
  va_start(args, fmt);
  f = fopen("josh.log", "a");
  vfprintf(f, xfmt, args);
  fflush(f);
  fclose(f);
  va_end(args);
  exit(1);
}

void joshlog(const char *fmt, ...)
{

  va_list args;

  va_start(args, fmt);
  joshlogv(fmt, args);
  va_end(args);
}

extern void joshlogv(const char *fmt, va_list args) {
    FILE *f;
    f = fopen("josh.log", "a");
    if (joshlog_echo_to_stdout) {
        vprintf(fmt, args);
    }
    vfprintf(f, fmt, args);
    fflush(f);
    fclose(f);
}
