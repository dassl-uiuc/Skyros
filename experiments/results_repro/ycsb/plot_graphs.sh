#!/bin/bash

#for the throughput bar graphs
./calc.py .
./draw_thrpt.py .


#for cdf graphs
./ycsb_latencies.py orig .
./ycsb_latencies.py rtop .

./transform.py a.vr.orig.data.no.3.yes.0.10.1.dir/all.lat > a_orig
./transform.py a.vr.orig.data.no.3.yes.0.10.1.dir/read.lat > a_orig_read
./transform.py b.vr.orig.data.no.3.yes.0.10.1.dir/all.lat > b_orig
./transform.py b.vr.orig.data.no.3.yes.0.10.1.dir/read.lat > b_orig_read

./transform.py a.vr.rtop.data.no.3.yes.0.10.1.dir/all.lat > a_rtop
./transform.py a.vr.rtop.data.no.3.yes.0.10.1.dir/read.lat > a_rtop_read
./transform.py b.vr.rtop.data.no.3.yes.0.10.1.dir/all.lat > b_rtop
./transform.py b.vr.rtop.data.no.3.yes.0.10.1.dir/read.lat > b_rtop_read

./plot-a-read.sh
./plot-a-all.sh
./plot-b-read.sh
./plot-b-all.sh

