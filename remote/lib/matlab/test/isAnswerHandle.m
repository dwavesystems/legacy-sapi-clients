% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function z = isAnswerHandle(h)
z = isstruct(h) && all(isfield(h, {'internal_id', 'connection'}));
end
