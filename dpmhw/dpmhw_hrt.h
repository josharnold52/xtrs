//
// Created by arnold on 1/14/25.
//

#ifndef XTRS_DPMHW_HRT_H
#define XTRS_DPMHW_HRT_H

#include <cstdint>

#include "dpmhw.h"
#include "dpmhw_hdadev.h"

namespace dpmhw::hrt {

    typedef int64_t HRTime;

    class HRTimer {
    public:
        typedef HRTime (*TimeSource)(void *) noexcept;
        const double frequencyHz;
        const TimeSource source;
        void * const sourceData;

    private:
        HRTimer(double frequencyHz, TimeSource source, void *sourceData) :
            frequencyHz(frequencyHz), source(source), sourceData(sourceData) {};
    public:
        static HRTimer createRdtscTimer();
        static HRTimer createUclockTimer();
        static HRTimer createHdaTimer(HdaDevice *pDev);

    };


}


#endif //XTRS_DPMHW_HRT_H
