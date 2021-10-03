#!/bin/bash

VER=1.9.9
make dist VERSION=$VER
cp dict2-${VER}.tar.gz dict2.desktop pixmaps/dict2.xpm /usr/src/redhat/SOURCES/
cp dict2.spec /usr/src/redhat/SPECS/
cd /usr/src/redhat/SPECS
rpmbuild -ba dict2.spec
