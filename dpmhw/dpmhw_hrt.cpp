//
// Created by arnold on 1/14/25.
//

#include <ctime>
#include "dpmhw_hrt.h"

using namespace dpmhw::hrt;

static HRTime rdtscFunc(void *) noexcept {
    return dpmhw::dpmhw_rdtsc();
}


HRTimer HRTimer::createRdtscTimer() {
    //TODO Get rdtsc config
    return {1e8, rdtscFunc, nullptr};
}

static_assert(sizeof (HRTime) == sizeof (uclock_t));
static HRTime uclockFunc(void *) noexcept {
    return uclock();
}

HRTimer HRTimer::createUclockTimer() {
    return {(double)UCLOCKS_PER_SEC, uclockFunc, nullptr};
}

static HRTime hdaFunc(void * p) noexcept {
    auto pHda = static_cast<dpmhw::HdaDevice *>(p);
    return ((HRTime)pHda->getWallClockCount()) << 32;
}

HRTimer HRTimer::createHdaTimer(dpmhw::HdaDevice *pDev) {
    return {24e6 * (1ll << 32), hdaFunc, pDev};
}


