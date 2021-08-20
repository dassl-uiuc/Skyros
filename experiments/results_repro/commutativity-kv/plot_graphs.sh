#! /bin/bash

#for bar (throughput) graphs
./calc.py .
./draw.py .

#for cdf graphs
./latencies.py orig .
./latencies.py curp .
./latencies.py rtop .

./transform.py a.vr.orig.data.no.3.yes.0.10.1.dir/read.lat > orig_read
./transform.py a.vr.orig.data.no.3.yes.0.10.1.dir/nilextwrite.lat > orig_write


./transform.py a.vr.curp.data.no.3.yes.0.10.1.dir/read.lat > curp_read
./transform.py a.vr.curp.data.no.3.yes.0.10.1.dir/nilextwrite.lat > curp_write

./transform.py a.vr.rtop.data.no.3.yes.0.10.1.dir/read.lat > rtop_read
./transform.py a.vr.rtop.data.no.3.yes.0.10.1.dir/nilextwrite.lat > rtop_write

./plot-a-read.sh
./plot-a-write.sh


