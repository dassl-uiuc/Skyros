#!/bin/bash


r_medium='data'
r_time=10 # time in seconds to run each single point in the graph
r_cluster='us-east-1' # aws region
r_user='ubuntu' # aws user
r_system='vr' #VR for all protocol variants including Skyros
workload='t' #write-only workload for throughput latency graphs

# n: numclients -- for this experiment, we need to try different number of clients
# i: iteration -- paper reported average of 3 runs. For reasonable experimental times for AE, we will just do one run
# code: orig - original paxos, rtop - skyros


# initial setup
chmod 0400 ../../pems/$r_cluster.pem

./update_sources.py

# paxos (no batch)
for n in 2 5 10 15 20 40; do
for i in 1; do
for code in orig; do
        # set the batch parameter to 1 for no batching (for paxos-nobatch variant)
        ../../remote-throughput.py --medium $r_medium --code $code --time $r_time --run $i --cluster $r_cluster --sync no --user $r_user --workload $workload --num_nodes 5 --target_system_name $r_system --sync_rep_factor 3 --num_clients $n --leader_reads yes --batch 1
        sleep 2
done
done
done

rm -rf ./$workload.$r_system.orignobatch.*
mv ../../$workload.$r_system.orignobatch.* .


# paxos
for n in 2 5 12 25 35 40 50 70 100; do
for i in 1; do
for code in orig; do
        # set the batch parameter to 20 batched Paxos...this is the value for which paxos performed the best
        ../../remote-throughput.py --medium $r_medium --code $code --time $r_time --run $i --cluster $r_cluster --sync no --user $r_user --workload $workload --num_nodes 5 --target_system_name $r_system --sync_rep_factor 3 --num_clients $n --leader_reads yes --batch 20
        sleep 2
done
done
done

rm -rf ./$workload.$r_system.orig.*
mv ../../$workload.$r_system.orig.* .


# Skyros
for n in 2 5 12 15 25 35 50 70 100; do
for i in 1; do
for code in rtop; do
        # set the batch parameter to 20 batched Paxos...this is the value for which paxos performed the best
        ../../remote-throughput.py --medium $r_medium --code $code --time $r_time --run $i --cluster $r_cluster --sync no --user $r_user --workload $workload --num_nodes 5 --target_system_name $r_system --sync_rep_factor 3 --num_clients $n --leader_reads yes --batch 64
        sleep 2
done
done
done

rm -rf ./$workload.$r_system.rtop.*
mv ../../$workload.$r_system.rtop.* .

./plot_graphs.sh
