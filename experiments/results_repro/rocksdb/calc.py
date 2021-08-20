#!/usr/bin/env python
import os
import sys
import subprocess
import argparse
from statistics import mean, stdev
from collections import defaultdict

def invoke_cmd(command):
	p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if err is not None and len(err) > 0:
		pass
		#print("Warning for" +cmd + ":" + str(err))
	return (out, err)

client = 10
workloads = ['w', 'a']
codes = ['orig', 'rtop']
workload_names = {}
workload_names['w'] = 'LOAD'
workload_names['a'] = 'A'
workload_names['b'] = 'B'
workload_names['c'] = 'C'
workload_names['d'] = 'D'
workload_names['f'] = 'F'
run_ids =[ '1']
d = sys.argv[1]
base_file = d + "/{0}.vr.{1}.data.no.3.yes.0.{2}.{3}.dir/{0}.vr.{1}.data.no.3.yes.0.{2}.{3}"
baseline = 'orig'
throughput_dict = defaultdict(dict)
files = {}
for code in codes:
	files[code] = open(code , 'w')
	for workload in workloads:
		t_put_total = []
		avg_lat_total = []
		for run_id in run_ids:
			resfile = base_file.format(workload, code, client, run_id)

			out,err = invoke_cmd('cat {0} | grep Average'.format(resfile))
			out = out.split('\n')
			out = list(filter(lambda x: (len(x) > 0), out))
	                if not client == len(out):
	                    print resfile, len(out)
	                    assert False
			#print out
			avg_lat = 0.0
			for o in out:
				v = o.split(' ')
				v = list(filter(lambda x: (len(x) > 0), v))
				assert len(v) == 12
				avg_lat += (float(v[-4]))
			avg_lat /= client
			avg_lat /= 1000.0 #ns to us
			tput_sum = (1000*1000*client)/avg_lat
			#print 'Throughput for {0} clients: {1}; Avg Latency: {2}'.format(c, tput_sum, avg_lat) 
			t_put_total.append(tput_sum)
			avg_lat_total.append(avg_lat)
			#print tput_sum,avg_lat,c
			#sys.exit(0)
		#

		print code, workload, mean(t_put_total)/1000.0, mean(avg_lat_total), t_put_total#, stdev(t_put_total)/1000.0
		throughput_dict[code][workload] = mean(t_put_total)/1000.0

curx = 16
bar_width = 8

for workload in workloads:
	for code in codes:
		normalized = round(throughput_dict[code][workload]/throughput_dict[baseline][workload],2)
		print(str(curx) + '\t' + workload_names[workload] + '\t' + code + '\t' + str(normalized) + '\t' +  str(throughput_dict[code][workload]))
		files[code].write(str(curx) + '\t' +  workload_names[workload] + '\t' + str(normalized) + '\t' +  str(throughput_dict[code][workload]) + '\n')
		curx += bar_width
	curx += (bar_width)

for code in codes:
	files[code].close()
