% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testConnection %#ok<STOUT,*DEFNU>
initTestSuite
end

function testConnectionDefaultProxy
conn = sapiremote_connection('http://example.com/sapi', 'token');
assertEqual(conn.url, 'http://example.com/sapi')
assertEqual(conn.token, 'token')
assertTrue(isempty(conn.proxy) || ischar(conn.proxy))
end

function testConnectionEnvDefaultProxy
conn = sapiremote_connection('http://example.com/sapi', 'token', []);
assertEqual(conn.url, 'http://example.com/sapi')
assertEqual(conn.token, 'token')
assertEqual(conn.proxy, [])
end

function testConnectionProxy
conn = sapiremote_connection('http://example.com/sapi', 'token', 'proxy');
assertEqual(conn.url, 'http://example.com/sapi')
assertEqual(conn.token, 'token')
assertEqual(conn.proxy, 'proxy')
end
