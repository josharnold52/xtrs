//
// Created by arnold on 12/26/24.
//

#include "dpmhw_rtsound.h"
#include "dpmhw_hdadev.h"
#include "dpmhw_hdastream.h"
#include "dpmhw_hdacodec.h"
#include "dpmhw_impl.h"
#include "dpmhw_memory.h"
#include "dpmhw_pci.h"

using namespace dpmhw::rtsound;


typedef HdaRealTimeSound::tick tick;

//This needs to keep track of the last time it was called
//This should also evaluate where we are relative to the DMA Pointer and stretch/contract the elapsed time accordingly
// We probably need to set the expected "lead" time based on hardware.
// Ideally we keep lead time within 10ms but that may not be possible with virtualbox.
// -> VirtualBox DMA pointer seems to round to 2K (= 512 samples =~ 12 ms)
//       So perhaps aim to be 4K ahead ( ~ 24 ms).  If we see it fall to 2K then speed up and if we're at 6K then slow down
// -> With real hardware, could maybe try to aim for 1K ahead (~ 6ms) and speed up/slow down at 1K - 128 or 1K + 128
//    However, maybe get virtualbox to work first

//  With a 4K target lead, I might aim for a 8K buffer.  Could do more, but not sure why I'd bother.   Well... Maybe
//   a 16K buffer would be better at the extremes (with 8K, I can't distinguish between 6K ahead and 2K behind.  Of course,
//   behind is a bad state but it would be good to be able to detect it.

// Thinking maybe aim to be 6K ahead for my first pass

/*
tick getElapsed();

const int TICK_FRAC_BITS = 8;
const tick TICK_UNIT = 1 << TICK_FRAC_BITS;
const tick TICK_FRAC_MASK = TICK_UNIT - 1;
const tick TICK_UNIT_MASK = ~TICK_FRAC_MASK;

const tick MAX_DELAY = 4096 << TICK_FRAC_BITS; //TODO Change this

void sendDma(uint16_t  sampleVal) {
    static tick lastTick;

    static int32_t fracAmt;
    static tick fracWeight;

    static int offset;
    static int bufferSize;


    tick elapsed = getElapsed();
    if (elapsed == 0) {
        return;
    }
    if (elapsed < 0 || elapsed > MAX_DELAY) {
        //TODO reset and restart
        return;
    }
    if (fracWeight > 0) {
        tick fracRemain = TICK_UNIT - fracAmt;
        tick fracAdd = fracRemain > elapsed ? fracRemain : elapsed;
        fracAmt += ((int32_t)fracAdd) * ((int32_t)sampleVal);
        fracWeight += fracAdd;
        elapsed -= fracAdd;
        if (fracWeight == TICK_UNIT) {
            //TODO - Send sample
            fracWeight = 0;
            fracAmt = 0;
        }
    }
    const tick wholeUnits = elapsed & TICK_UNIT_MASK;
    if (wholeUnits > 0) {
        const int32_t n = wholeUnits >> TICK_FRAC_BITS;
        //TODO - send "n" samples
        elapsed -= wholeUnits;
    }
    const tick frackUnits = elapsed & TICK_FRAC_MASK;
    fracWeight = frackUnits;

}
*/

//Warning - don't change these without checking for overflow in getElapsed
static inline const unsigned int TOTAL_BUFFER_SIZE_IN_SAMPLES = 4096;
static inline const unsigned int TOTAL_BUFFER_SIZE_IN_BYTES = 4 * TOTAL_BUFFER_SIZE_IN_SAMPLES;

static inline const tick TICKS_PER_SAMPLE = 500;
static inline const tick MAX_DELAY = ((tick)(TOTAL_BUFFER_SIZE_IN_SAMPLES)) * TICKS_PER_SAMPLE;


HdaRealTimeSound::HdaRealTimeSound(dpmhw::HdaDevice *d, unsigned char descNo, unsigned char streamNo)
: pDevice(d)
, stream(d, TOTAL_BUFFER_SIZE_IN_BYTES >> 1, 2, descNo, streamNo)
, started(false)
, lastWallClock(0)
, samplePos(0)
, fracAmt(0)
, fracWeight(0)
{
    if (!stream.allocationSucceeded) {
        return;
    }
    stream.dmaBuffers.fill16(0x8000);

}

HdaRealTimeSound::~HdaRealTimeSound() {
    stop();
}

tick HdaRealTimeSound::getElapsed() {
    int32_t now = pDevice->getWallClockCount();
    int32_t diff = now - lastWallClock;
    if (diff == 0) {
        return 0;
    }
    lastWallClock = now;
    if (diff < 0 || diff > MAX_DELAY) {
        return -1;
    }
    int32_t lead = (samplePos - curDmaSample()) & ((int32_t )(TOTAL_BUFFER_SIZE_IN_SAMPLES - 1));
    if (lead <= 512) {
        diff = (diff * 1126) >> 10; // Shouldn't overflow because MAX_DELAY < 2^21
        return diff <= MAX_DELAY ? diff : -1;
    } else if (lead >= 2048) {
        diff = (diff * 931) >> 10;
        return diff;
    } else {
        return diff;
    }
}

void HdaRealTimeSound::start() {
    if (started) {
        return;
    }
    if (!stream.allocationSucceeded) {
        dpmhw_log("Cannot start HdaRealTimeSound because stream is invalid");
        return;
    }
    stream.dmaBuffers.fill16(0x8000);
    //TODO Need to set up codecs, etc.   For now, we'll just assume that has been done externally
    stream.run();
    started = true;
}

void HdaRealTimeSound::stop() {
    if (!started) {
        return;
    }
    stream.stop();
    //TODO - Need to reset codecs, etc. ?  For now we'll just assume that is done externally
}

void HdaRealTimeSound::resetBuffer(uint16_t level) {
    if (!started) {
        return;
    }
    fracWeight = 0;
    fracAmt = 0;
    stream.dmaBuffers.fill16(level);
    //1280 is midway between our too-fast/too-slow thresholds
    samplePos = (curDmaSample() + 1280) & ((int32_t )(TOTAL_BUFFER_SIZE_IN_SAMPLES - 1));
}

static void sendOneSample(uint16_t level, const dpmhw::DmaRegion::DmaBlock &block, int32_t &samplePos) {
    uint32_t l32 = (((uint32_t)level) << 16) | ((uint32_t)level);
    uint32_t offset = samplePos << 2;
    block.selector.poke32(block.selectorAddress + offset, l32);

    int32_t next = (samplePos + 1) & ((int32_t)(TOTAL_BUFFER_SIZE_IN_SAMPLES - 1));
    if ((next ^ samplePos) & (~15)) {
        //we are in the next block of 16 samples (64 bytes)
        block.selector.flushLine(offset);
    }
    samplePos = next;
}

void HdaRealTimeSound::soundOut(uint16_t level) {
    if (!started) {
        return;
    }
    tick elapsed = getElapsed();
    if (elapsed <= 0) {
        if (elapsed < 0) {
            resetBuffer(level);
        }
        return;
    }
    if (fracWeight > 0) {
        tick fracRemain = TICKS_PER_SAMPLE - fracWeight;
        tick fracAdd = fracRemain < elapsed ? fracRemain : elapsed;
        fracAmt += ((int32_t)fracAdd) * ((int32_t)level);
        fracWeight += fracAdd;
        if (fracWeight < TICKS_PER_SAMPLE) {
            return;
        }
        // Send (fracAmt / TICKS_PER_SAMPLE) sample
        sendOneSample((uint16_t)(fracAmt / TICKS_PER_SAMPLE), stream.dmaBuffers, samplePos);
        elapsed -= fracAdd;
        fracWeight = 0;
        fracAmt = 0;
    }
    //TODO - Could do this in bulk for all samples rather than a sample at a time
    while(elapsed > TICKS_PER_SAMPLE) {
        // Send "level" sample
        sendOneSample(level, stream.dmaBuffers, samplePos);
        elapsed -= TICKS_PER_SAMPLE;
    }
    if (elapsed > 0) {
        fracWeight = elapsed;
        fracAmt = ((int32_t)elapsed) * ((int32_t)level);
    }
}


