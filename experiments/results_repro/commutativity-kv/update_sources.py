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

desired_branch = 'commkv'
update_sources_common.main(desired_branch)
