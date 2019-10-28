% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function testCancel
h = answerHandle('incomplete');
assertFalse(sapiremote_done(h))

sapiremote_cancel(h)
assertTrue(sapiremote_done(h))
end
