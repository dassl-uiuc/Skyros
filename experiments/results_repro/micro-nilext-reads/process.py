#!/usr/bin/env python

import os
import sys
import numpy
from collections import defaultdict

perf_dict = defaultdict(dict)
dist = sys.argv[1]
codes = ['orig', 'rtop']
operations = ['all']
metrics = ['avg', '99.0']
outdir = './'
workloads = ['nw0.1.', 'nw0.3.', 'nw0.5.', 'nw0.7.', 'nw0.9.']

workload_name = {}
workload_name['nw0.1.'] = '10'
workload_name['nw0.3.'] = '30'
workload_name['nw0.5.'] = '50'
workload_name['nw0.7.'] = '70'
workload_name['nw0.9.'] = '90'
for metric_pos in range(0, len(metrics)):

	metric = metrics[metric_pos]

	perf_dict =  defaultdict(dict)
	for code in codes:
		filename = code + '_' + dist
		with open(filename, 'r') as f:
			line_number = 0 
			for line in f:
				line = line.replace('\n', '')
				line_split_tmp = line.split(' ')
				if len(line_split_tmp) == 1:
					continue
				workload = line_split_tmp[0]
				if line_split_tmp[1] != 'all':
					continue
				if metric != line_split_tmp[2]:
					continue
				perf_dict[code][workload] = line_split_tmp[3]


	#print perf_dict
	outfile = metric + '-' + dist
	f = open(outfile, 'w')
	for workload in workloads:
		print str(workload_name[workload]) + '\t' + str(metric),
		f.write(str(workload_name[workload]) + '\t' + str(metric))
		for i in range (0, len(codes)):
			code = codes[i]
			latency_system  = float(perf_dict[code][workload])/1000.0
			latency_baseline = float(perf_dict['orig'][workload]) /1000.0
			normalized = round(latency_baseline/latency_system,1)
			print '\t' + str(normalized) + '\t' + str(latency_system),
			f.write('\t' + str(normalized) + '\t' + str(latency_system))
		print ''
		f.write('\n')
	f.close()