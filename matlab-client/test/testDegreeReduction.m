% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testDegreeReduction %#ok<STOUT,*DEFNU>
initTestSuite
end



function testMakeQuadratic

% f's length should be power of 2
f = [];
assertExceptionThrown(@()makeQuadratic(f), '');

% first elemetn in f should be 0
f = [1; 2];
assertExceptionThrown(@()makeQuadratic(f), '');

% f's length should be power of 2
f = [0; 2; 3];
assertExceptionThrown(@()makeQuadratic(f), '');

f = [0; -3; 0; 2; 1; -4; 0; 5; 1; 2; 0; 2; 1; 0; -3; -1];
expectedQ =[ [ -3.0000   15.0000   -1.0000    2.0000  -25.0000         0];
             [ 15.0000         0   -0.5000   -0.5000  -25.0000   -1.0000];
             [ -1.0000   -0.5000    1.0000   12.0000    2.5000  -25.0000];
             [  2.0000   -0.5000   12.0000    1.0000   -2.0000  -25.0000];
             [-25.0000  -25.0000    2.5000   -2.0000   75.0000   -1.5000];
             [       0   -1.0000  -25.0000  -25.0000   -1.5000   75.0000] ];
expectedObj = {[1], [1 2], [3], [1 3], [2 3], [5 3], [4], [1 4], [2 4], [5 4], [3 4], [6 2], [6 5]};
expectedVars = [[5 6]; [1 3]; [2 4]];
[Q, obj, vars] = makeQuadratic(f);
assertEqual(expectedQ, Q);
assertEqual(expectedObj, obj);
assertEqual(expectedVars, vars);

f = [0; -1; 2; 1; 4; -1; 0; 0; -1; -3; 0; -1; 0; 3; 2; 2];
expectedQ = [ [ -1.0000   22.5000   -2.0000   -0.5000  -45.0000    4.5000];
              [ 22.5000    2.0000   -3.0000   -0.5000  -45.0000    3.5000];
              [ -2.0000   -3.0000    4.0000   21.0000    2.5000  -45.0000];
              [ -0.5000   -0.5000   21.0000   -1.0000    0.5000  -45.0000];
              [-45.0000  -45.0000    2.5000    0.5000  135.0000   -4.5000];
              [  4.5000    3.5000  -45.0000  -45.0000   -4.5000  135.0000] ];
%expectedR =  [-1     2     4    -1    30    30]';
expectedObj = {[1], [2], [3], [1 3], [2 3], [5 3], [4], [1 4], [2 4], [5 4], [3 4], [6 1], [6 2], [6 5]};
expectedVars = [[5 6]; [1 3]; [2 4]];
[Q, obj, vars] = makeQuadratic(f);
assertEqual(expectedQ, Q);
assertEqual(expectedObj, obj);
assertEqual(expectedVars, vars);


f = [0; -3; 0; 2; 1; -4; 0; 5; 1; 2; 0; 2; 1; 0; -3; -1];
numDimOrig = floor(log2(length(f)));
[Q, obj, vars] = makeQuadratic(f);
numDimFinal = size(Q, 1);
xx = zeros(2^numDimFinal, numDimFinal);
energy = zeros(2^numDimFinal, 1);
cnt = 1;
for j = 0 : 2^numDimOrig - 1
    flag = false;
    for k = 0 : 2^(numDimFinal - numDimOrig) - 1
      xx(cnt, 1 : numDimOrig) = dec2bin(j, numDimOrig) - '0';
      xx(cnt, numDimOrig + 1 : end) = dec2bin(k, numDimFinal - numDimOrig) - '0';
      energy(cnt) = xx(cnt, :) * Q * xx(cnt, :)';
      if (~flag)
          lowestEnergy = energy(cnt);
          flag = true;
      elseif energy(cnt) < lowestEnergy
          lowestEnergy = energy(cnt);
      end
      cnt = cnt + 1;
    end
    str = dec2bin(j, numDimOrig);
    str = fliplr(str);
    ind = bin2dec(str);
    assertEqual(lowestEnergy, f(ind + 1));
end


f = [0; -1; 2; 1; 4; -1; 0; 0; -1; -3; 0; -1; 0; 3; 2; 2];
numDimOrig = floor(log2(length(f)));
[Q, obj, vars] = makeQuadratic(f);
numDimFinal = size(Q, 1);
xx = zeros(2^numDimFinal, numDimFinal);
energy = zeros(2^numDimFinal, 1);
cnt = 1;
for j = 0 : 2^numDimOrig - 1
    flag = false;
    for k = 0 : 2^(numDimFinal - numDimOrig) - 1
      xx(cnt, 1 : numDimOrig) = dec2bin(j, numDimOrig) - '0';
      xx(cnt, numDimOrig + 1 : end) = dec2bin(k, numDimFinal - numDimOrig) - '0';
      energy(cnt) = xx(cnt, :) * Q * xx(cnt, :)';
      if (~flag)
          lowestEnergy = energy(cnt);
          flag = true;
      elseif energy(cnt) < lowestEnergy
          lowestEnergy = energy(cnt);
      end
      cnt = cnt + 1;
    end
    str = dec2bin(j, numDimOrig);
    str = fliplr(str);
    ind = bin2dec(str);
    assertEqual(lowestEnergy, f(ind + 1));
end

end



function testReduceDegree

terms = {};
[new_terms vars_rep] = reduceDegree(terms);
assertTrue(verifyTermsVars(vars_rep, terms, new_terms));

terms = cell(1, 2);
terms{1} = [1];
terms{2} = [2];
[new_terms vars_rep] = reduceDegree(terms);
assertTrue(verifyTermsVars(vars_rep, terms, new_terms));

terms = cell(1, 2);
terms{1} = [1 2];
terms{2} = [3 4];
[new_terms vars_rep] = reduceDegree(terms);
assertTrue(verifyTermsVars(vars_rep, terms, new_terms));


terms = cell(1, 6);
terms{1} = [2 3 4 5 8];
terms{2} = [3 6 8];
terms{3} = [1 6 7 8];
terms{4} = [2 3 5 6 7];
terms{5} = [1 3 6];
terms{6} = [1 6 8 10 12];
[new_terms vars_rep] = reduceDegree(terms);
assertTrue(verifyTermsVars(vars_rep, terms, new_terms));

terms = cell(1, 3);
terms{1} = [2 3 4 5 8];
terms{2} = [3 6 8];
terms{3} = [200 1 6 7];
[new_terms vars_rep] = reduceDegree(terms);
assertTrue(verifyTermsVars(vars_rep, terms, new_terms));

terms = cell(1, 1);
terms{1} = [1 : 200];
[new_terms vars_rep] = reduceDegree(terms);
assertTrue(verifyTermsVars(vars_rep, terms, new_terms));

end



function ret = verifyTermsVars(vars, originalTerm, newTerm)

while true
   flag = true;
   for i=1 : size(newTerm, 2)
       tmp = [];
       for j=1 : size(newTerm{i}, 2)
           if size(vars, 1) > 0 && size(vars, 2) > 0 && newTerm{i}(j) >= vars(1, 1)
               flag = false;
               for k = 1 : size(vars, 2)
                   if newTerm{i}(j) == vars(1, k)
                       tmp = [tmp vars(2, k) vars(3, k)];
                   end
               end
           else
               tmp = [tmp newTerm{i}(j)];
           end
       end
       newTerm{i} = tmp;
   end

   if flag
       break;
   end
end

if size(originalTerm, 2) ~= size(newTerm, 2)
    ret = false;
    return;
end

for i=1:size(newTerm, 2)
    if size(originalTerm{i}, 2) ~= size(newTerm{i}, 2)
        ret = false;
        return;
    end
    tmp = (sort(originalTerm{i}) == sort(newTerm{i}));

    if sum(tmp) ~= size(originalTerm{i}, 2)
        ret = false;
        return;
    end
end

ret = true;

end
