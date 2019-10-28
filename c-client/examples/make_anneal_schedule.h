//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef MAKEANNEALSCHEDULE_H_INCLUDED
#define MAKEANNEALSCHEDULE_H_INCLUDED

#include <dwave_sapi.h>

/*
Build annealing waveform pattern. Supports pause and fast_ramp features.

nominal_anneal_time: specify length of regular linear anneal
feature_start_frac: fraction through regular anneal to begin hold
    and/or fast ramp
hold_time: amount of time to hold at constant value starting at
    feature_start_frac*nominal_anneal_time
fast_ramp_slope: if > 0.0, apply fast-ramp with given slope
    (in 1/us) following hold

Returns an anneal schedule s that can be used as a solver parameter.  To free
memory, call free(s->elements) and free(s);

Returns NULL if any parameter is out of range or if memory allocation fails.
*/
sapi_AnnealSchedule *make_anneal_schedule(
    double nominal_anneal_time, double feature_start_frac,
    double hold_time, double fast_ramp_slope);

#endif
