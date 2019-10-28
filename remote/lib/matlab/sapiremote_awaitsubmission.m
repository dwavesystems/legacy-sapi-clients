% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function newphs = sapiremote_awaitsubmission(phs, timeout)

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if ~isnumeric(timeout)
  error('sapiremote:BadArgType', 'timeout must be a number')
end
t = tic;
newphs = sapiremote_mex('awaitsubmission', phs, min(1, timeout));
r = cellfun(@isempty, newphs);
while toc(t) < timeout && any(r)
  newphs(r) = sapiremote_mex('awaitsubmission', ...
    phs(r), min(1, timeout - toc(t)));
  r = cellfun(@isempty, newphs);
end
end
