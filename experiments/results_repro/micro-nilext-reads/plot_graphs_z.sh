#!/bin/bash

./latencies.py orig . zipfian > orig_zipfian
./latencies.py rtop . zipfian > rtop_zipfian
./process.py zipfian
./plot_zipfian.sh