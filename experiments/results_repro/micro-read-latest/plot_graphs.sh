#! /bin/bash

./calc.py rtop . 100 > 100
./calc.py rtop . 200 > 200
./calc.py rtop . 1000 > 1000
./calc_orig.py orig .  > orig
./plot.sh
