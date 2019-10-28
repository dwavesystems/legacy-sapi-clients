# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import json
import sys
from os.path import join, dirname, realpath

with open(join(dirname(realpath(__file__)), '..', 'version.txt')) as f:
    VERSION = f.readline().strip()

INSTALL_DIR = '/usr/share/qubist-clients'

PRETTY_LANG = {'python': 'Python', 'matlab': 'MATLAB', 'c': 'C'}
PRETTY_PLAT = {'linux64': 'Linux', 'osx': 'OSX', 'win32': '32-bit Windows',
               'win64': '64-bit Windows'}


def main():
    clients = []
    for lang in ('python', 'matlab', 'c'):
        for plat in ('linux64', 'osx', 'win32', 'win64'):
            if plat.startswith('win'):
                ext = 'zip'
                mimetype = 'application/zip'
            else:
                ext = 'tar.gz'
                mimetype = 'application/x-tar-gz'
            filename = 'sapi-{lang}-client-{ver}-{plat}.{ext}'.format(
                lang=lang, ver=VERSION, plat=plat, ext=ext)
            clients.append({
                'name': '{lang} Pack for {plat}'.format(
                    lang=PRETTY_LANG[lang], plat=PRETTY_PLAT[plat]),
                'id': filename,
                'filename': join(INSTALL_DIR, filename),
                'group': '{lang} Developer Packs'.format(
                    lang=PRETTY_LANG[lang]),
                'keywords': 'clients,{lang}'.format(lang=lang),
                'mime_type': mimetype,
            })
    if len(sys.argv) > 1:
        with open(sys.argv[1], 'w') as f:
            json.dump(clients, f, sort_keys=True, indent=4,
                      separators=(',', ': '))
            f.write('\n')
    else:
        print json.dumps(clients, sort_keys=True, indent=4,
                         separators=(',', ': '))


if __name__ == '__main__':
    main()
