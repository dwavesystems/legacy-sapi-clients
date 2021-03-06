% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testQpAnswer %#ok<STOUT,*DEFNU>
initTestSuite
end

function testEmpty
answer = struct( ...
  'format', 'qp', ...
  'energies', '', ...
  'num_variables', 0, ...
  'active_variables', '', ...
  'num_occurrences', '', ...
  'solutions', '');
assertEqual(sapiremote_decodeqp('ising', answer), ...
  struct('energies', zeros(1, 0), 'solutions', [], 'num_occurrences', zeros(1, 0)))
end

function testIsingHist
answer = struct( ...
  'format', 'qp', ...
  'energies', 'AAAAAAAAJMAAAAAAAAAUwA==', ...
  'num_variables', 5, ...
  'active_variables', 'AQAAAAIAAAAEAAAA', ...
  'solutions', 'AOA=', ...
  'num_occurrences', '6AMAANsDAAA=', ...
  'extra', 1234);
assertEqual(sapiremote_decodeqp('ising', answer), ...
  struct( ...
    'energies', [-10.0, -5.0], ...
    'solutions', [3, -1, -1, 3, -1; 3, 1, 1, 3, 1]', ...
    'num_occurrences', [1000, 987], ...
    'extra', 1234));
end

function testIsingRaw
answer = struct( ...
  'format', 'qp', ...
  'energies', 'AAAAAAAAJMAAAAAAAAAUwA==', ...
  'num_variables', 5, ...
  'active_variables', 'AQAAAAIAAAAEAAAA', ...
  'solutions', 'AOA=', ...
  'extra', 1234);
assertEqual(sapiremote_decodeqp('ising', answer), ...
  struct( ...
    'energies', [-10.0, -5.0], ...
    'solutions', [3, -1, -1, 3, -1; 3, 1, 1, 3, 1]', ...
    'extra', 1234));
end

function testQuboHist
answer = struct( ...
  'format', 'qp', ...
  'energies', 'AAAAAAAAJMAAAAAAAAAUwA==', ...
  'num_variables', 5, ...
  'active_variables', 'AQAAAAIAAAAEAAAA', ...
  'solutions', 'AOA=', ...
  'num_occurrences', '6AMAANsDAAA=', ...
  'extra', 1234);
assertEqual(sapiremote_decodeqp('qubo', answer), ...
  struct( ...
    'energies', [-10.0, -5.0], ...
    'solutions', [3, 0, 0, 3, 0; 3, 1, 1, 3, 1]', ...
    'num_occurrences', [1000, 987], ...
    'extra', 1234));
end

function testQuboRaw
answer = struct( ...
  'format', 'qp', ...
  'energies', 'AAAAAAAAJMAAAAAAAAAUwA==', ...
  'num_variables', 5, ...
  'active_variables', 'AQAAAAIAAAAEAAAA', ...
  'solutions', 'AOA=', ...
  'extra', 1234);
assertEqual(sapiremote_decodeqp('qubo', answer), ...
  struct( ...
    'energies', [-10.0, -5.0], ...
    'solutions', [3, 0, 0, 3, 0; 3, 1, 1, 3, 1]', ...
    'extra', 1234));
end

function testDecodingError
answer.format = 'qp';
assertExceptionThrown(@() sapiremote_decodeqp('qubo', answer), ...
  'sapiremote:Error');
end
