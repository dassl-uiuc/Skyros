#! /bin/bash

./calc.py orig . |cut -d ' ' -f 1,2 > orig
./calc.py rtop . |cut -d ' ' -f 1,2 > rtop
./compare.py > out
./plot_a.sh