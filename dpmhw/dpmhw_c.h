//
// Created by arnold on 1/26/25.
//

#ifndef XTRS_DPMHW_C_H
#define XTRS_DPMHW_C_H

#if defined(__cplusplus)
#include <cstdint>
namespace cpdpmhw { extern "C" {
#else
#include <stdint.h>
#endif

// Return truthy if success, false otherwise
int cdpmhw_init_sound(uint16_t level, int64_t now, double clocksPerSecond);
void cdpmhw_shutdown_sound();
void cdpmhw_sound_out(uint16_t level, int64_t now);
void cdpmhw_clock_update(int64_t now);

void cdpmhw_volume_up();
void cdpmhw_volume_down();
void cdpmhw_set_clock_speed(double clocksPerSecond);

#if defined(__cplusplus)
} }
#endif


#endif //XTRS_DPMHW_C_H
