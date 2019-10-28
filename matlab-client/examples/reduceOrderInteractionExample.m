% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function reduceOrderInteractionExample

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

% f =   x2 * x3 * x4 * x5 * x8
%     + x3 * x6 * x8
%     + x1 * x6 * x7 * x8
%     + x2 * x3 * x5 * x6 * x7
%     + x1 * x3 * x6
%     + x1 * x6 * x8 * x10 * x12

terms = cell(1, 6);
terms{1} = [2 3 4 5 8];
terms{2} = [3 6 8];
terms{3} = [1 6 7 8];
terms{4} = [2 3 5 6 7];
terms{5} = [1 3 6];
terms{6} = [1 6 8 10 12];

% newTerms means terms after using ancillary variables
% varsRep means ancillary variables
[newTerms, varsRep] = reduceDegree(terms);
for i = 1 : length(newTerms)
    fprintf('terms %d:', i);
    disp(newTerms{i});
end
varsRep


% A function f defined over binary variables represented as a
% vector stored in decimal order
%
% f(x4, x3, x2, x1) =   a
%                     + b * x1
%                     + c * x2
%                     + d * x3
%                     + e * x4
%                     + g * x1 * x2
%                     + h * x1 * x3
%                     + i * x1 * x4
%                     + j * x2 * x3
%                     + k * x2 * x4
%                     + l * x3 * x4
%                     + m * x1 * x2 * x3
%                     + n * x1 * x2 * x4
%                     + o * x1 * x3 * x4
%                     + p * x2 * x3 * x4
%                     + q * x1 * x2 * x3 * x4
%
% f(0000) means when x4 = 0, x3 = 0, x2 = 0, x1 = 0, so f(0000) = a
% f(0001) means when x4 = 0, x3 = 0, x2 = 0, x1 = 1, so f(0001) = a + b
% f(0010) means when x4 = 0, x3 = 0, x2 = 1, x1 = 0, so f(0010) = a + c
% etc.

f = [0; -1; 2; 1; 4; -1; 0; 0; -1; -3; 0; -1; 0; 3; 2; 2];
[Q, newTerms, varsRep] = makeQuadratic(f);
Q
for i = 1 : length(newTerms)
    fprintf('terms %d:', i);
    disp(newTerms{i});
end
varsRep

end
