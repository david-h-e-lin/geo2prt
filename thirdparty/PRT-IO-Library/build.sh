#!/bin/sh

g++ -O2 -fPIC -I. -I$HT/include/zlib -I$HT/include/OpenEXR -L$HFS/dsolib example.cpp -lHalf -lz
