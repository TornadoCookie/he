#!/bin/bash

# $1 = src
# $2 = dest

mkdir -p $2 $2/lib
cp $1/build/*.so $2/lib
cp $1/build/*.a $2/lib
cp -r $1/include $2


