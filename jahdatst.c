//
// Created by Joshua Arnold on 2/25/23.
//

#include "z80.h"
#include "trs.h"


char *program_name;
int main(int argc, char **argv) {
    joshlog_echo_to_stdout = 1;

    program_name=argv[0];
    trs_ich_setup();
    return 0;
}