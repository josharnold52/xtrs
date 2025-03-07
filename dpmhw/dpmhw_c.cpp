//
// Created by arnold on 1/26/25.
//

#include <atomic>
#include "dpmhw.h"
#include "dpmhw_config.h"
#include "dpmhw_c.h"
#include "dpmhw_dacemu.h"

using namespace  cpdpmhw;

static char dacemu_mem[sizeof(dpmhw::EmulatedDac)] = {0};

std::atomic<int> init_flag(0);

inline dpmhw::EmulatedDac *dacPtr() {
    return reinterpret_cast<dpmhw::EmulatedDac *>(dacemu_mem);
}


extern "C"
int cdpmhw_init_sound(uint16_t level, int64_t now, double clocksPerSecond) {
    dpmhw::dpmhw_log("TRS-HDA: Init\n");
    int flag = 0;
    if (!(init_flag.compare_exchange_strong(flag, 1))) {
        return (flag == 1 && dacPtr()->isRunning()) ? 1 : 0;
    }
    dpmhw::dpmhw_log("TRS-HDA: Created DAC\n");
    new (dacemu_mem) dpmhw::EmulatedDac();
    dpmhw::dpmhw_log("TRS-HDA: Activating DAC\n");
    dacPtr()->activate();
    dpmhw::dpmhw_log("TRS-HDA: Stating DAC\n");
    dacPtr()->start(now, clocksPerSecond);
    atexit(cpdpmhw::cdpmhw_shutdown_sound);
    dpmhw::dpmhw_log("TRS-HDA: DAC STATE %d %d %d\n", dacPtr()->isValid() ? 1 : 0, dacPtr()->isActive() ? 1 : 0, dacPtr()->isRunning() ? 1 : 0 );
    return dacPtr()->isRunning() ? 1 : 0;
}

extern "C"
void cdpmhw_shutdown_sound() {
    int flag = 1;
    if (!(init_flag.compare_exchange_strong(flag, 2))) {
        return;
    }
    dpmhw::dpmhw_log("TRS-HD: Shutdown DAC\n");
    dacPtr()->~EmulatedDac();
}

extern "C"
void cdpmhw_sound_out(uint16_t level, int64_t now) {
    if (init_flag.load() == 1) {
        dacPtr()->soundOut(level, now);
    }
}


extern "C"
void cdpmhw_clock_update(int64_t now) {
    if (init_flag.load() == 1) {
        dacPtr()->clock_update(now);
    }
}
