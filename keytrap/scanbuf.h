

#ifndef INCLUDED_SCANBUFFER_H
#define INCLUDED_SCANBUFFER_H 1

struct scan_buffer {
    unsigned char suppress_flag;
    unsigned char filler0[7];
    unsigned char next_offset;
    unsigned char filler1[7];
    unsigned char key_ring[256];
    unsigned char key_states[128];
};



#endif

