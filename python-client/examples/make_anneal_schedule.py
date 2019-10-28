# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

def make_anneal_schedule(nominal_anneal_time, feature_start_frac=0.0,
                         hold_time=0.0, fast_ramp_slope=0.0):
    """Build annealing waveform pattern. Supports pause and fast_ramp features.

    nominal_anneal_time: specify length of regular linear anneal
    feature_start_frac: fraction through regular anneal to begin hold
        and/or fast ramp
    hold_time: amount of time to hold at constant value starting at
        feature_start_frac*nominal_anneal_time
    fast_ramp_slope: if > 0.0, apply fast-ramp with given slope
        (in 1/us) following hold
    """
    # validate parameters
    if nominal_anneal_time <= 0.0:
        raise ValueError("anneal-time must be > 0.0")
    if feature_start_frac < 0.0 or feature_start_frac > 1.0:
        raise ValueError("feature_start_frac must be >= 0.0 and <= 1.0")
    if hold_time < 0.0:
        raise ValueError("hold_time must be >= 0.0")
    if fast_ramp_slope < 0.0:
        raise ValueError("fast_ramp_slope must be >= 0.0")

    # schedule always starts at (0,0)
    schedule = [[0.0, 0.0]]

    # do we have a feature?
    # check on feature_start_frac avoids duplicating (0,0)
    if ((hold_time > 0) or (fast_ramp_slope > 0)) and feature_start_frac > 0:
        schedule.append([feature_start_frac * nominal_anneal_time,
                         feature_start_frac])

    # do we have a hold?
    if hold_time > 0:
        last_point = schedule[-1]
        schedule.append([last_point[0] + hold_time, last_point[1]])

    # add fast ramp or rest of linear anneal
    # check that we aren't already at end of anneal
    if feature_start_frac < 1.0:
        if fast_ramp_slope > 0:
            last_point = schedule[-1]
            ramp_time = (1.0 - last_point[1]) / fast_ramp_slope
            schedule.append([last_point[0] + ramp_time, 1.0])
        else:
            schedule.append([nominal_anneal_time + hold_time, 1.0])

    return schedule
