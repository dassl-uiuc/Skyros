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


workload = 't'
code = sys.argv[1]
run_ids =[ '1']
clients = []
if code == 'orignobatch':
    clients = [2, 5, 10, 15, 20, 40]
elif code == 'rtop':
    clients= [2, 5, 12, 15, 25, 35, 50, 70, 100]
else:
    assert code == 'orig'
    clients= [2, 5, 12, 25, 35, 40, 50, 70, 100]

d = sys.argv[2]
base_file = d + "/{0}.vr.{1}.data.no.3.yes.0.{2}.{3}.dir/{0}.vr.{1}.data.no.3.yes.0.{2}.{3}"

for c in clients:
	t_put_total = []
	avg_lat_total = []
	for run_id in run_ids:
		if run_id == '5' and code == 'orig' and c == 110:
			continue
		resfile = base_file.format(workload, code, c, run_id)
		
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
                if not c == len(out):
                    print resfile, len(out)
		#print out
		avg_lat = 0.0
		for o in out:
			v = o.split(' ')
			v = list(filter(lambda x: (len(x) > 0), v))
			assert len(v) == 12
			avg_lat += (float(v[-4]))
		avg_lat /= c
		avg_lat /= 1000.0 #ns to us
		tput_sum = (1000*1000*c)/avg_lat
		#print 'Throughput for {0} clients: {1}; Avg Latency: {2}'.format(c, tput_sum, avg_lat) 
		t_put_total.append(tput_sum)
		avg_lat_total.append(avg_lat)
		#print tput_sum,avg_lat,c
		#sys.exit(0)
	#
	print c, mean(t_put_total)/1000.0, mean(avg_lat_total), t_put_total#, stdev(t_put_total)/1000.0
