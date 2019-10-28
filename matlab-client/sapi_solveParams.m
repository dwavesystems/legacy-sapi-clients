% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function params = sapi_solveParams(args)

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

params = struct;
ii = 1;
while ii <= numel(args)
  arg = args{ii};
  ii = ii + 1;
  if isstruct(arg)
    fs = reshape(fieldnames(arg), 1, []);
    for f = fs
       params.(f{1}) = arg.(f{1});
    end
  else
    value = args{ii};
    ii = ii + 1;
    params.(arg) = value;
  end
end

end
