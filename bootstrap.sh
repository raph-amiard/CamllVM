#!/bin/bash

autoheader autoconf/configure.ac && rm -Rf autom4te.cache
cd autoconf
autoconf -o ../configure configure.ac

cd ..
cat lib/config.h.in | grep -iv package > tmp
mv tmp lib/config.h.in

touch configure

### autoreconf -f -i -Wall

