#!/bin/bash

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

results_home=/mnt/base/skyros/experiments/results_repro
pem='./pems/us-east-1.pem'
user=ubuntu
client=$(tail -n 1 ./external_ips) 

scp -i $pem $user@$client:$results_home/micro-nilext-only/micro-nilext-only.eps .
scp -i $pem $user@$client:$results_home/micro-nilext-nonnilext/micro-nilext-nonnilext.eps .
scp -i $pem $user@$client:$results_home/micro-nilext-reads/*.eps .
scp -i $pem $user@$client:$results_home/micro-mixed-all/micro-mixed-all.eps .
scp -i $pem $user@$client:$results_home/ycsb/*.eps .
scp -i $pem $user@$client:$results_home/micro-read-latest/*.eps .
scp -i $pem $user@$client:$results_home/rocksdb/*.eps .
scp -i $pem $user@$client:$results_home/commutativity-kv/*.eps .
scp -i $pem $user@$client:$results_home/commutativity-compat/*.eps .
scp -i $pem $user@$client:$results_home/commutativity-fs/*.eps .