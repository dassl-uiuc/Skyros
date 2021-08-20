#!/usr/bin/env python
import os
import sys
import subprocess
import time
import copy
import random
import threading

sys.path.append('..')

import update_sources_common

desired_branch = 'rocksdb'

machs = []
with open('../../external_ips', 'r') as ip_reader:
	for line in ip_reader:
		line = line.replace('\n','').replace('\t','')
		machs.append(line)

def build_rocks(src_machine):
	update_sources_common.run_remote(src_machine, "cd {0}; git fetch; git checkout {1}; git pull origin {1}".format(update_sources_common.repo_base, desired_branch))
	update_sources_common.run_remote(src_machine, "cd {0}/src/deps/rocksdb-6.12.7/; make static_lib -j16".format(update_sources_common.repo_base))
	
update_sources_common.parallel_start_and_join([threading.Thread(target=build_rocks, args=(str(m),)) for m in machs])
update_sources_common.main(desired_branch)
