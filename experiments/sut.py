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


from abc import ABC, abstractmethod

class SystemUnderTest(ABC):
	@abstractmethod
	def home_dir(self, context):
		pass

	@abstractmethod
	def workload_dir(self, context):
		pass

	@abstractmethod
	def load_dir(self, context):
		pass

	@abstractmethod
	def get_local_remote_prenode_config_paths(self, i, context, is_load):
		pass

	@abstractmethod
	def start_prenode_command(self, i, context, is_load):
		pass

	@abstractmethod
	def get_local_remote_config_paths(self, i, context, is_load):
		pass
		
	@abstractmethod
	def start_node_command(self, i, context, is_load):
		pass

	@abstractmethod
	def reset_node_command(self, i, context):
		pass

	@abstractmethod
	def finish_load_command(self, i, context):
		pass
		
	@abstractmethod
	def stop_node_command(self, i, context):
		pass

	@abstractmethod
	def load_command(self, context, num_clients):
		pass

	@abstractmethod
	def workload_command(self, context, num_clients):
		pass

	@abstractmethod
	def postprocessing_command(self, i, context):
		pass

	@abstractmethod
	def client_postprocessing_command(self, i, context):
		pass