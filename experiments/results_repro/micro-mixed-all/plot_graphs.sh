#!/bin/bash

./calcexp2.py orig . |cut -d ' ' -f 1,2 > orig
./calcexp2.py rtop . |cut -d ' ' -f 1,2 > rtop
./compare.py > output
./plot.sh