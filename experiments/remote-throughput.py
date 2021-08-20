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
import subprocess
import argparse
from vr import vr

AWS_UBUNTU_USER = 'ubuntu'
AWS_REGION = 'us-east-1'
CURR_DIR = os.path.dirname(os.path.abspath(__file__))
PEM_DIR = CURR_DIR + '/pems'

def target_factory(target_system_name, num_nodes):
	if target_system_name == 'vr':
		return vr(num_nodes)
	else:
		assert False

def invoke_remote_cmd(machine_ip, user, command):
	cmd = 'ssh -i {0}/{1}.pem {2}@{3} \'{4}\''.format(PEM_DIR, AWS_REGION, AWS_UBUNTU_USER, machine_ip, command)
	p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if err is not None and len(err) > 0:
		print("Warning for" +cmd + ":" + str(err))
	return (out, err)

def run_remote(machine_ip, user, command):
	cmd = 'ssh -i {0}/{1}.pem {2}@{3} \'{4}\''.format(PEM_DIR, AWS_REGION, AWS_UBUNTU_USER, machine_ip, command)
	os.system(cmd)

def copy_file_remote(machine_ip, user, from_file_path, to_file_path):
	cmd = 'scp -i {0}/{1}.pem {2} {3}@{4}:{5}'.format(PEM_DIR, AWS_REGION, from_file_path, AWS_UBUNTU_USER, machine_ip, to_file_path)
	os.system(cmd)

def copy_file_from_remote(machine_ip, user, from_file_path, to_file_path):
	cmd = 'scp -i {0}/{1}.pem {3}@{4}:{5} {2}'.format(PEM_DIR, AWS_REGION, to_file_path, AWS_UBUNTU_USER, machine_ip, from_file_path)
	os.system(cmd)

def check_context_sanity(context):
	assert context.target_system_name in ['vr']
	# hack instead of choosing ssd/disk
	assert context.medium in ['data'] 
	assert context.code in ['orig', 'rtop', 'curp', 'rtopcomm']
	assert context.time > 0
	assert context.run >= 0
	assert context.cluster in ['us-east-1']
	assert context.sync in ['yes', 'no']
	assert context.leader_reads in ['yes', 'no']
	assert context.user in ['ubuntu']
	assert context.workload in ['t', 'w', 'a', 'b', 'c', 'd', 'f', 'm', 'e', 'n', 'r', 'fs', 'fsar', 'nilz', 'nilu', 'compat', 'compatconflict', 'nc']
	if context.code == 'orig' and context.sync == 'yes':
		assert context.leader_reads == 'yes'

def get_internal_ips(servers, num_nodes):
	internal_ips = {}
	count = 0
	with open('./internal_ips', 'r') as ip_reader:
		for line in ip_reader:
			line = line.replace('\n','').replace('\t','')
			internal_ips[servers[count]] = line
			count += 1
			if count == num_nodes:
				break
	return internal_ips

def get_server_client_external_ips(servers, num_nodes):
	ips = {}
	count = 0
	client_ip = ''
	with open('./external_ips', 'r') as ip_reader:
		for line in ip_reader:
			line = line.replace('\n','').replace('\t','')
			if count < num_nodes:
				ips[servers[count]] = line
			if count == num_nodes:
				client_ip = line
			count += 1

	assert client_ip != ''
	assert len(ips) == len(servers)
	return (ips, client_ip)

def write_to_output(outfile, is_append, to_write_buf):
	if to_write_buf is None or len(to_write_buf) == 0:
		return

	if not is_append:
		with open(outfile, 'w') as ff:
			ff.write(to_write_buf)
	else:
		with open(outfile, 'a') as ff:
			ff.write(to_write_buf)	

def initialize_output_file(outfile):
	write_to_output(outfile, False, '#num_clients\ttotal_ops\tavg_time_taken\tcalc_ops_per_sec\ttotal_ops_per_sec\n')

def load(context):
	assert context.workload != 'w'
	assert context.workload != 't'
	if context.load_afresh:
		# start the "pre" nodes -- these are things are started before the actual data server processes -- e.g., sentinels in redis
		print('Copying prenode load configs')
		for i in context.servers:
			local_pre_config, remote_pre_config = target_system.get_local_remote_prenode_config_paths(i, context, True)
			if local_pre_config is not None and remote_pre_config is not None:
				copy_file_remote(ips[i], context.user, local_pre_config, remote_pre_config)

		print('Starting the prenodes for load phase')
		for i in context.servers:
			prenode_start_cmd = target_system.start_prenode_command(i, context, True)
			if prenode_start_cmd is not None:
				run_remote(ips[i], context.user, prenode_start_cmd)

		os.system("sleep 3")

		print('Copying load configs')
		for i in context.servers:
			local_load_config, remote_load_config = target_system.get_local_remote_config_paths(i, context, True)
			if local_load_config is not None and remote_load_config is not None:
				copy_file_remote(ips[i], context.user, local_load_config, remote_load_config)

		print('Starting the servers for load phase')
		for i in context.servers:
			run_remote(ips[i], context.user, target_system.start_node_command(i, context, True))

		os.system("sleep 5")
		print('Starting the load phase')
		out,err = invoke_remote_cmd(context.client_ip, context.user, target_system.load_command(context, num_clients))
		err = err.decode(sys.stdout.encoding)
		if err is None or len(err) == 0:
			print('Completed load phase')
		else:
			print('Problem in load phase...Exiting. Err:' + str(err))
			sys.exit(-1)

		os.system("sleep 4")	
		for i in context.servers:
			run_remote(ips[i], context.user, target_system.finish_load_command(i, context))
		
if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--target_system_name', type = str, required = True, help = 'what system?')
	parser.add_argument('--medium', type = str, required = True, help = 'disk or ssd?')
	parser.add_argument('--code', type = str, required = True, help = 'orig or retfromleader?')
	parser.add_argument('--time', type = int, required = True, help = 'time to run in seconds')
	parser.add_argument('--run', type = int, required = True, help = '#run')
	parser.add_argument('--cluster', type = str, required = True, help = 'wisc or utah?')
	parser.add_argument('--sync', type = str, required = True, help = 'yes or no?')
	parser.add_argument('--user', type = str, required = True, help = 'ra or aganesan?')
	parser.add_argument('--load_afresh', action ='store_true', help = 'load afresh?')
	parser.add_argument('--workload', type = str, required = True, help = 'workload')
	parser.add_argument('--sync_rep_factor', type = int, required = True, help = 'no of nodes to replicate synchronously')
	parser.add_argument('--num_nodes', type = int, required = True, help = 'number of nodes')
	parser.add_argument('--system_home_dir', type = str, help = 'leave this blank')
	parser.add_argument('--local_mount_point', type = str, help = 'leave this blank')
	parser.add_argument('--remote_mount_point', type = str, help = 'leave this blank')
	parser.add_argument('--workload_home', type = str, help = 'leave this blank')
	parser.add_argument('--load_home', type = str, help = 'leave this blank')
	parser.add_argument('--internal_ips', type = dict, help = 'leave this blank')
	parser.add_argument('--servers', type = list, help = 'leave this blank')
	parser.add_argument('--client_ip', type = str, help = 'leave this blank')
	parser.add_argument('--result_dir', type = str, help = 'leave this blank')
	parser.add_argument('--client_delay', type = int, default = 0, help = 'client_delay in microseconds')
	parser.add_argument('--num_clients', type = int, default = 8, help = 'no of clients')
	parser.add_argument('--geo', action ='store_true', help = 'geo?')
	parser.add_argument('--leader_reads', type = str, required = True, help = 'yes or no?')
	parser.add_argument('--batch', type = int, default = 64, help = 'batch size for vr?')
	parser.add_argument('--workload_trace_prefix', type = str, default = 'run', help = 'workload_trace_prefix')
	parser.add_argument('--readwindow', type = int, default = 1000, help = 'readwindow')
	parser.add_argument('--windowfrac', type = float, default = 0.5, help = 'windowfrac')

	# change the current working dir
	os.chdir(os.path.dirname(os.path.realpath(__file__)))
	local_dc = True
	context = parser.parse_args()
	check_context_sanity(context)
	

	# initialize target_system object
	target_system = target_factory(context.target_system_name, context.num_nodes)

	# populate constructed values
	context.servers =  [str(x+1) for x in range(context.num_nodes)]
	context.system_home_dir = target_system.home_dir(context) 
	assert context.system_home_dir is not None
	#uppath = lambda _path, n: os.sep.join(_path.split(os.sep)[:-n])
	context.local_mount_point = os.getcwd()
	context.remote_mount_point = os.getcwd()
	context.local_perf_dir = context.local_mount_point
	context.remote_perf_dir = context.remote_mount_point
	context.workload_home = target_system.workload_dir(context)
	context.load_home = target_system.load_dir(context)	
	num_clients = context.num_clients
	
	# initialize locals
	#clients = [1]
	outfile = str(context.workload) + '.' + str(context.target_system_name) + '.' + str(context.code) + '.' + str(context.medium) + '.' + context.sync  + '.' + str(context.sync_rep_factor) + '.' + str(context.leader_reads) + '.' + str(context.client_delay) + '.' + str(num_clients) + '.' + str(context.run)
	if context.batch == 1:
		outfile = str(context.workload) + '.' + str(context.target_system_name) + '.' + str(context.code) + 'nobatch.' + str(context.medium) + '.' + context.sync  + '.' + str(context.sync_rep_factor) + '.' + str(context.leader_reads) + '.' + str(context.client_delay) + '.' + str(num_clients) + '.' + str(context.run)
	
	if context.workload == 'm' or context.workload == 'e' or context.workload == 'n':
		outfile = str(context.workload) + str(context.workload_trace_prefix) + '.' + str(context.target_system_name) + '.' + str(context.code) + '.' + str(context.medium) + '.' + context.sync  + '.' + str(context.sync_rep_factor) + '.' + str(context.leader_reads) + '.' + str(context.client_delay) + '.' + str(num_clients) + '.' + str(context.run)

	if context.workload == 'r':
		outfile = str(context.workload) + '.' + str(context.readwindow) + '.' + str(context.windowfrac) + '.' + str(context.target_system_name) + '.' + str(context.code) + '.' + str(context.medium) + '.' + context.sync  + '.' + str(context.sync_rep_factor) + '.' + str(context.leader_reads) + '.' + str(context.client_delay) + '.' + str(num_clients) + '.' + str(context.run)

	print('-----------------------------------------')
	print(outfile)
	print('-----------------------------------------')

	context.result_dir = outfile + '.dir'
	# this is a management script, not the client; hence, we use external IPs for running commands
	ips, client_ip = get_server_client_external_ips(context.servers, context.num_nodes)
	
	if not context.geo:
		context.internal_ips = get_internal_ips(context.servers, context.num_nodes)
	else:
		# use this for geo-rep we use ext ip as int ips.
		context.internal_ips = ips
		#assert False

	initialize_output_file(outfile)
	context.client_ip = client_ip

	# stop the servers
	print('Stopping the servers')
	for i in context.servers:
		run_remote(ips[i], context.user, target_system.stop_node_command(i, context))
	os.system('sleep 1')

	# reset the servers
	print('Resetting the servers')
	for i in context.servers:
		run_remote(ips[i], context.user, target_system.reset_node_command(i, context))

	# load data if necessary
	if context.workload != 'w' and context.workload != 't':
		load(context)

	# start the "pre" nodes -- these are things are started before the actual data server processes -- e.g., sentinels in redis

	if context.workload == 'w' or context.workload == 't':
		print('Copying prenode configs')
		for i in context.servers:
			local_pre_config, remote_pre_config = target_system.get_local_remote_prenode_config_paths(i, context, False)
			if local_pre_config is not None and remote_pre_config is not None:
				copy_file_remote(ips[i], context.user, local_pre_config, remote_pre_config)

	print('Starting the prenodes')
	for i in context.servers:
		prenode_start_cmd = target_system.start_prenode_command(i, context, False)
		if prenode_start_cmd is not None:
			run_remote(ips[i], context.user, prenode_start_cmd)

	print('Copying configs')
	for i in context.servers:
		local_config, remote_config = target_system.get_local_remote_config_paths(i, context, False)
		if local_config is not None and remote_config is not None:
			copy_file_remote(ips[i], context.user, local_config, remote_config)
	
	if context.target_system_name == 'vr':
		local_config, remote_config = target_system.get_local_remote_config_paths(i, context, False)
		copy_file_remote(client_ip, context.user, local_config, remote_config)

	if context.target_system_name == 'vr':
		local_config, remote_config = target_system.get_local_remote_client_config_paths(i, context, False)
		for i in context.servers:
			if local_config is not None and remote_config is not None:
				copy_file_remote(ips[i], context.user, local_config, remote_config)
		copy_file_remote(client_ip, context.user, local_config, remote_config)

	print('Starting the servers')
	for i in context.servers:
		run_remote(ips[i], context.user, target_system.start_node_command(i, context, False))
		if i == '1':
			os.system("sleep 5")

	os.system("sleep 5")
	
	# invoke the workload command from the client
	#have to change the interface correctly!
	if context.client_delay > 0:
		print('Will delay client')
		out,err = invoke_remote_cmd(context.client_ip, context.user, "sudo tc qdisc add dev eth2 root netem delay " + str(context.client_delay) + "us")

	if context.target_system_name == 'vr':
		out,err = invoke_remote_cmd(context.client_ip, context.user, "killall client")

	print('Starting performance run')	
	print(target_system.workload_command(context, num_clients))
	out,err = invoke_remote_cmd(context.client_ip, context.user, target_system.workload_command(context, num_clients))
	print('Completed performance run')

	# Python3 messes with some strings
	write_to_output(outfile, True, out.decode(sys.stdout.encoding))
	os.system('sleep 1')

	if context.client_delay > 0:
		print('Will remove the delay from client')
		out,err = invoke_remote_cmd(context.client_ip, context.user, "sudo tc qdisc del dev eth2 root netem")
	
	if context.target_system_name == 'vr':
		for i in context.servers:
			copy_file_from_remote(ips[i], context.user, "/tmp/vrlog", "./vrlog.{0}".format(i))
		command = 'rm -rf {0}; mkdir {0}; mv {1} {0}; mv ./vrlog* {0}; mv {2}/latencies.*.raw {0}'.format(context.result_dir, outfile, context.remote_perf_dir)
		os.system(command)

	for i in context.servers:
		# stop the servers
		run_remote(ips[i], context.user, target_system.stop_node_command(i, context))

		# invoke any postprocessing commands
		ppc = target_system.postprocessing_command(i, context)
		if ppc is not None:
			out,err = invoke_remote_cmd(ips[i], context.user, ppc)
			write_to_output(outfile, True, out.decode(sys.stdout.encoding))

	cmd = target_system.client_postprocessing_command(i, context)
	if cmd is not None:
		os.system(cmd + '; mv {0} {1}'.format(outfile, context.result_dir))
