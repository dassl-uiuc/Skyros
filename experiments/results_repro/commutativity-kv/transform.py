#!/usr/bin/env python
import os
import sys
import subprocess
import argparse
from statistics import mean, stdev

filename = sys.argv[1]
with open(filename, 'r') as f:
	for line in f:
		line=line.replace('\n', '')
		line=line.split('\t')
		print line[0], float(line[1])/1000.0