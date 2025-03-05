//
// Created by arnold on 3/1/25.
//

#ifndef XTRS_DPMHW_DACEMU_H
#define XTRS_DPMHW_DACEMU_H

#include "dpmhw_pci.h"
#include "dpmhw_memory.h"
#include "dpmhw_hdadev.h"
#include "dpmhw_rtsound.h"
#include "dpmhw_hdacodec.h"


namespace dpmhw {

    //High level interface to an emulated audio DAC
    class EmulatedDac {
    private:
        dpmhw::PciFunction hdaPciFunction;
        dpmhw::PciFunctionBusMasterEnabler busMasterEnabler;
        SelectorMem deviceMemory;
        HdaDevice device;
        const bool hdaDeviceRunning;
        rtsound::HdaRealTimeSound rtSound;
        bool codecInfoLoaded = false;
        hda::codec_info codecInfo = {};
        const bool valid;
        bool active = false;
        bool running = false;

        //TODO - we probably need to do more than sets up codec - we need to activate the HDA too.   These operations
        // should be tied together....maybe have activate/deactivate commands.   Deactivating should possibly
        // power down the AFG code
        bool setupCodecs();

    public:
        EmulatedDac();
        ~EmulatedDac();


        bool activate() {
            if (!valid) {
                return false;
            }
            if (active) {
                return true;
            }
            if (!setupCodecs()) {
                return false;
            }
            active = true;
            return true;
        }

        void deactivate() {
            if (running) {
                stop();
            }
            if (active) {
                //TODO - Reset codecs?
                active = false;
            }
        }


        [[nodiscard]] bool isValid() const { return valid; }
        [[nodiscard]] bool isActive() const { return active; }
        [[nodiscard]] bool isRunning() const { return running; }

        bool start(int64_t now, double clocksPerSecond) {
            if (!active) {
                return false;
            }
            if (running) {
                return true;
            }
            rtSound.start(now, clocksPerSecond);
            running = true;
            return true;
        }
        void stop() {
            if (running) {
                rtSound.stop();
                running = false;
            }
        }

        void resetBuffer(uint16_t level, int64_t now, double clocksPerSecond) {
            if (running)
                rtSound.resetBuffer(level, now, clocksPerSecond);
        }
        void resetBuffer(uint16_t level, int64_t now) {
            if (running)
                rtSound.resetBuffer(level, now);
        }
        /**
         * @return the number of samples written to the audio stream - mostly useful for debugging
         */
        int32_t soundOut(uint16_t level, int64_t now) {
            return running ? rtSound.soundOut(level, now) : 0;
        }

    };

}



#endif //XTRS_DPMHW_DACEMU_H
