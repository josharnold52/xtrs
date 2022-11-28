
#include <time.h>
#include <dos.h>
#include "z80.h"
#include "trs.h"

//DJGPP Notes
// UCLOCKS_PER_SEC shows timing for uclock
// call uclock() to get result
// Both tstate_t and uclock_t can be assumed to be 64 bits at least (I should verify)

// Note: MODEL 1 clock is 1.774 Mhz
// Model 3 clock is faster (maybe 2Mhz?)
// Model 4 clock is selectable (I think 4Mhz/2Mhz)


#define TSTATES_PER_SEC_M1 ( 1774000u )

#define REAL_USEC_FACTOR ( 1.0e6 / UCLOCKS_PER_SEC  )

#define TSTATE_USEC_FACTOR_M1 ( 1.0e6 / TSTATES_PER_SEC_M1  )



#define MIN_TSTATE_CHANGE_BEFORE_SYNC 25


#define MIN_UCLOCK_BEFORE_RESET ( ((uclock_t)(  UCLOCKS_PER_SEC)) * 3600  )


static int realtime_suppress = 0;

static tstate_t z80_basetime;
static uclock_t real_basetime;


static tstate_t last_synced_at_tstate;
static uclock_t last_reset_at_uclock;


void trs_realtime_reset() {
    joshlog("Reset relatime counters\n");

    z80_basetime = z80_state.t_count;
    real_basetime = uclock();
    last_synced_at_tstate = z80_basetime;
    last_reset_at_uclock = real_basetime;
}


void trs_realtime_sync(tstate_t threhsold) {
    uclock_t now_uclock;
    double elapsed_rt;
    double elapsed_t;
    double elapsed_delta;
    int i;

    if (realtime_suppress > 0) {
        return;
    }
    if ( (z80_state.t_count - last_synced_at_tstate) < threhsold ) {
        return;
    }
    last_synced_at_tstate = z80_state.t_count;
    for(;;) {

        now_uclock = uclock();

        elapsed_rt = (now_uclock - real_basetime) * REAL_USEC_FACTOR;
        elapsed_t = (z80_state.t_count - z80_basetime) * TSTATE_USEC_FACTOR_M1;
        elapsed_delta = elapsed_t - elapsed_rt;
        if (elapsed_delta <= 10) {
            break;
        }
        //joshlog("SYNCING! %4.8f %4.8f %4.8f\n",elapsed_rt, elapsed_t, elapsed_delta);
        if (elapsed_delta > 1000000) {
            elapsed_delta = 1000000;
        } 
        for(i = 0; i < 10000; i++) {
            asm("pause");
        }
        //asm ("pause" : /*no output*/ : /*no input */ : /* no clobber */);
        //delay((unsigned int)elapsed_delta);
    }

    if ((now_uclock - last_reset_at_uclock) > MIN_UCLOCK_BEFORE_RESET) {
        //uclock can't reliably measure periods over 24 hours for ...reasons?.
        //So we reset everything after an hour.   Technically we might be able to only
        //reset the real time counters but then we'd have to keep track of an additional
        //offset to add to the elapsed realtime.
        trs_realtime_reset();
    }



}


void trs_realtime_disable() {
    realtime_suppress ++;
}

void trs_realtime_enable() {
    int z = --realtime_suppress;

    if (z <= 0) {
        trs_realtime_reset();
        realtime_suppress = 0;
    }
}