% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function conn = sapiremote_connection(url, token, proxy)
%sapiremote_connection Create a remote SAPI connection.
%
%  conn = sapiremote_connection(url, token)
%  conn = sapiremote_connection(url, token, proxy)
%
%  Input Parameters
%    url: SAPI URL.  Get this from the web user interface (using the menu:
%      Developers > Solver API, "Connecting to SAPI" section).
%    token: API token.  This is also obtained from the web user interface
%      (menu: <user id> > API tokens).
%    proxy: Proxy URL.  By default (i.e. when no value or an empty matrix is
%      provided), MATLAB's proxy settings are used.  If those settings are not
%      available, the environment variables all_proxy, http_proxy, and
%      https_proxy are checked (http_proxy and https_proxy are
%      protocol-specific, all_proxy is not).  The environment variable
%      no_proxy is also examined; it can contain a comma-separated list of
%      hosts for which no proxy server should be used, or an asterisk to
%      override the other environment variables.  If a proxy URL is given,
%      then it is used directly, without checking MATLAB's settings or any
%      environment variable.  In particular, an empty string (i.e. '', which
%      is interpreted differently from []) forces no proxy server to be used.
%
%  Output
%    conn: handle to the remote connection.  Do not rely on its internal
%      structure, it may change in future versions.
%

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if nargin < 3
  proxy = [];
end

% try to use matlab proxy settings
if isempty(proxy) && ~ischar(proxy) && usejava('jvm')
  try
    if verLessThan('matlab', '7.8') % pre-2009a
      proxyHost = java.lang.System.getProperty('http.proxyHost');
      if ~isempty(proxyHost)
        proxy = [char(proxyHost) ':' ...
          char(java.lang.System.getProperty('http.proxyPort'))];
      end
    else % works at least up to 2011a
      mwtcp = com.mathworks.net.transport.MWTransportClientPropertiesFactory.create();
      proxyHost = mwtcp.getProxyHost;
      if ~isempty(proxyHost)
        proxy = [char(proxyHost) ':' char(mwtcp.getProxyPort)];
      end
    end
  catch %#ok<CTCH>
    proxy = [];
  end
end

conn = struct('url', url, 'token', token, 'proxy', proxy);
end
