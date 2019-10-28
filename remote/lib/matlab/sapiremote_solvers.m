% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function solvers = sapiremote_solvers(conn)
%sapiremote_solvers Retrieve remote SAPI solvers.
%
%  solvers = sapiremote_solvers(conn)
%
%  Input Parameters:
%    conn: remote connection handle obtained from sapiremote_connection.
%
%  Output
%    solvers: a structure array of solvers.  Fields are 'id', containing the
%      solver ID string; and 'properties', a structure of solver properties
%      (fields are solver-dependent).

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

solvers = sapiremote_mex('solvers', conn);
end
