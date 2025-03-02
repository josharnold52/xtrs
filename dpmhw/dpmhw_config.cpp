//
// Created by arnold on 1/14/25.
//

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "dpmhw_config.h"
#include "../dpmutil/dpmutil_ini.h"

using namespace dpmhw;

namespace {
    std::atomic<DPMHardwareConfig *> config_ptr(nullptr);


    HRTimerSource getTimerSourceOrElse(const dpmutil::IniSettings &ini, const char *section, const char *key, HRTimerSource defaultValue) {
        auto c  = HRTimerSource{ini.getCharOrElse(section, key, static_cast<char>(defaultValue))};
        if (c == HRTimerSource::uclock || c == HRTimerSource::hdaclock || c == HRTimerSource::rdtsc) {
            return c;
        }
        return defaultValue;
    }

    void readConfig(const dpmutil::IniSettings &ini, DPMHardwareConfig &cfg) {
        static_assert(std::numeric_limits<double>::has_quiet_NaN);
        const double nan = std::numeric_limits<double>::quiet_NaN();
        cfg.rdtscFrequencyHz = ini.getDoubleOrElse("dpmhw", "rdtscFrequencyHz", nan);


        cfg.hdaSupported = ini.getBooleanOrElse("dpmhw", "hdaSupported", false);
        cfg.hdaMinDmaLead = ini.getIntOrElse("dpmhw", "hdaMinDmaLead", 256);
        if (cfg.hdaMinDmaLead > 8192) {
            cfg.hdaMinDmaLead = 8192;
        }
        cfg.hdaMaxDmaLead = ini.getIntOrElse("dpmhw", "hdaMaxDmaLead", cfg.hdaMinDmaLead + 512);
        if (cfg.hdaMaxDmaLead < cfg.hdaMinDmaLead) {
            cfg.hdaMaxDmaLead = cfg.hdaMinDmaLead + 512;
        } else if (cfg.hdaMaxDmaLead > 16384) {
            cfg.hdaMaxDmaLead = 16384;
        }

        cfg.emulatorCpuTimer = getTimerSourceOrElse(ini, "dpmhw", "emulatorCpuTimer", HRTimerSource::uclock);
        if (cfg.emulatorCpuTimer == HRTimerSource::hdaclock && !cfg.hdaSupported) {
            cfg.emulatorCpuTimer = HRTimerSource::uclock;
        }
        if (cfg.emulatorCpuTimer == HRTimerSource::rdtsc && !(std::isfinite(cfg.rdtscFrequencyHz) && cfg.rdtscFrequencyHz > 0)) {
            cfg.emulatorCpuTimer = HRTimerSource::uclock;
        }
    }

}




const DPMHardwareConfig *dpmhw::loadConfig() {
    auto cur = config_ptr.load();
    if (cur != nullptr) {
        return cur;
    }
    auto * const p = static_cast<DPMHardwareConfig*>(malloc(sizeof(DPMHardwareConfig)));
    p->hdaSupported = false;  //This will blow up if we can't allocate the config, but I think we may as well just crash in that case
    char name[1100];
    auto cfName = getenv("DPMHW_CONFIG");
    if (cfName && (strlen(cfName) < sizeof(name))) {
        strcpy(name, cfName);
    } else {
        strcpy(name, "DPMHW.INI");
    }
    dpmutil::IniSettings ini(name);
    readConfig(ini, *p);

    DPMHardwareConfig *pExpect = nullptr;
    if (!config_ptr.compare_exchange_strong(pExpect, p)) {
        free(p);
        return pExpect;
    }
    return p;
}

