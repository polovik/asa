#!/bin/sh

tar jcf ../asa_1.0.0.orig.tar.bz2 --exclude=".git" ./
dpkg-source -b ./
