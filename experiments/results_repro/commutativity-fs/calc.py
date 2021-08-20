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

client = 4
workload = 'fs'
codes = ['orig', 'curp', 'rtop']
codenames = {}
codenames['orig'] = 'Paxos'
codenames['curp'] = 'Curp'
codenames['rtop'] = 'Skyros'

run_ids =['1']
d = sys.argv[1]
base_file = d + "/{0}.vr.{1}.data.no.3.yes.0.{2}.{3}.dir/{0}.vr.{1}.data.no.3.yes.0.{2}.{3}"

throughput_dict = {}
for code in codes:
	t_put_total = []
	avg_lat_total = []
	for run_id in run_ids:
		resfile = base_file.format(workload, code, client, run_id)
		
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
	print code, mean(t_put_total)/1000.0, mean(avg_lat_total), sorted(t_put_total)#, stdev(t_put_total)/1000.0
	throughput_dict[code] = mean(t_put_total)/1000.0

curx = 12
bar_width = 8

baseline = 'orig'
for code in codes:
	normalized = round(throughput_dict[code]/throughput_dict[baseline],1)
	with open(code, 'w') as f:
		f.write(str(curx) + '\t' + codenames[code] + '\t' + str(normalized) + '\t' +  str(throughput_dict[code]) + '\n')
	curx += (bar_width*2)
