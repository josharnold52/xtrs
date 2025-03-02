//
// Created by arnold on 1/22/25.
//

#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include "../dpmutil/dpmutil_ini.h"
#include "../dpmhw/dpmhw.h"
#include "../dpmhw/dpmhw_config.h"

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

            if (!ini.readString(args.p(1), args.p(2), buf, sizeof(buf), vlen)) {
                fprintf(stderr, "Cannot read value\n");
                return -1;
            }

            printf("value=\"%s\"\n", buf);
            printf("length=%d\n", vlen);
            int ival = 0;
            if (ini.readInt(args.p(1), args.p(2), ival)) {
                printf("ival=%d\n", ival);
            }
            double dval = 0;
            if (ini.readDouble(args.p(1), args.p(2), dval)) {
                printf("dval=%g\n", dval);
            }
            bool bval = false;
            if (ini.readBoolean(args.p(1), args.p(2), bval)) {
                printf("bval=%s\n", bval ? "true" : "false");
            }
            char cval = '?';
            if (ini.readChar(args.p(1), args.p(2), cval)) {
                char tmps[] = {cval, 0};
                printf("cval=\'%s\'\n", tmps);
            }
            return 0;
        }},
        {"show-hw-config", nullptr, 0, [](args_container &args){
            auto cfg = dpmhw::loadConfig();
            printf("rdtscFrequencyHz=%f\n",cfg->rdtscFrequencyHz);
            printf("emulatorCpuTimer=%c\n",static_cast<char>(cfg->emulatorCpuTimer));
            printf("hdaSupported=%d\n",cfg->hdaSupported ? 1 : 0);
            printf("hdaMinDmaLead=0x%hx\n",cfg->hdaMinDmaLead);
            printf("hdaMaxDmaLead=0x%hx\n",cfg->hdaMaxDmaLead);
            return 0;
        }},
        {"measure-rdtsc", nullptr, 0, [](args_container &args){
            auto startc = clock();
            auto startu = uclock();
            auto startr = dpmhw::dpmhw_rdtsc();
            for(;;) {
                sleep(1);
                auto elapsedUclockInSeconds = static_cast<double>(uclock() - startu) * (1.0 / UCLOCKS_PER_SEC);
                auto elapsedTsc = (dpmhw::dpmhw_rdtsc() - startr);
                auto tscps = static_cast<double>(elapsedTsc) / elapsedUclockInSeconds;
                auto elapsedClockSeconds = static_cast<double>(clock() - startc) * (0.1 / CLOCKS_PER_SEC);
                printf("elapsed=%.2f  tscps=%.9e tscTicks=%-16lld clkchk=%.2f\n", elapsedUclockInSeconds, tscps, elapsedTsc, elapsedUclockInSeconds);
                if (elapsedUclockInSeconds >= 60.0) {
                    break;
                }
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
