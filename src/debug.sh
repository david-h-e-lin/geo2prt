#!/bin/bash

hcustom -g -s -lz -lHalf -I$HT/include/zlib -I$HT/include/OpenEXR -I../thirdparty/PRT-IO-Library prt2geo.C
