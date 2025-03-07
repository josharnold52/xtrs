//
// Created by arnold on 1/14/25.
//

#ifndef XTRS_DPMHW_CONFIG_H
#define XTRS_DPMHW_CONFIG_H

#if defined(__cplusplus)
#include <cstdint>
#else
#include <stdint.h>
#endif

#if defined(__cplusplus)
namespace dpmhw {
#endif


#if defined(__cplusplus)
    enum class HRTimerSource : char {
        rdtsc = 'r',
        hdaclock = 'h',
        uclock = 'u'
    };
#else
    typedef char HRTimerSource;
#define HR_TIMER_SOURCE_RDTSC ('r')
#define HR_TIMER_SOURCE_HDACLOCK ('h')
#define HR_TIMER_SOURCE_UCLOCK ('u')
#endif

    /** Note: The constraints listed below are enforced by the config reader */
    struct DPMHardwareConfig {
        double rdtscFrequencyHz;  // NaN if not supported.  Otherwise is a positive finite value

        HRTimerSource emulatorCpuTimer;

        bool hdaSupported;
        uint16_t hdaMinDmaLead;   // Minimum DMA lead value for HDA - maximum of 8192
        uint16_t hdaMaxDmaLead;   // Maximum DMA lead value for HDA - maximum of 16384
    };

#if !defined(__cplusplus)
    typedef struct DPMHardwareConfig DPMHardwareConfig;
#endif


#if defined(__cplusplus)
    const DPMHardwareConfig *loadConfig();
#else
    extern "C" const DPMHardwareConfig *cdpmhw_load_config();
#endif

#if defined(__cplusplus)
}
#endif

#endif //XTRS_DPMHW_CONFIG_H
