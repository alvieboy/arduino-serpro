#!/bin/sh

aclocal
libtoolize --force
automake -a
autoconf
echo
echo You should run ./configure now
echo
