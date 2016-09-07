#!/bin/bash

tar jcf ../asa_1.0.0.orig.tar.bz2 --exclude=".git" --exclude="rpm" --exclude="qt_code_style.sh" --exclude="schematic" --exclude="run_debug_version.bat" --exclude=".gitignore" --exclude="prepare_deb_package.sh" ./
# dpkg-source -b ./

#        sudo apt-get install devscripts dpkg-dev fakeroot
#        dpkg-source -x xxx.dsc
#        sudo apt-get install build-essential debhelper
#        debuild -b -uc -us
#        sudo dpkg -i ../asa.deb
