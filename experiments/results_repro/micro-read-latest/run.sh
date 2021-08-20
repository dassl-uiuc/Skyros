#!/bin/bash

r_medium='data'
r_time=60 # time in seconds to run each single point in the graph
r_cluster='us-east-1' # aws region
r_user='ubuntu' # aws user
r_system='vr' #VR for all protocol variants including Skyros
workload='r' #read latest workload

# n: numclients -- for this experiment, we need to try different number of clients
# i: iteration -- paper reported average of 3 runs. For reasonable experimental times for AE, we will just do one run
# code: orig - original paxos, rtop - skyros

n=10 # 10 clients for this experiment

# wp: write percentages
# in this experiment, all writes are nilext; remaining are reads
# In the trace file, U: nilext, R: read (you can cat, grep, wc -l to check fractions of different ops)

# initial setup
chmod 0400 ../../pems/$r_cluster.pem

./update_sources.py

pushd .
cd ../../reqserver
make # make the reqserver
popd

# paxos
code=orig
for rw in 100 200 1000; do
for wf in 0.01 0.02 0.05 0.1 0.2 0.5 0.75 1.0; do
for i in 1; do
        ../../remote-throughput.py --medium $r_medium --code $code --time $r_time --run $i --cluster $r_cluster --sync no --user $r_user --workload $workload --num_nodes 5 --target_system_name $r_system --sync_rep_factor 3 --num_clients $n --leader_reads yes --batch 20 --readwindow $rw --windowfrac $wf
	sleep 2
done
done
done

rm -rf ./r*orig*
mv ../../r*orig* .

code=rtop
for rw in 100 200 1000; do
for wf in 0.01 0.02 0.05 0.1 0.2 0.5 0.75 1.0; do
for i in 1; do
        ../../remote-throughput.py --medium $r_medium --code $code --time $r_time --run $i --cluster $r_cluster --sync no --user $r_user --workload $workload --num_nodes 5 --target_system_name $r_system --sync_rep_factor 3 --num_clients $n --leader_reads yes --batch 64 --readwindow $rw --windowfrac $wf
	sleep 2
done
done
done

rm -rf ./r*rtop*
mv ../../r*rtop* .


./plot_graphs.sh

