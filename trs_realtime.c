
#include <time.h>
#include <dos.h>
#include "z80.h"
#include "trs.h"
#include "dpmhw/dpmhw_c.h"

//DJGPP Notes
// UCLOCKS_PER_SEC shows timing for uclock
// call uclock() to get result
// Both tstate_t and uclock_t can be assumed to be 64 bits at least (I should verify)

// Note: MODEL 1 clock is 1.774 Mhz
// Model 3 clock is faster (maybe 2Mhz?)
// Model 4 clock is selectable (I think 4Mhz/2Mhz)


#define TSTATES_PER_SEC_M1 ( 1774000u )

// Note - This doesn't take wait states into account.  For the model 3, I think
// wait stats only happen (potentially) when accessing video memory
#define TSTATES_PER_SEC_M3 ( 2027520u )


#define REAL_USEC_FACTOR ( 1.0e6 / UCLOCKS_PER_SEC  )

#define TSTATE_USEC_FACTOR_M1 ( 1.0e6 / TSTATES_PER_SEC_M1  )

#define TSTATE_USEC_FACTOR_M3 ( 1.0e6 / TSTATES_PER_SEC_M3  )


#define MIN_TSTATE_CHANGE_BEFORE_SYNC 25

#define MIN_UCLOCK_BEFORE_RESET ( ((uclock_t)(  UCLOCKS_PER_SEC)) * 3600  )


static int realtime_suppress = 0;

static tstate_t tstates_per_sec = TSTATES_PER_SEC_M1;
static double tstate_usec_factor = TSTATE_USEC_FACTOR_M1;


static tstate_t z80_basetime;
static uclock_t real_basetime;


static tstate_t last_synced_at_tstate;
static uclock_t last_reset_at_uclock;

static int m4_fast = 0;

void trs_realtime_sync_uclock(tstate_t threhsold);

void trs_realtime_reset() {
    static int did_init_sound = 0;

    joshlog("Reset realtime counters model=%d\n", trs_model);
    // TODO: These values should be calculated from z80_state.clockMHz - and we should automatically
    //  reset whenever they change.  (Maybe save old value and do a very quick comparison / reset-on-change when
    //  we perform a realtime sync.  This way, we won't need to explicitly reset when the speed changes
    if (trs_model == 1) {
        tstate_usec_factor = TSTATE_USEC_FACTOR_M1;
        tstates_per_sec = TSTATES_PER_SEC_M1;
        //TODO
        if (trs_dpmsound_enabled) {
            joshlog("Before init call\n");
            cdpmhw_init_sound(0x8000, z80_state.t_count, TSTATES_PER_SEC_M1);
            joshlog("After init call\n");
            cdpmhw_clock_update(z80_state.t_count);
        }
    } else if (trs_model == 3) {
        tstate_usec_factor = TSTATE_USEC_FACTOR_M3;
        tstates_per_sec = TSTATES_PER_SEC_M3;
    } else {
        //M4
        tstate_usec_factor = TSTATE_USEC_FACTOR_M3 * (m4_fast ? 0.5 : 1);
        tstates_per_sec = TSTATES_PER_SEC_M3 * (m4_fast ? 2 : 1);
    }
    trs_realtime_sync = trs_realtime_sync_uclock;
    z80_basetime = z80_state.t_count;
    real_basetime = uclock();
    last_synced_at_tstate = z80_basetime;
    last_reset_at_uclock = real_basetime;
    if (trs_dpmsound_enabled) {
        if (!did_init_sound) {
            joshlog("Initializing dpmhw sound\n");
            if (!cdpmhw_init_sound(0x8000, (int64_t)(z80_state.t_count), TSTATES_PER_SEC_M1)) {
                joshlog("dpmhw initialization failed - disabling sound\n");
                trs_dpmsound_enabled = 0;
            } else {
                joshlog("Sound initialized\n");
            }
        }
    }
    if (trs_dpmsound_enabled) {
        cdpmhw_clock_update((int64_t)(z80_state.t_count));
        cdpmhw_set_clock_speed((double)tstates_per_sec);
    }
}

void trs_realtime_set_m4_speed(int fast) {
    m4_fast = fast;
    if (trs_model == 4) {
        trs_realtime_reset();
    }
}

unsigned long long trs_rt_rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

void trs_realtime_log_status(char ctl) { 

    uclock_t now_uclock;
    time_t now_tod;
    unsigned long long tsc;

    now_uclock = uclock();
    now_tod = time(0);

    if (ctl & 128) {
        tsc = trs_rt_rdtsc();
    } else {
        tsc = 0;
    }

    double elapsed_rt;
    double elapsed_t;
    double elapsed_delta;

    elapsed_rt = (now_uclock - real_basetime) * REAL_USEC_FACTOR;
    elapsed_t = (z80_state.t_count - z80_basetime) * tstate_usec_factor;
    elapsed_delta = elapsed_t - elapsed_rt;



    joshlog("TSC=%llu,UTIME=%u,ST=%lu,UT=%lu,TS=%llu,TSB=%llu,TSR=%llu,UC=%lld,UCB=%lld,UCR=%lld,UPS=%lld,TPS=%llu,ERT=%4.8f,ET=%4.8f,ED=%4.8f\n",
        tsc,now_tod,sizeof(tstate_t), sizeof(uclock_t),
        z80_state.t_count, z80_basetime, last_synced_at_tstate,
        now_uclock, real_basetime, last_reset_at_uclock,
        ((uclock_t)UCLOCKS_PER_SEC), ((tstate_t)tstates_per_sec),
        elapsed_rt, elapsed_t, elapsed_delta);
}

void trs_realtime_sync_uclock(tstate_t threhsold) {
    uclock_t now_uclock;
    double elapsed_rt;
    double elapsed_t;
    double elapsed_delta;
    int i;

    if (realtime_suppress > 0) {
        //Process keyboard events (so we don't hang up if the CPU never touches keyboard memory)
        trs_get_event(0);
        return;
    }
    if (trs_dpmsound_enabled) {
        cdpmhw_clock_update(z80_state.t_count);
    }
    if ( (z80_state.t_count - last_synced_at_tstate) < threhsold ) {
        return;
    }
    last_synced_at_tstate = z80_state.t_count;
    for(;;) {
        //Trying this for power
        __asm__ __volatile__ ("pause");
        //Process keyboard events (so we don't hang up if the CPU never touches keyboard memory)
        trs_get_event(0);

        //Now sync...
        now_uclock = uclock();

        elapsed_rt = ((double)(now_uclock - real_basetime)) * REAL_USEC_FACTOR;
        elapsed_t = ((double)(z80_state.t_count - z80_basetime)) * tstate_usec_factor;
        elapsed_delta = elapsed_t - elapsed_rt;
        if (elapsed_delta <= 10) {
            break;
        }
        //joshlog("SYNCING! %4.8f %4.8f %4.8f\n",elapsed_rt, elapsed_t, elapsed_delta);
        if (elapsed_delta > 1000000) {
            elapsed_delta = 1000000;
        } 
        for(i = 0; i < 100; i++) {
            __asm__ __volatile__ ("pause");
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
    int pre = realtime_suppress;
    int post = realtime_suppress + 1;
    realtime_suppress = post;
    if (pre <= 0 && post > 0)
        joshlog("Realtime throttle is suppressed! %d\n", realtime_suppress);
}

static void do_trs_realtime_enable(int force) {
    int pre = realtime_suppress;
    int post = force ? 0 : ((pre > 0) ? pre - 1 : 0);
    realtime_suppress = post;
    if (pre > 0 && post <= 0) {
        trs_realtime_reset();
        joshlog("Realtime throttle is enabled! %d\n", realtime_suppress);
    }
}

void trs_realtime_enable() {
    do_trs_realtime_enable(0);
}
void trs_realtime_force_enable() {
    do_trs_realtime_enable(1);
}

int trs_is_realtime_enabled() {
    return realtime_suppress == 0;
}

void (*trs_realtime_sync)(tstate_t threhsold) = trs_realtime_sync_uclock;
