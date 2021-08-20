#!/bin/bash

./latencies.py orig . uniform > orig_uniform
./latencies.py rtop . uniform > rtop_uniform
./process.py uniform
./plot_uniform.sh