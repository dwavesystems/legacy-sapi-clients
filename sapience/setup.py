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
    name='sapience',
    version=version(),
    description='D-Wave(TM) Quantum Computer remote software solver interface',
    author='D-Wave Systems Inc.',
    author_email='dw1support@dwavesys.com',
    url='http://www.dwavesys.com',
    py_modules=['sapience'],
    install_requires=['sapilocal=={v}'.format(v=version())]
)
