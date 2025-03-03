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
        int64_t lastClock;

        int32_t samplePos;
        int32_t fracAmt;
        tick fracWeight;

        double ticksPerClock;
        double clocksPerTick;

        uint16_t lastLevel;
    private:
        tick getElapsed(int64_t clock);

        int32_t curDmaSample() {
            return (int32_t)( pDevice->getDmaPos(stream.descriptorNumber) >> 2);  // shift to convert to samples
        }
    public:
        /**
         * Default constructor is unusable but convenient if we want to pre-allocate storage in a controlled way
         * and then later overwrite it with placement-new move constructor
         */
        HdaRealTimeSound();
        HdaRealTimeSound(HdaDevice *d, unsigned char descNo, unsigned char streamNo);
        ~HdaRealTimeSound();

        HdaRealTimeSound(const HdaRealTimeSound &rhs) = delete;
        void operator=(const HdaRealTimeSound&) = delete;

        // Enable move construction since HdaOutputStream now supports it
        HdaRealTimeSound(HdaRealTimeSound &&rhs) = default;


        void start(int64_t now, double clocksPerSecond);
        void stop();

        void resetBuffer(uint16_t level, int64_t now, double clocksPerSecond);
        void resetBuffer(uint16_t level, int64_t now);
        int32_t soundOut(uint16_t level, int64_t now);

        [[nodiscard]] bool isValid() const {
            return stream.allocationSucceeded.get();
        }
        [[nodiscard]] unsigned char getStreamNumber() const {
            return stream.getStreamNumber();
        }
        [[nodiscard]] unsigned char getDescriptorNumber() const {
            return stream.getDescriptorNumber();
        }
        [[nodiscard]] unsigned char getStreamFormat() const {
            return stream.getFormat();
        }

    };

}

#endif //XTRS_DPMHW_RTSOUND_H
