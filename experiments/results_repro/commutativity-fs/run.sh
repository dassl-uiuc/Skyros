#!/bin/bash

r_medium='data'
r_time=60 # time in seconds to run each single point in the graph
r_cluster='us-east-1' # aws region
r_user='ubuntu' # aws user
r_system='vr' #VR for all protocol variants including Skyros

# n: numclients -- for this experiment, we need to try different number of clients
# i: iteration -- paper reported average of 3 runs. For reasonable experimental times for AE, we will just do one run
# code: orig - original paxos, rtop - skyros

n=4 # 10 clients for this experiment

# initial setup
chmod 0400 ../../pems/$r_cluster.pem

./update_sources.py


# paxos
code=orig
for workload in fs; do
for i in 1; do
        ../../remote-throughput.py --medium $r_medium --code $code --time $r_time --run $i --cluster $r_cluster --sync no --user $r_user --workload $workload --num_nodes 5 --target_system_name $r_system --sync_rep_factor 3 --num_clients $n --leader_reads yes --batch 20
        sleep 2
	rm -rf ./$workload*orig*
	mv ../../$workload*orig* .
done
done

# curp
code=curp
for workload in fs; do
for i in 1; do
        ../../remote-throughput.py --medium $r_medium --code $code --time $r_time --run $i --cluster $r_cluster --sync no --user $r_user --workload $workload --num_nodes 5 --target_system_name $r_system --sync_rep_factor 3 --num_clients $n --leader_reads yes --batch 20
        sleep 2
	rm -rf ./$workload*curp*
	mv ../../$workload*curp* .
done
done

# skyros
code=rtop
for workload in fs; do
for i in 1; do
        ../../remote-throughput.py --medium $r_medium --code $code --time $r_time --run $i --cluster $r_cluster --sync no --user $r_user --workload $workload --num_nodes 5 --target_system_name $r_system --sync_rep_factor 3 --num_clients $n --leader_reads yes --batch 20
        sleep 2
	rm -rf ./$workload*rtop*
	mv ../../$workload*rtop* .
done
done

./calc.py .
./draw.py .
