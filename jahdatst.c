//
// Created by Joshua Arnold on 2/25/23.
//

#include "z80.h"
#include "trs.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void test_exit_handler() {
    printf("My handler during exit...\n");
}

/**
 * NOTE: I need to know how to trap signals so that I can shutdown hda audio on exit
 * @param sig
 */
void test_signal_trapper(int sig) {
    printf("trapped sig %d\n",sig);
    __djgpp_traceback_exit(sig);
}

char *program_name;
int main(int argc, char **argv) {
    joshlog_echo_to_stdout = 1;
    atexit(test_exit_handler);

    program_name=argv[0];
    //Test signal trapping - see above for why I think I need this
    signal(SIGSEGV, test_signal_trapper);
    trs_ich_setup();
    return 0;
}