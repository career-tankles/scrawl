#!/bin/sh

cp ../../src/febird/objbox.* ../../src/febird/ptree.* ~/qihoo/cs/trunk/cloudmark/php
mkdir -p ~/qihoo/cs/trunk/cloudmark/php/test
cp *.php load.cpp sample.cpp ~/qihoo/cs/trunk/cloudmark/php/test
sed -i 's/namespace\s\+febird/namespace php/' ~/qihoo/cs/trunk/cloudmark/php/*.{cpp,hpp}
sed -i 's/febird/php/g' ~/qihoo/cs/trunk/cloudmark/php/test/*.cpp

