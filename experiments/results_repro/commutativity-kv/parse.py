#!/usr/bin/env python
import os
import sys
import subprocess
import glob
import numpy as np

latencyfiles = glob.glob(sys.argv[1] + '/latencies.*.raw*')
print(latencyfiles)
interested = [50.0, 90.0, 95.0, 99.0]

def print_percentile(latencies, percentiles, filename):
        op_type = filename.split('.')[0]
        with open(sys.argv[1] + '/'+ filename, 'w') as f:
                if len(latencies) == 0:
                    return
                print(op_type + "\tavg\t"+str(np.average(latencies)))
                vals = np.percentile(latencies, percentiles)
                for p in range(0, len(percentiles)):
                        if percentiles[p] in interested:
                            print(op_type + "\t" + str(percentiles[p]) + "\t" + str(vals[p]))
                        f.write(str(percentiles[p]) + "\t" + str(vals[p]) + "\n")

read_latencies = []
nilext_write_latencies = []
non_nilext_write_latencies = []
all_latencies = []

for lf in latencyfiles:
        with open(lf, 'r') as f:
                for line in f:
                        if line[0] == 'R' or line[0] == 'r':
                                read_latencies.append(int(line[1:]))
                        elif line[0] == 'I' or line[0] == 'i' or line[0] == 'U' or line[0] == 'u':
                                nilext_write_latencies.append(int(line[1:]))
                        elif line[0] == 'E' or line[0] == 'e':
                                non_nilext_write_latencies.append(int(line[1:]))
                        else:
                                assert False
                        all_latencies.append(int(line[1:]))

percentiles = range(1, 10001)
percentiles = [x/100.0 for x in percentiles]
print_percentile(read_latencies,percentiles,'read.lat')
print_percentile(nilext_write_latencies,percentiles,'nilextwrite.lat')
print_percentile(non_nilext_write_latencies,percentiles,'nonnilextwrite.lat')
print_percentile(all_latencies,percentiles,'all.lat')
