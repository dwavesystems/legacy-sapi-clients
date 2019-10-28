% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testIsingQuboConversion %#ok<STOUT,*DEFNU>
initTestSuite
end



function testIsingToQuboEmpty

[Q, offset] = isingToQubo([], []);
assertEqual(Q, [])
assertEqual(offset, 0)

[Q, offset] = isingToQubo([], zeros(0, 5));
assertEqual(Q, [])
assertEqual(offset, 0)

[Q, offset] = isingToQubo([], sparse([]));
assertEqual(Q, [])
assertEqual(offset, 0)

[Q, offset] = isingToQubo(sparse([]), sparse([]));
assertEqual(Q, [])
assertEqual(offset, 0)

end



function testIsingToQubo

h = [-4  3  1  4  0  0  3 -2];
J = [ 0  2  4  3  7  1 -8  0
      4  0  0  0  0  0  8 -2
     -6 -1  0  0 -8  0 -4 -4
     -2 -4  0  0  3  0  0 -6
      0  0 -3  0  0  4  0  0
     -8  0 -7  1  8  0 -5 -2
     -3  0 -6 -4  0  0  0  0
      3  1 -5  0 -1 -7  0  0];

expectedQ = [ -2   8  16  12  28   4 -32   0
              16 -10   0   0   0   0  32  -8
             -24  -4  82   0 -32   0 -16 -16
              -8 -16   0  26  12   0   0 -24
               0   0 -12   0 -20  16   0   0
             -32   0 -28   4  32  30 -20  -8
             -12   0 -24 -16   0   0  50   0
              12   4 -20   0  -4 -28   0  42];
expectedOffset = -52;

[Q, offset] = isingToQubo(h, J);
assertEqual(Q, expectedQ);
assertEqual(offset, expectedOffset)

[Q, offset] = isingToQubo(sparse(h), J);
assertEqual(Q, expectedQ);
assertEqual(offset, expectedOffset)

[Q, offset] = isingToQubo(h, sparse(J));
assertEqual(Q, sparse(expectedQ));
assertEqual(offset, expectedOffset)

[Q, offset] = isingToQubo(sparse(h), sparse(J));
assertEqual(Q, sparse(expectedQ));
assertEqual(offset, expectedOffset)

end



function testIsingToQuboNonsquare

expectedQ = [-2 0 4; 0 0 0; 0 0 -2];
[Q, offset] = isingToQubo([], [0 0 1]);
assertEqual(Q, expectedQ)
assertEqual(offset, 1)

end



function testQuboToIsingEmpty

[h, J, offset] = quboToIsing([]);
assertEqual(h, [])
assertEqual(J, [])
assertEqual(offset, 0)

[h, J, offset] = quboToIsing(sparse([]));
assertEqual(h, [])
assertEqual(J, [])
assertEqual(offset, 0)

[h, J, offset] = quboToIsing(zeros(2, 0));
assertEqual(h, [])
assertEqual(J, [])
assertEqual(offset, 0)

end



function testQuboToIsing

Q = [ 1 -1  1 -7  1  0  7
      7 -4 -2  0  0 -6  0
      0  0 -5  3 -8  4 -5
     -2  0 -5 -6  5  1  0
      0  0  0  0  5 -4  3
     -4  0 -1 -5 -1  0  5
      0  0  4  2 -8  4  0];

expectedH = [1.00 -2.50 -4.75 -5.00 -0.50 -1.75 3.00]';
expectedJ = [ 0.00 -0.25  0.25 -1.75  0.25  0.00  1.75
              1.75  0.00 -0.50  0.00  0.00 -1.50  0.00
              0.00  0.00  0.00  0.75 -2.00  1.00 -1.25
             -0.50  0.00 -1.25  0.00  1.25  0.25  0.00
              0.00  0.00  0.00  0.00  0.00 -1.00  0.75
             -1.00  0.00 -0.25 -1.25 -0.25  0.00  1.25
              0.00  0.00  1.00  0.50 -2.00  1.00  0.00];
expectedOffset = -7.5;

[h, J, offset] = quboToIsing(Q);
assertEqual(h, expectedH)
assertEqual(J, expectedJ)
assertEqual(offset, expectedOffset)

[h, J, offset] = quboToIsing(sparse(Q));
assertEqual(h, expectedH)
assertEqual(J, sparse(expectedJ))
assertEqual(offset, expectedOffset)

end



function testQuboToIsingNonsquare

expectedH = [0 -1 -1]';
expectedJ = [0 0 0; 0 0 0; 0 -1 0];
[h, J, offset] = quboToIsing([0 0; 0 0; 0 -4]);
assertEqual(h, expectedH)
assertEqual(J, expectedJ)
assertEqual(offset, -1)

end
