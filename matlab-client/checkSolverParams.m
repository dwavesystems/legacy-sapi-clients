% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function checkSolverParams(solver, params)

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

persistent message
if isempty(message)
    lines = {
        'unable to validate parameter names'
        ''
        'There is no information available about the parameter names that this'
        'solver accepts. It still works but misspelled parameter names will be'
        'silently ignored. Please complain to your system administrator.'
        ''
        '*** This is not an error! Your problem has been submitted! ***'
        ''
        'You can disable this warning message with this command:'
        ''
        '    warning(''OFF'', ''sapi:ParameterValidation'')'
    };
    message = sprintf('%s\n', lines{:});
end

props = sapiSolverProperties(solver);
if ~isfield(props, 'parameters')
    warning('sapi:ParameterValidation', message);
else
    solverParamsNames = fieldnames(props.parameters);
    inputParamsNames = fieldnames(params);
    for ii = 1:numel(inputParamsNames)
        param = inputParamsNames{ii};
        if ~any(ismember(solverParamsNames, param)) ...
                && ~(numel(param) >= 2 && isequal(param(1:2), 'x_'))
            error('sapi:InvalidParameter', ...
                '''%s'' is not a valid solver parameter', ...
                inputParamsNames{ii});
        end
    end
end

end
