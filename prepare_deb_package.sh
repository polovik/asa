#!/bin/bash

tar zcf ../asa_1.0.0.orig.tar.gz --exclude="debian" --exclude=".git" --exclude="rpm" --exclude="qt_code_style.sh" --exclude="schematic" --exclude="run_debug_version.bat" --exclude=".gitignore" --exclude="prepare_deb_package.sh" --exclude="*.pro.user" ./
dpkg-source --diff-ignore="(\.git|schematic|debian|rpm|qt_code_style\.sh|run_debug_version\.bat|prepare_deb_package\.sh|.*pro\.user)" -b ./

sed -i -e '/^.*debian.tar.gz$/d' ../asa_1.0.0.dsc
cp ../asa_1.0.0.dsc ../home:polovik/asa/asa.dsc
cp debian/control ../home:polovik/asa/debian.control
cp debian/rules ../home:polovik/asa/debian.rules
cp debian/changelog ../home:polovik/asa/debian.changelog
cp debian/compat ../home:polovik/asa/debian.compat
cp ../asa_1.0.0.orig.tar.gz ../home:polovik/asa/
#        sudo apt-get install devscripts dpkg-dev fakeroot
#        dpkg-source -x xxx.dsc
#        sudo apt-get install build-essential debhelper
#        debuild -b -uc -us
#        sudo dpkg -i ../asa.deb
