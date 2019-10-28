% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function varargout = sapiremote_retry(varargin)
persistent history
if isempty(history)
    history = {};
end

if nargin == 1 && strcmp(varargin{1}, 'test:reset')
    varargout{1} = history;
    history = {};
else
    history(end+1) = {varargin};
end
end
