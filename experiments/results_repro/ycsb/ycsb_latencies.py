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
workloads = ['a', 'b']
code = sys.argv[1]
run_ids =[ '1']
d = sys.argv[2]
base_dir = d + "/{0}.vr.{1}.data.no.3.yes.0.{2}.{3}.dir/"
op_types = ['all', 'nilextwrite', 'read', 'nonnilextwrite']
interested = ['avg', '50.0', '90.0', '95.0', '99.0']
for workload in workloads:
	all_dict = {}
	all_dict['all'] = defaultdict(list)
	all_dict['nilextwrite'] = defaultdict(list)
	all_dict['read'] = defaultdict(list)
	all_dict['nonnilextwrite'] = defaultdict(list)
	for run_id in run_ids:
		resfile = base_dir.format(workload, code, client, run_id)
		out1,err = invoke_cmd('./parse.py ' + str(resfile))
		for op_type in op_types:
			out = out1.split('\n')
			for line in out:
				if len(line) <= 1:
					continue
				if op_type not in line:
					continue
				metric = line.split('\t')[1]
				value = float(line.split('\t')[-1])
				#print workload,run_id,op_type,metric,value
				all_dict[op_type][metric].append(value)

	for op_type in op_types:

		for metric in interested:
                        if len(all_dict[op_type][metric])==0:
                            continue
			print workload, op_type, metric, mean(all_dict[op_type][metric]), all_dict[op_type][metric]

