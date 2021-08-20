#!/usr/bin/env python
import os
import sys
import subprocess
import argparse
from statistics import mean, stdev

def invoke_cmd(command):
	p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if err is not None and len(err) > 0:
		pass
		#print("Warning for" +cmd + ":" + str(err))
	return (out, err)

client = 10
workloads = ['0.01', '0.02', '0.05', '0.1', '0.2', '0.5', '0.75', '1.0']
code = sys.argv[1]
run_ids =['1']
d = sys.argv[2]
delay = sys.argv[3]
base_file = d + "/r.{4}.{0}.vr.{1}.data.no.3.yes.0.{2}.{3}.dir/r.{4}.{0}.vr.{1}.data.no.3.yes.0.{2}.{3}"

for workload in workloads:
	t_put_total = []
	avg_lat_total = []
	for run_id in run_ids:
		resfile = base_file.format(workload, code, client, run_id, delay)
		'''out,err = invoke_cmd('cat {0} | grep Completed | grep -v -i warm | grep request'.format(resfile))
		out = out.split('\n')
		out = list(filter(lambda x: (len(x) > 0), out))
		#print out
		assert c == len(out)

		tput_sum = 0.0
		for o in out:
			v = o.split(' ')
			v = list(filter(lambda x: (len(x) > 0), v))
			assert len(v) == 11
			tput_sum += (float(v[-5])/float(v[-2]))'''

		out,err = invoke_cmd('cat {0} | grep Average'.format(resfile))
		out = out.split('\n')
		out = list(filter(lambda x: (len(x) > 0), out))
                if not client == len(out):
                    print resfile, len(out)
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
	print int(float(workload)*100), mean(avg_lat_total)#, t_put_total, stdev(t_put_total)/1000.0
