% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [h, varargout] = answerHandle(type, varargin)

solvers = sapiremote_solvers(sapiremote_connection('', '', []));

switch type
  case 'incomplete'
    params = initParams(varargin, 1);
    h = sapiremote_submit(solvers(1), '', [], params);
  case 'answer'
    params = initParams(varargin, 3);
    params.answer = true;
    if nargout > 1
      varargout{1} = struct( ...
        'type', varargin{1}, ...
        'data', varargin{2}, ...
        'params', params);
    end
    h = sapiremote_submit(solvers(1), varargin{1}, varargin{2}, params);
  case 'error'
    params = initParams(varargin, 2);
    params.error = varargin{1};
    h = sapiremote_submit(solvers(1), '', [], params);
  otherwise
    error('bad type')
end
end

function params = initParams(args, idIndex)
assert(numel(args) <= idIndex);
params = struct;
if numel(args) == idIndex
  params.problem_id = args{end};
end
end
