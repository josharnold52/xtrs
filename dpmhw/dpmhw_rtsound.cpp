//
// Created by arnold on 12/26/24.
//

#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>
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

static inline const uint16_t LEVEL_NEUTRAL = 0x8000;

//Warning - don't change these without checking for overflow in getElapsed  - I think MAX_DELAY is the critical one
static inline const unsigned int TOTAL_BUFFER_SIZE_IN_SAMPLES = 65536;
static inline const unsigned int TOTAL_BUFFER_SIZE_IN_BYTES = 4 * TOTAL_BUFFER_SIZE_IN_SAMPLES;

static inline const tick TICKS_PER_SAMPLE = 500;
static inline const tick MAX_DELAY = ((tick)(4096)) * TICKS_PER_SAMPLE;


static inline const tick LEAD_MIN = 8192;
static inline const tick LEAD_MAX = 16384;

struct sampleLog {
    uint16_t offset;
    uint16_t level;
    uint16_t dmaOffset;
};

static sampleLog loggedSamples[0x20000];
static uint32_t sampleLogPointer  = 0;

static void writeToLog(uint16_t samplePos, uint16_t level, uint16_t curDma) {
    if (sampleLogPointer < (sizeof (loggedSamples) / sizeof(loggedSamples[0]) - 1)) {
        loggedSamples[sampleLogPointer] = {samplePos, level, curDma};
        sampleLogPointer++;
        loggedSamples[sampleLogPointer] = {0xFFFF, 0xFFFF, 0xFFFF};
    }
}

HdaRealTimeSound::HdaRealTimeSound(dpmhw::HdaDevice *d, unsigned char descNo, unsigned char streamNo)
: pDevice(d)
, stream(d, TOTAL_BUFFER_SIZE_IN_BYTES >> 1, 2, descNo, streamNo)
, started(false)
, lastClock(0)
, samplePos(0)
, fracAmt(0)
, fracWeight(0)
, ticksPerClock(1.0)
, clocksPerTick(1.0)
, lastLevel(LEVEL_NEUTRAL)
{
    if (!stream.allocationSucceeded) {
        return;
    }
    stream.dmaBuffers.fill16(LEVEL_NEUTRAL);

}

HdaRealTimeSound::~HdaRealTimeSound() {
    stop();
}


tick HdaRealTimeSound::getElapsed(const int64_t now) {
    static int32_t callcount = 0;
    static int32_t nzcallcount = 0;
    //2496004058
    // What's this? 24e6 is number of ticks per second, and (24959999078 / 10) is tsc per second
    //   24e6 is ticks per second because it is 48khz and 500 ticks per sample
    //static const double tscFactor = 24e6 * 10.0 / 24959999078.0;
    //static const double invTscFactor = 1.0 / tscFactor;

    callcount++;
    //int32_t now = pDevice->getWallClockCount();
    int64_t raw_diff = now - lastClock;
    if (raw_diff < 0) {
        dpmhw_log("neg diff %lld\n", raw_diff);
        lastClock = now;
        return -1;
    }
    if (raw_diff == 0) {
        dpmhw_log("No change raw\n");
        return 0;
    }
    auto diff = (int32_t)floor(ticksPerClock * (double)raw_diff);
    if (diff < 0 || diff > MAX_DELAY) {
        dpmhw_log("long diff %lld %lld %d\n", lastClock, raw_diff, diff);
        return -1;
    }
    nzcallcount++;
    lastClock += llround(((double)diff) * clocksPerTick);
    int32_t lead = (samplePos - curDmaSample()) & ((int32_t )(TOTAL_BUFFER_SIZE_IN_SAMPLES - 1));
    if (lead < LEAD_MIN) {
        writeToLog((uint16_t)samplePos, (uint16_t)lead, (uint16_t)curDmaSample());
        //dpmhw_log("Too small %u %lld %lld %d %d %d\n", lead, lastClock, raw_diff, diff, callcount, nzcallcount);
        diff = (diff * 1126) >> 10; // Shouldn't overflow because MAX_DELAY < 2^21
        return diff <= MAX_DELAY ? diff : -1;
    } else if (lead > LEAD_MAX) {
        writeToLog((uint16_t)samplePos, (uint16_t)lead, (uint16_t)curDmaSample());
        //dpmhw_log("Too big %u %lld %lld %d %d %d\n", lead, lastClock, raw_diff, diff, callcount, nzcallcount);
        diff = (diff * 931) >> 10;
        //return diff;
        return diff;
    } else {
        return diff;
    }
}




void HdaRealTimeSound::start(int64_t clock, double clocksPerSecond) {
    if (started) {
        return;
    }
    memset(loggedSamples, 0, sizeof(loggedSamples));
    if (!stream.allocationSucceeded) {
        dpmhw_log("Cannot start HdaRealTimeSound because stream is invalid");
        return;
    }
    stream.dmaBuffers.fill16(LEVEL_NEUTRAL);
    //TODO Need to set up codecs, etc.   For now, we'll just assume that has been done externally
    stream.run();
    started = true;
    resetBuffer(LEVEL_NEUTRAL, clock, clocksPerSecond);
}

void HdaRealTimeSound::stop() {
    if (!started) {
        return;
    }
    started = false;
    stream.stop();
    //TODO - Need to reset codecs, etc. ?  For now we'll just assume that is done externally
    auto x = open("SAMPLES.LOG", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWUSR);
    if (x >= 0) {
        write(x, loggedSamples, sizeof(loggedSamples));
        close(x);
        dpmhw_log("Wrote samples to SAMPLE.LOG\n");
    } else {
        dpmhw_log("Failed to open sample log file\n");
    }
}

void HdaRealTimeSound::resetBuffer(uint16_t level, int64_t clock) {
    if (!started) {
        return;
    }
    dpmhw_log("Resetting buffer\n");
    fracWeight = 0;
    fracAmt = 0;
    stream.dmaBuffers.fill16(level);
    samplePos = (curDmaSample() + LEAD_MAX) & ((int32_t )(TOTAL_BUFFER_SIZE_IN_SAMPLES - 1));
    lastClock = clock;
    lastLevel = level;
}
void HdaRealTimeSound::resetBuffer(uint16_t level, int64_t now, double clocksPerSecond) {
    ticksPerClock = 24e6 / clocksPerSecond;
    clocksPerTick = clocksPerSecond / 24e6;
    resetBuffer(level, now);
}




static void sendOneSample(uint16_t level, const dpmhw::DmaRegion::DmaBlock &block, int32_t &samplePos, int32_t curDmaSample) {
    static uint32_t test_todo_counter = 0;

    /*
    if (sampleLogPointer < (sizeof (loggedSamples) / sizeof(loggedSamples[0]) - 1)) {
        loggedSamples[sampleLogPointer] = {(uint16_t) samplePos, level, (uint16_t)curDmaSample};
        sampleLogPointer++;
        loggedSamples[sampleLogPointer] = {0xFFFF, 0xFFFF, 0xFFFF};
    }
    */
    if ((test_todo_counter++) > 200) {
        test_todo_counter = 0;
    }

    //uint16_t lover = test_todo_counter > 100 ? 0xD000 : 0x2000;
    uint16_t lover = level;
    uint32_t l32 = (((uint32_t)lover) << 16) | ((uint32_t)lover);
    uint32_t offset = samplePos << 2;
    block.selector.poke32(block.selectorAddress + offset, l32);

    int32_t next = (samplePos + 1) & ((int32_t)(TOTAL_BUFFER_SIZE_IN_SAMPLES - 1));
    if ((next ^ samplePos) & (~15)) {
        //we are in the next block of 16 samples (64 bytes)
        block.selector.flushLine(offset);
        //dpmhw::dpmhw_log("%08X %08X %08X\n",l32, samplePos, offset);
    }
    samplePos = next;
}

int32_t HdaRealTimeSound::soundOut(const uint16_t new_level, int64_t now) {
    // We actually want update the DMA with the previous_level, and then set new_level
    // as the previous_level so that it gets filled in on the next set.

    const auto level = lastLevel;   // The value we will update the DMA with up to the current time
    lastLevel = new_level;  // Our new_level will get filled in the next time that soundOut is called.

    int32_t sampleCounter = 0;
    if (!started) {
        return sampleCounter;
    }
    const tick origElapsed = getElapsed(now);
    tick elapsed = origElapsed;
    if (elapsed <= 0) {
        if (elapsed < 0) {
            dpmhw_log("neg elapsed %d\n", elapsed);
            resetBuffer(level, now);
        }
        return sampleCounter;
    }
    if (fracWeight > 0) {
        tick fracRemain = TICKS_PER_SAMPLE - fracWeight;
        tick fracAdd = fracRemain < elapsed ? fracRemain : elapsed;
        fracAmt += ((int32_t)fracAdd) * ((int32_t)level);
        fracWeight += fracAdd;
        if (fracWeight < TICKS_PER_SAMPLE) {
            return sampleCounter;
        }
        // Send (fracAmt / TICKS_PER_SAMPLE) sample
        sampleCounter += 1;
        //TODO
        sendOneSample((uint16_t)(fracAmt / TICKS_PER_SAMPLE), stream.dmaBuffers, samplePos, curDmaSample());
        //sendOneSample(level, stream.dmaBuffers, samplePos ,curDmaSample());  // <<- Testing if the weighting logic is buggy
        elapsed -= fracAdd;
        fracWeight = 0;
        fracAmt = 0;
    }
    //TODO - Could do this in bulk for all samples rather than a sample at a time
    while (elapsed >= TICKS_PER_SAMPLE) {
        // Send "level" sample
        sampleCounter += 1;
        sendOneSample(level, stream.dmaBuffers, samplePos, curDmaSample());
        elapsed -= TICKS_PER_SAMPLE;
    }
    if (elapsed > 0) {
        fracWeight = elapsed;
        fracAmt = ((int32_t)elapsed) * ((int32_t)level);
    }
    //if (sampleCounter > 1000) {
    //    dpmhw_log("WTF %d\n", origElapsed);
    //}
    return sampleCounter;
}


/**
 * TODO (README)
 *
 * Coming back to this after going down the atomic rabbit hole and then a pause on the project....
 * I think most of the earlier TODOs have been taken care of
 * although I still need to clean up the code.   Also, I should consider cleaning up the sample log
 * code (currently commented out).   It was pretty useful when graphed in R so I'd suggest
 *   1. Make a first-class sample log class
 *   2. Add a way to easily enable/disable the log
 *   3. Write some scripts for loading into R, etc.   IIRC I figured out how to quickly read binary data
 *      into a data frame
 *
 * What's next?
 *
 * Well, I _think_ I had come around to the idea that the RTSound "elapsed" clock should actually be based off
 * the emulated CPU clock (T state counter), and we'll rely on the emulator to keep that synced to real time
 * (and presumably the sample stream).   This would suggest that we either pass the clock value in each time
 * we send the sample, or that we have a callback of sorts to get the time.   I'm inclined to use the
 * explicit pass-in.   Maybe we pass in the clock value at start, and pass it in at each soundOut call, and
 * the RTSound clock can keep track of the previous value so it can calculate deltas.
 *
 * What units and types should we use?   Options:
 *   1. Configure the class with the frequency, and pass in the raw clock value as a 64-bit integer
 *   2. Or, fix the frequency,  In this case we could pass the value as an integer or a double.  I suspect
 *      that double will lead to fewer double <-> int64 conversions, but we want to make sure that we
 *      don't lose too much precision.   Also, double is great for time difference but not necessarily absolute
 *      time.
 *
 * When choosing, also consider that the the Model 4 emulator supports changing the clock speed.  If we go for #1,
 * we may need to reset the sound on a clock speed change.   I guess that's OK
 *
 * Overall plan of attack
 *   * Switch the clock scheme as above and test it in my test program
 *   * Start plumbing through to xtrs - initialization, shutdown (including trapping error exits!), sending the sample
 *     speed changes, configurable XTRS time base, maybe make RT mode more accurate
 */

