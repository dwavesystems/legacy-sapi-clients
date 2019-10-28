% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function status = sapiremote_status(h)
if isfield(h, 'test_status')
    status = h.test_status;
else
    status = false;
end
end
