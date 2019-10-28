//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <stdlib.h>
#include <dwave_sapi.h>
#include "make_anneal_schedule.h"

sapi_AnnealSchedule *make_anneal_schedule(
        double nominal_anneal_time, double feature_start_frac,
        double hold_time, double fast_ramp_slope) {

    sapi_AnnealSchedule *schedule;
    sapi_AnnealSchedulePoint *points;

    if (nominal_anneal_time <= 0.0
        || feature_start_frac < 0.0 || feature_start_frac > 1.0
        || hold_time < 0.0 || fast_ramp_slope < 0.0) {

        return NULL;
    }

    /* allocate memory.  At most 4 points will be created. */
    schedule = malloc(sizeof(sapi_AnnealSchedule));
    points = malloc(4 * sizeof(sapi_AnnealSchedulePoint));
    if (schedule == NULL || points == NULL) {
        free(schedule);
        free(points);
        return NULL;
    }
    schedule->elements = points;

    /* schedule always starts at (0,0) */
    points[0].time = 0.0;
    points[0].relative_current = 0.0;
    schedule->len = 1;
#define LAST_POINT (points[schedule->len - 1])

    /* do we have a feature?
     * check on feature_start_frac avoids duplicating (0,0) */
    if ((hold_time > 0 || fast_ramp_slope > 0) && feature_start_frac > 0) {
        points[schedule->len].time = feature_start_frac * nominal_anneal_time;
        points[schedule->len].relative_current = feature_start_frac;
        schedule->len++;
    }

    /* do we have a hold? */
    if (hold_time > 0) {
        points[schedule->len].time = LAST_POINT.time + hold_time;
        points[schedule->len].relative_current = LAST_POINT.relative_current;
        schedule->len++;
    }

    /* add fast ramp or rest of linear anneal
     * check that we aren't already at end of anneal */
    if (feature_start_frac < 1.0) {
        if (fast_ramp_slope > 0) {
            double ramp_time = (1.0 - LAST_POINT.relative_current) / fast_ramp_slope;
            points[schedule->len].time = LAST_POINT.time + ramp_time;
            points[schedule->len].relative_current = 1.0;
            schedule->len++;
        } else {
            points[schedule->len].time = nominal_anneal_time + hold_time;
            points[schedule->len].relative_current = 1.0;
            schedule->len++;
        }
    }
#undef LAST_POINT

    return schedule;
}
