% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testJson %#ok<STOUT,*DEFNU>
initTestSuite
end

function testParsePrimitives
assertEqual(jsontest('parse', 'null'), [])
assertEqual(jsontest('parse', 'true'), true);
assertEqual(jsontest('parse', 'false'), false);
assertEqual(jsontest('parse', '0'), 0);
assertEqual(jsontest('parse', '1e100'), 1e100);
assertEqual(jsontest('parse', '-1.5'), -1.5);
assertEqual(jsontest('parse', '""'), '');
assertEqual(jsontest('parse', '"hello"'), 'hello');
end

function testParseHeterogeneousArray
assertEqual(jsontest('parse', '[null, true, 3, "abc"]'), {[], true, 3, 'abc'})
end

function testParseArrayAsMatrix
assertEqual(jsontest('parse', ...
  ['[ [[1,2],[3,4],[5,6]], [[7,8],[9,10],[11,12]],' ...
   '[[13,14],[15,16],[17,18]], [[19,20],[21,22],[23,24]] ]']), ...
  reshape(1:24, [2, 3, 4]))
assertEqual(jsontest('parse', '[[[[[1,2]],[[3,4]]]]]'), reshape(1:4, [2 1 2]))
assertEqual(jsontest('parse', '[[true, true], [false, false]]'), ...
  [true, false; true, false])
end

function testParseObject
assertEqual(jsontest('parse', '{"a": 123, "b": -4}'), ...
  struct('a', 123, 'b', -4))

maxkey = repmat('a', 1, namelengthmax);
assertEqual(...
  jsontest('parse', ['{"1": "one", "hey!": "yo", "' maxkey 'z": null}']), ...
  struct('X1', 'one', 'hey_', 'yo', maxkey, []))
end

function testParseComplex
json = ['{"hello":[1,2,3,null,[[0,0],[0,0]]],' ...
  '"another":["one",["two",["three",[false,false,true]]]],' ...
  '"more":{"a":{"b":{"c":null}}}}'];
expected.hello = {1 2 3 [] zeros(2)};
expected.more.a.b.c = [];
expected.another = {'one' {'two' {'three' [false; false; true]}}};
assertEqual(jsontest('parse', json), expected)
end

function testConvertPrimitives
assertEqual(jsontest('unambiguous', true), true)
assertEqual(jsontest('unambiguous', false), false)
assertEqual(jsontest('unambiguous', 0), 0)
assertEqual(jsontest('unambiguous', 1e100), 1e100)
assertEqual(jsontest('unambiguous', -1.5), -1.5)
assertEqual(jsontest('unambiguous', ''), '')
assertEqual(jsontest('unambiguous', 'hello'), 'hello')
end

function testConvertMatrix
assertEqual(jsontest('unambiguous', [1 3; 2 4]), {{1;2};{3;4}})
assertEqual(jsontest('unambiguous', reshape(1:6, [1 1 3 2])), ...
  {{1;2;3};{4;5;6}})
assertEqual(jsontest('unambiguous', [[1;2],[3;4],[5;6]]), ...
  {{1;2};{3;4};{5;6}})
end

function testConvertSparseMatrix
assertEqual(jsontest('unambiguous', ...
  sparse([1 2 4 2 3 6 1 2 3 4 5 6], [1 1 1 2 4 5 6 6 6 6 6 6], 1:12, 7, 7)), ...
  {{1; 2; 0; 3; 0; 0; 0}; {0; 4; 0; 0; 0; 0; 0}; {0; 0; 0; 0; 0; 0; 0}; ...
   {0; 0; 5; 0; 0; 0; 0}; {0; 0; 0; 0; 0; 6; 0}; {7; 8; 9;10;11;12; 0}; ...
   {0; 0; 0; 0; 0; 0; 0}})
assertEqual(jsontest('unambiguous', logical(speye(3))), ...
  {{true; false; false}; {false; true; false}; {false; false; true}})
end

function testConvertStruct
assertEqual(jsontest('unambiguous', struct([])), cell(0, 2))
assertEqual(jsontest('unambiguous', struct), cell(0, 2))
assertEqual(jsontest('unambiguous', struct('z', 1, 'a', 'a')), ...
  {'a' 'a'; 'z' 1})
assertEqual(jsontest('unambiguous', ...
  struct('x', {1 'one'}, 'y', {2 'two'})), ...
  {{'x' 1; 'y' 2}; {'x' 'one'; 'y' 'two'}})
assertEqual(jsontest('unambiguous', reshape(struct( ...
  'f1', {1, 2, 3, 'a', 'b', 'c'}, ...
  'f2', {true, false, {}, [], struct, struct}), [2, 1, 3])), ...
  {{{'f1', 1; 'f2', true}; {'f1', 2; 'f2', false}}; ...
   {{'f1', 3; 'f2', cell(0, 1)}; {'f1', 'a'; 'f2', cell(0, 1)}}; ...
   {{'f1', 'b'; 'f2', cell(0, 2)}; {'f1', 'c'; 'f2', cell(0, 2)}}})
end

function testConvertCellArray
assertEqual(jsontest('unambiguous', {}), cell(0, 1))
assertEqual(jsontest('unambiguous', {1}), {1})
assertEqual(jsontest('unambiguous', {1,2,3}), {1;2;3})
assertEqual(jsontest('unambiguous', {1;2;3}), {1;2;3})
assertEqual( ...
    jsontest('unambiguous', reshape(num2cell(1:12), [1,3,1,2,1,2,1])), ...
    { {{1;2;3};{4;5;6}}; {{7;8;9};{10;11;12}} })
assertEqual(jsontest('unambiguous', {{{{true, 0, 'hello'}}}}), ...
    {{{{true; 0; 'hello'}}}})
end
