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

from sut import SystemUnderTest
import os
import sys
import math

class vr(SystemUnderTest):
	def __init__(self, num_nodes):
		self.name = "vr"
		self.num_nodes = num_nodes

	# Private
	def get_config_str(self, context, i, is_load, is_client_facing):
		assert len(context.internal_ips) == context.num_nodes
		count = 0
		if is_client_facing:
			base_port = 14000
		else:
			base_port = 13000

		num_faults = math.floor(self.num_nodes/2)
		config_str = '''f {0}\n'''.format(num_faults)
		for s in context.servers:
			ip = context.internal_ips[s]
			port = base_port + (int(s)-1)
			config_str += 'replica {0}:{1}\n'.format(ip, port)
		return config_str

	# SystemUnderTest methods
	def home_dir(self, context):
		CURR_DIR = os.path.dirname(os.path.abspath(__file__))
		REPO_HOME = os.path.dirname(os.path.abspath(CURR_DIR))
		SRC_DIR = os.path.join(REPO_HOME, "src")
		paxos_dir = "paxos"
		skyros_dir = "skyros"
		curp_dir = "curp"
		skyroscomm_dir = "skyroscomm"

		if context.code == 'orig':
			return os.path.join(SRC_DIR, paxos_dir)
		elif context.code == 'rtop':
			return os.path.join(SRC_DIR, skyros_dir)
		elif context.code == 'curp':
			return os.path.join(SRC_DIR, curp_dir)
		elif context.code == 'rtopcomm':
			return os.path.join(SRC_DIR, skyroscomm_dir)
		else:
			assert False

	def workload_dir(self, context):
		#these are not used in vr
		return '/mnt/' + context.medium + '/'

	def load_dir(self, context):
		#these are not used in vr
		return '/run/shm'

	def get_local_remote_prenode_config_paths(self, i, context, is_load):
		return (None, None)

	def start_prenode_command(self, i, context, is_load):
		return None

	def get_local_remote_config_paths(self, i, context, is_load):
		cfg_filename = 'vrconfig'
		local_cfg_file = '{0}/{1}'.format(context.local_perf_dir, cfg_filename)
		remote_cfg_file = '{0}/{1}'.format(context.remote_perf_dir, cfg_filename)

		# delete and recreate
		os.system('rm -rf {0}'.format(local_cfg_file))
		with open(local_cfg_file, 'w') as fh:
			fh.write(self.get_config_str(context, i, is_load, False))

		return (local_cfg_file, remote_cfg_file)
	
	def get_local_remote_client_config_paths(self, i, context, is_load):
		cfg_filename = 'vrconfig.client'
		local_cfg_file = '{0}/{1}'.format(context.local_perf_dir, cfg_filename)
		remote_cfg_file = '{0}/{1}'.format(context.remote_perf_dir, cfg_filename)

		# delete and recreate
		os.system('rm -rf {0}'.format(local_cfg_file))
		with open(local_cfg_file, 'w') as fh:
			fh.write(self.get_config_str(context, i, is_load, True))

		return (local_cfg_file, remote_cfg_file)
	

	def start_node_command(self, i, context, is_load):
		cfg_filename = 'vrconfig'
		remote_command = ''
		batch_size = context.batch
		#if context.num_clients > 64:
                        #batch_size = context.num_clients 
		command = '''{0}/bench/replica -c {1} -b {2} -i {3} -m vr >/tmp/vrlog 2>&1 & '''.format(context.system_home_dir, context.remote_perf_dir + '/' + cfg_filename, batch_size, int(i)-1)			
		remote_command += command 
		return remote_command

	def reset_node_command(self, i, context):
		remote_cfg_file = '{0}/vrconfig'.format(context.remote_perf_dir)

		cmd = "killall -s 9 replica >/dev/null 2>&1; killall -s 9 replica >/dev/null 2>&1;"
		cmd += 'rm -rf /tmp/vrlog;'
		
		cmd += 'sleep 1;'
		cmd += "killall -s 9 replica >/dev/null 2>&1; killall -s 9 replica >/dev/null 2>&1;"
		cmd += 'rm -rf {0};'.format(remote_cfg_file)
		cmd += 'rm -rf /dev/shm/rocks*;'
		return cmd

	def finish_load_command(self, i, context):
		return None
		
	def stop_node_command(self, i, context):
		#rm -rf /tmp/vrlog*
		return "killall -s 9 replica >/dev/null 2>&1; killall -s 9 replica >/dev/null 2>&1;"

	def load_command(self, context, num_clients):
		return None

	def workload_command(self, context, num_clients):
		ops_per_client = 2000000
		assert ops_per_client > 0
		if context.workload == 'm' or context.workload == 'e' or  context.workload == 'n':
			workload_trace_prefix = context.workload_trace_prefix
		else:
			workload_trace_prefix = 'run' + str(context.workload)
		if context.code == 'orig':
			return 'killall -s 9 client; {0}/vrclient.py {1} {2} {3} {4} {5} {6} {7} {8} {9} {10}'.format(context.remote_perf_dir, self.home_dir(context) + '/bench/client', context.remote_perf_dir + '/vrconfig', context.num_clients, ops_per_client, context.workload, context.code, context.time, workload_trace_prefix, context.readwindow, context.windowfrac)
		elif context.code == 'rtop' or context.code == 'curpopt' or context.code == 'rtopcomm':
			#we give the client-visible config here.
			return 'killall -s 9 client; {0}/vrclient.py {1} {2} {3} {4} {5} {6} {7} {8} {9} {10} {11}'.format(context.remote_perf_dir, self.home_dir(context) + '/bench/client', context.remote_perf_dir + '/vrconfig.client', context.num_clients, ops_per_client, context.workload, context.code, context.time, workload_trace_prefix, context.readwindow, context.windowfrac, context.remote_perf_dir + '/vrconfig')
		elif context.code == 'curp':
			#we give the client-visible config here.
			return 'killall -s 9 client; {0}/vrclient.py {1} {2} {3} {4} {5} {6} {7} {8} {9} {10}'.format(context.remote_perf_dir, self.home_dir(context) + '/bench/client', context.remote_perf_dir + '/vrconfig.client', context.num_clients, ops_per_client, context.workload, context.code, context.time, workload_trace_prefix, context.readwindow, context.windowfrac)

		
	def postprocessing_command(self, i, context):
		pass
		#return '''python {0}/parse.py {0}'''.format(context.remote_perf_dir)

	def client_postprocessing_command(self, i, context):
		return None
