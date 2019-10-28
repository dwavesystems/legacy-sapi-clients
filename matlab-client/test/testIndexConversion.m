% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testIndexConversion %#ok<STOUT,*DEFNU>
initTestSuite
end



function testChimeraToLinearIndex

% valid input - case 1
M = 4;
N = 4;
L = 4;

% seven parameters provided
cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                ind = chimeraToLinearIndex(i, j, u, k, M, N, L);
                assertEqual(ind, cnt);
                cnt = cnt + 1;
            end
        end
    end
end

% four parameters provided
ii = zeros(1, M * N * L * 2);
jj = zeros(1, M * N * L * 2);
uu = zeros(1, M * N * L * 2);
kk = zeros(1, M * N * L * 2);
cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                ii(cnt) = i;
                jj(cnt) = j;
                uu(cnt) = u;
                kk(cnt) = k;
                cnt = cnt + 1;
            end
        end
    end
end

ind = chimeraToLinearIndex([ii' jj' uu' kk'], M, N, L);

for i = 1 : M * N * L * 2
    assertEqual(ind(i), i);
end

% valid input - case 2
M = 16;
N = 18;
L = 4;

% seven parameters provided
cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                ind = chimeraToLinearIndex(i, j, u, k, M, N, L);
                assertEqual(ind, cnt);
                cnt = cnt + 1;
            end
        end
    end
end

% four parameters provided
ii = zeros(1, M * N * L * 2);
jj = zeros(1, M * N * L * 2);
uu = zeros(1, M * N * L * 2);
kk = zeros(1, M * N * L * 2);
cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                ii(cnt) = i;
                jj(cnt) = j;
                uu(cnt) = u;
                kk(cnt) = k;
                cnt = cnt + 1;
            end
        end
    end
end

ind = chimeraToLinearIndex([ii' jj' uu' kk'], M, N, L);

for i = 1 : M * N * L * 2
    assertEqual(ind(i), i);
end

% valid input - case 3
M = 18;
N = 16;
L = 4;

% seven parameters provided
cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                ind = chimeraToLinearIndex(i, j, u, k, M, N, L);
                assertEqual(ind, cnt);
                cnt = cnt + 1;
            end
        end
    end
end

% four parameters provided
ii = zeros(1, M * N * L * 2);
jj = zeros(1, M * N * L * 2);
uu = zeros(1, M * N * L * 2);
kk = zeros(1, M * N * L * 2);
cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                ii(cnt) = i;
                jj(cnt) = j;
                uu(cnt) = u;
                kk(cnt) = k;
                cnt = cnt + 1;
            end
        end
    end
end

ind = chimeraToLinearIndex([ii' jj' uu' kk'], M, N, L);

for i = 1 : M * N * L * 2
    assertEqual(ind(i), i);
end

% i out of range
assertExceptionThrown(@()chimeraToLinearIndex(0, 1, 1, 1, M, N, L), '');

% j out of range
assertExceptionThrown(@()chimeraToLinearIndex(1, N + 1, 1, 1, M, N, L), '');

% u out of range
assertExceptionThrown(@()chimeraToLinearIndex(M, N, 3, L, M, N, L), '');

% k out of range
assertExceptionThrown(@()chimeraToLinearIndex(M, N, 2, 0, M, N, L), '');

% M is non-positive
assertExceptionThrown(@()chimeraToLinearIndex(1, 1, 1, 1, -1, N, L), '');
assertExceptionThrown(@()chimeraToLinearIndex(1, 1, 1, 1, 0, N, L), '');

% N is non-positive
assertExceptionThrown(@()chimeraToLinearIndex(1, 1, 1, 1, M, -1, L), '');
assertExceptionThrown(@()chimeraToLinearIndex(1, 1, 1, 1, M, 0, L), '');

% L is non-positive
assertExceptionThrown(@()chimeraToLinearIndex(1, 1, 1, 1, M, N, -1), '');
assertExceptionThrown(@()chimeraToLinearIndex(1, 1, 1, 1, M, N, 0), '');

end



function testLinearToChimeraIndex

% valid input - case 1
M = 4;
N = 4;
L = 4;

cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                [ii jj uu kk] = linearToChimeraIndex(cnt, M, N, L);
                assertEqual(i, ii);
                assertEqual(j, jj);
                assertEqual(u, uu);
                assertEqual(k, kk);
                cnt = cnt + 1;
            end
        end
    end
end

% valid input - case 2
M = 16;
N = 18;
L = 4;

cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                [ii jj uu kk] = linearToChimeraIndex(cnt, M, N, L);
                assertEqual(i, ii);
                assertEqual(j, jj);
                assertEqual(u, uu);
                assertEqual(k, kk);
                cnt = cnt + 1;
            end
        end
    end
end

% valid input - case 3
M = 18;
N = 16;
L = 4;

cnt = 1;
for i = 1 : M
    for j = 1 : N
        for u = 1 : 2
            for k = 1 : L
                [ii jj uu kk] = linearToChimeraIndex(cnt, M, N, L);
                assertEqual(i, ii);
                assertEqual(j, jj);
                assertEqual(u, uu);
                assertEqual(k, kk);
                cnt = cnt + 1;
            end
        end
    end
end


% lin out of range
assertExceptionThrown(@()linearToChimeraIndex(0, M, N, L), '');

% lin out of range
assertExceptionThrown(@()linearToChimeraIndex(M * N * L * 2 + 1, M, N, L), '');

% M is non-positive
assertExceptionThrown(@()linearToChimeraIndex(1, -1, N, L), '');
assertExceptionThrown(@()linearToChimeraIndex(1, 0, N, L), '');

% N is non-positive
assertExceptionThrown(@()linearToChimeraIndex(1, M, -1, L), '');
assertExceptionThrown(@()linearToChimeraIndex(1, M, 0, L), '');

% L is non-positive
assertExceptionThrown(@()linearToChimeraIndex(1, M, N, -1), '');
assertExceptionThrown(@()linearToChimeraIndex(1, M, N, 0), '');

end
