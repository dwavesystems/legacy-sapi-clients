# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from setuptools import setup
from os.path import dirname, join, realpath

def version():
    dir = dirname(realpath(__file__))
    files = (join(dir, 'version.txt'), join(dir, '..', 'version.txt'))
    for verfile in files:
        try:
            with open(verfile) as f:
                return f.readline().strip()
        except IOError as e:
            pass
    raise IOError('No version.txt found in same or parent dirctory')


setup(
    name='dwave_sapi2',
    version=version(),
    description='D-Wave(TM) Quantum Computer Python Client',
    author='D-Wave Systems Inc.',
    author_email='dw1support@dwavesys.com',
    url='http://www.dwavesys.com',
    packages=['dwave_sapi2'],
    install_requires=['sapiremote'],
    tests_require=['pytest', 'flexmock'],
)
