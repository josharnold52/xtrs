//
// Created by arnold on 12/26/24.
//

#ifndef XTRS_DPMHW_RTSOUND_H
#define XTRS_DPMHW_RTSOUND_H

#include "dpmhw_hdadev.h"
#include "dpmhw_hdastream.h"

namespace dpmhw::rtsound {

    class HdaRealTimeSound {
    public:
        typedef int32_t tick;
    private:
        HdaDevice * const pDevice;
        HdaOutputStream stream;
        bool started;
        int32_t lastWallClock;

        int32_t samplePos;
        int32_t fracAmt;
        tick fracWeight;

    private:
        tick getElapsed();

        int32_t curDmaSample() {
            return (int32_t)( pDevice->getDmaPos(stream.descriptorNumber) >> 2);  // shift to convert to samples
        }
    public:
        HdaRealTimeSound(HdaDevice *d, unsigned char descNo, unsigned char streamNo);
        ~HdaRealTimeSound();

        HdaRealTimeSound(const HdaRealTimeSound &rhs) = delete;
        void operator=(const HdaRealTimeSound&) = delete;

        void start();
        void stop();

        void resetBuffer(uint16_t level);
        void soundOut(uint16_t level);

    };

}

#endif //XTRS_DPMHW_RTSOUND_H
