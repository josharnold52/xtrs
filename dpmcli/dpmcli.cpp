//
// Created by arnold on 1/22/25.
//

#include <cstdio>
#include <cstring>
#include "../dpmutil/dpmutil_ini.h"
#include "../dpmhw/dpmhw.h"

char default_program_name[] = "dpmcli";
char *program_name;

struct args_container {
    const int argc;
    const char * const * const argv;

    const char * p(int c) {
        int x = c + 2;
        return x >= 0 && x <argc ? argv[x] : nullptr;
    }
};


struct command {
    const char *text;
    const char *usage;
    int minParams; // Counting as in param() above - i.e. - doesn't include executable or command name
    int (*cmd)(args_container &);
};

static int list_all_commands(args_container &args);

const command all_commands[] = {
        {"list-commands", nullptr, 0, list_all_commands},
        {"get-ini", "<file> <section> <key>", 3, [](args_container &args) {
            dpmutil::IniSettings ini(args.p(0));
            if (ini.hadLoadError()) {
                fprintf(stderr, "Cannot load ini\n");
                return -1;
            }
            char buf[256];
            int vlen = 0;

            if (!ini.getString(args.p(1), args.p(2), buf, sizeof(buf), vlen)) {
                fprintf(stderr, "Cannot read value\n");
                return -1;
            }

            printf("value=\"%s\"\n", buf);
            printf("length=%d\n", vlen);
            int ival = 0;
            if (ini.getInt(args.p(1), args.p(2), ival)) {
                printf("ival=%d\n", ival);
            }
            double dval = 0;
            if (ini.getDouble(args.p(1), args.p(2), dval)) {
                printf("dval=%g\n", dval);
            }
            bool bval = false;
            if (ini.getBoolean(args.p(1), args.p(2), bval)) {
                printf("bval=%s\n", bval ? "true" : "false");
            }
            char cval = '?';
            if (ini.getChar(args.p(1), args.p(2), cval)) {
                char tmps[] = {cval, 0};
                printf("cval=\'%s\'\n", tmps);
            }
            return 0;
        }}
};

static int list_all_commands(args_container &args) {
    for (auto cmd: all_commands) {
        if (cmd.usage && cmd.usage[0]) {
            printf("%s %s\n", cmd.text, cmd.usage);
        } else {
            printf("%s\n", cmd.text);
        }
    }
    return 0;
}
extern "C" {
extern int joshlog_echo_to_stdout;
}

int main(int argc, char** argv) {
    program_name = argc > 0 ? argv[0] : default_program_name;
    joshlog_echo_to_stdout = 1;
    args_container args{argc, argv};
    if (argc < 2) {
        fprintf(stderr,"NO_COMMAND: Missing command\n");
        return 1;
    }
    for(auto cmd : all_commands) {
        if (strcmp(cmd.text, argv[1]) != 0) {
            continue;
        }
        if (argc < (cmd.minParams + 2)) {
            fprintf(stderr, "NO_COMMAND: Insufficient parameters\n");
            return 1;
        }
        return cmd.cmd(args);
    }
    printf("Valid commands are:\n");
    list_all_commands(args);
    printf("\n");
    fprintf(stderr, "NO_COMMAND: Unknown command: %s\n", argv[1]);
    return -1;
}
