#!/usr/bin/env python

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
import time
import copy
import random
import threading

AWS_UBUNTU_USER = 'ubuntu'
AWS_REGION = 'us-east-1'
PEM_DIR = '../../pems'

machines = []

repo_base = '/mnt/base/skyros'
desired_branch = 'master'

def run_remote(machine_ip, command):
	cmd = 'ssh -i {0}/{1}.pem {2}@{3} \'{4}\''.format(PEM_DIR, AWS_REGION, AWS_UBUNTU_USER, machine_ip, command)
	os.system(cmd)

def invoke_remote_cmd(machine_ip, command):
	cmd = 'ssh -i {0}/{1}.pem {2}@{3} \'{4}\''.format(PEM_DIR, AWS_REGION, AWS_UBUNTU_USER, machine_ip, command)
	p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if err is not None and len(err) > 0:
		print("Warning for" +cmd + ":" + str(err))
	return (out, err)

def parallel_start_and_join(threads):
	map(lambda t: t.start(), threads)
	map(lambda t: t.join(), threads)

def update_machine(src_machine, desired_branch):
	run_remote(src_machine, "cd {0}/experiments; sudo bash udp_parms".format(repo_base))
	run_remote(src_machine, "cd {0}; git fetch; git checkout {1}; git pull origin {1}".format(repo_base, desired_branch))
	run_remote(src_machine, "cd {0}/src/paxos;  make -j16;".format(repo_base))
	run_remote(src_machine, "cd {0}/src/skyros; make -j16;".format(repo_base))

	if desired_branch == 'fs':
		run_remote(src_machine, "cd {0}/src/curp; make -j16;".format(repo_base))

	if desired_branch == 'commkv':
		run_remote(src_machine, "cd {0}/src/curp; make -j16;".format(repo_base))
		run_remote(src_machine, "cd {0}/src/skyroscomm; make -j16;".format(repo_base))

	o,e = invoke_remote_cmd(src_machine, "cd {0}; git status | grep \"On branch\";".format(repo_base))
        o = o.strip().replace('\n', '')
        expected = "On branch {0}".format(desired_branch)
        assert o == expected
        
def main(desired_branch):
	with open('../../external_ips', 'r') as ip_reader:
		for line in ip_reader:
			line = line.replace('\n','').replace('\t','')
			machines.append(line)

	print machines
	parallel_start_and_join([threading.Thread(target=update_machine, args=(str(m), str(desired_branch))) for m in machines])
