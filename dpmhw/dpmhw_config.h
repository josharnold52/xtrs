//
// Created by arnold on 1/14/25.
//

#ifndef XTRS_DPMHW_CONFIG_H
#define XTRS_DPMHW_CONFIG_H

#include <cstdint>

namespace dpmhw {

    enum class HRTimerSource : char {
        rdtsc = 'r',
        hdaclock = 'h',
        uclock = 'u'
    };

    /** Note: The constraints listed below are enforced by the config reader */
    struct DPMHardwareConfig {
        double rdtscFrequencyHz;  // NaN if not supported.  Otherwise is a positive finite value

        HRTimerSource emulatorCpuTimer;

        bool hdaSupported;
        uint16_t hdaMinDmaLead;   // Minimum DMA lead value for HDA - maximum of 8192
        uint16_t hdaMaxDmaLead;   // Maximum DMA lead value for HDA - maximum of 16384
    };

    const DPMHardwareConfig *loadConfig();
}

#endif //XTRS_DPMHW_CONFIG_H
