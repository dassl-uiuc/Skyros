#!/usr/bin/env python3

#Copyright (c) 2021 Aishwarya Ganesan and Ramnatthan Alagappan.

#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:

#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.

import os
import sys
import math
import subprocess
import time

#first arg is client binary path
#second is config file path
#third is num clients
#fourth is num operations

def invoke_remote_cmd(machine_ip, pdir, command):
	cmd = 'ssh -i {0}/{1}.pem {2}@{3} \'{4}\''.format(pdir, "us-east-1", "ubuntu", machine_ip, command)
	# print (cmd)
	p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if err is not None and len(err) > 0:
		#print cmd, out
		print("Warning for" +cmd + ":" + str(err))
	return (out, err)


def invoke_cmd(command):
	p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if err is not None and len(err) > 0:
		pass
		#print("Warning for" +cmd + ":" + str(err))
	return (out, err)

client_binary_path = sys.argv[1]
config_file_path = sys.argv[2]
perf_dir = os.path.dirname(os.path.realpath(config_file_path))
num_clients = int(sys.argv[3])
num_ops = int(sys.argv[4])
workload = sys.argv[5]
code = sys.argv[6]
time_run = sys.argv[7]
workload_trace_prefix = sys.argv[8]
readwindow = sys.argv[9]
windowfrac = sys.argv[10]

if code == 'rtop' or code == 'curpopt' or code == 'rtopcomm':
	consensus_config = sys.argv[11]

workload_trace_dir = '/mnt/data/'


if workload == 'w' or workload == 'e' or workload == 'fs' or workload == 'fsar' or workload == 'compat' or workload == 'nc':
	print ('W: Not going to load')

else:
	print ('Going to load')

	load_file_prefix = 'load/load.'
	if workload == 'r':
		load_file_prefix = 'loadr/load.'

	for i in range(1, 9):
		#we don't want the load output when we download the results, so redirecting to dev null
		if code == 'rtop' or code == 'rtopcomm':
			os.system('{0} -c {1} -s {5} -m vr -n {2} -k {3} > /tmp/load.log.{4} 2>&1 &'.format(client_binary_path, config_file_path, 125000, workload_trace_dir + load_file_prefix + str(i), str(i), consensus_config))
		elif code == 'orig':
			os.system('{0} -c {1} -m vr -n {2} -k {3} > /tmp/load.log.{4} 2>&1 &'.format(client_binary_path, config_file_path, 125000, workload_trace_dir + load_file_prefix + str(i), str(i)))
		elif code == 'curp':
			os.system('{0} -c {1} -m vr -n {2} -k {3} > /tmp/load.log.{4} 2>&1 &'.format(client_binary_path, config_file_path, 125000, workload_trace_dir + load_file_prefix + str(i), str(i)))
		else:
			assert False

	out = 'something'
	while out is not None and len(out) != 0:
		out, err = invoke_cmd('ps aux | grep bench | grep client | grep vr | grep -v py')
		time.sleep(2)

	print('Finished loading; sleeping for 5s')

	os.system('sleep 5')

	if code == 'rtop' or code == 'curp' or code == 'rtopcomm':
		#making sure that load completed in the background
		ip = ''
		with open('{0}/external_ips'.format(perf_dir)) as f:
			for line in f:
				ip = line.replace('\n', '')
				break

		times_checked = 0
		out = 'something'
		while '1000000' not in out:
			out, err = invoke_remote_cmd(ip, perf_dir + "/pems", "cat /tmp/vrlog* | grep -i lastcommitted")
			out = out.decode(sys.stdout.encoding)
			time.sleep(2)
			times_checked += 1
			if times_checked > 7:
				print('Load did not complete after checking for 7 times... exiting.')
				sys.exit(0)

		print('Load completed in the background. Seen {0}'.format(out.split('\n')[-2]))

if workload != 'm' and workload != 'e' and workload != 'n':
	workload_file_prefix = workload_trace_dir + 'run' +str(workload) + '/run.' + str(workload) + '.'
else:
		if workload == 'e':
			#/mnt/data/rune/e0/
			#run.e.1
			workload_file_prefix = workload_trace_dir + '/rune/' + workload_trace_prefix + '/' + 'run.e.'
		elif workload == 'm':
			workload_file_prefix = workload_trace_dir + '/exp2-traces/' + workload_trace_prefix + '/' + workload_trace_prefix + '.'
		elif workload == 'n':
			workload_file_prefix = workload_trace_dir + '/2c-traces/' + workload_trace_prefix + '/' + workload_trace_prefix + '.'

os.system("rm -rf {0}/lat.*; rm -rf {0}/latencies.*".format(perf_dir))
if workload == 'r':
	os.system('{0}/reqserver/server {1} {2} > /tmp/serverlog 2>&1  &'.format(perf_dir, str(readwindow), str(windowfrac)))
	os.system('sleep 1')

for i in range(1, num_clients + 1):
	if code == 'rtop' or code == 'curpopt' or code == 'rtopcomm':
		if workload == 'r':			
			os.system('{0} -c {1} -s {7} -m vr -n {2} -k {5} -e {6} -l {4}/latencies.{3} > {4}/lat.{3} 2>&1 &'.format(client_binary_path, config_file_path, num_ops, str(i), perf_dir, "nullfile", time_run, consensus_config))
		else:
			os.system('{0} -c {1} -s {7} -m vr -n {2} -k {5} -e {6} -l {4}/latencies.{3} > {4}/lat.{3} 2>&1 &'.format(client_binary_path, config_file_path, num_ops, str(i), perf_dir, workload_file_prefix + str(i), time_run, consensus_config))
	elif code == 'orig' or code == 'curp':
		if workload == 'r':
			os.system('{0} -c {1} -m vr -n {2} -k {5} -e {6} -l {4}/latencies.{3} > {4}/lat.{3} 2>&1 &'.format(client_binary_path, config_file_path, num_ops, str(i), perf_dir, "nullfile", time_run))
		else:
			os.system('{0} -c {1} -m vr -n {2} -k {5} -e {6} -l {4}/latencies.{3} > {4}/lat.{3} 2>&1 &'.format(client_binary_path, config_file_path, num_ops, str(i), perf_dir, workload_file_prefix + str(i), time_run))
	else:
		assert False

out = 'something'
while out is not None and len(out) != 0:
	out, err = invoke_cmd('ps aux | grep bench | grep client | grep vr | grep -v py')
	time.sleep(2)

for i in range(1, num_clients+1):
	with open('{0}/lat.{1}'.format(perf_dir, i), 'r') as f:
		for line in f:
			print(line.replace('\n', ''))

os.system('killall -s 9 server')