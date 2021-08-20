#! /bin/bash
# take the experimental data and calculate throughput latencies
./calcvrtput.py orignobatch . | cut -d' ' -f1-3 > w-orignobatch
./calcvrtput.py orig . | cut -d' ' -f1-3 > w-orig
./calcvrtput.py rtop . | cut -d' ' -f1-3 > w-rtop

# plot the result
./plot_lat_thrpt.sh

