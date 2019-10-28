% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

%makeAnnealSchedule Create and anneal schedule with pause and ramp
%
%  schedule = makeAnnealSchedule(nominalAnnealTime, featureStartFrac, ...
%      holdTime, fastRampSlope)
%
%      nominalAnnealTime: length of regular linear anneal
%      featureStartFrac: fraction through regular anneal to begin hold
%          and/or fast ramp (default: 0)
%      holdTime: amount of time to hold at constant value starting at
%          featureStartFrac * nominalAnnealTime (default: 0)
%      fast_ramp_slope: if > 0, apply fast-ramp with given slope
%          (in 1/us) following hold (default: 0)

function schedule = makeAnnealSchedule(nominalAnnealTime, ...
    featureStartFrac, holdTime, fastRampSlope)

if nargin < 2 || isempty(featureStartFrac)
    featureStartFrac = 0;
end
if nargin < 3 || isempty(holdTime)
    holdTime = 0;
end
if nargin < 4 || isempty(fastRampSlope)
    fastRampSlope = 0;
end

if nominalAnnealTime <= 0
    error('nomimalAnnealTime must be positive')
end
if featureStartFrac < 0 || featureStartFrac > 1
    error('featureStartFrac must be in the range [0, 1]')
end
if holdTime < 0
    error('holdTime must be non-negative')
end
if fastRampSlope < 0
    error('fastRampSlope must be non-negative')
end

% schedule always starts at (0,0)
schedule = [0; 0];

% do we have a feature?
% check on feature_start_frac avoids duplicating (0,0)
if (holdTime > 0 || fastRampSlope > 0) && featureStartFrac > 0
    schedule(:, end+1) = ...
        [featureStartFrac * nominalAnnealTime; featureStartFrac];
end

% do we have a hold?
if holdTime > 0
    schedule(:, end+1) = schedule(:, end) + [holdTime; 0];
end

% add fast ramp or rest of linear anneal
% check that we aren't already at end of anneal
if featureStartFrac < 1
    if fastRampSlope > 0
        rampTime = (1 - schedule(2, end)) / fastRampSlope;
        schedule(:, end+1) = [schedule(1, end) + rampTime; 1];
    else
        schedule(:, end+1) = [nominalAnnealTime + holdTime; 1.0];
    end
end
end
