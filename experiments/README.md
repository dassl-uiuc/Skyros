## Artifact evaluation

To make the artifact evaluation process easier, we have set up a cluster on AWS EC2. Evaluators can use this cluster to run experiments and reproduce results. This repository and all required dependencies have already been cloned and installed on the machines. 


## Important notes about using the cluster for the first time 

Basic information about the cluster: our cluster comprises of six machines (five replicas and one client). This cluster is hosted on EC2 m5zn.baremetal instances. All machines run Ubuntu 18.04.

For artifact evaluation purposes, we have automated running all the experiments. To start these experiments, you will need to ssh into the client machines. The below instructions tell how to successfully ssh into the client (or generally any machine in the cluster).

First, clone this repository on to your local machine. You need a pem file to ssh into the cluster. The pem file is supposed to be present inside the experiments/pem folder. Because this is a sensitive file, you must copy the pem file contents from HotCRP and put it into a file called us-east-1.pem in the experiments/pems/ folder. Run chmod 0400 experiments/pems/us-east-1.pem to enable correct level of permissions. 

Second, you need to know the external ips of the machines in the cluster to ssh into them. The ip addresses are present in the experiments/external_ips file. However, every time the cluster is restarted, the external ips change. Whenever we restart the cluster, we will make sure to update the external_ips file and push it to the repo. Thus, you must make sure that you pull the latest code on your local workstation before you try to ssh into the machines.

Finally, your public IP must be added to our security group so that you can ssh into the machines (this is a AWS EC2 best practice; otherwise, anybody can try to ssh and this has caused problems in the past). We will help you when you would like to access the cluster for the first time. You may need to run 'curl ipecho.net/plain; echo' on your terminal to let us know your public IP; we will add it.

If all steps are done correctly, you must be able to ssh into the cluster. Test this by running: "chmod 0400 experiments/pems/us-east-1.pem; client=$(tail -n 1 experiments/external_ips); ssh -i experiments/pems/us-east-1.pem ubuntu@$client". This command will ssh into the client machine which is usually the last line in the external_ips file. From your local machine, most of the times, you will need to ssh only into the client (not the five replicas); this is because you will start the experiments only from the client machine.

Finally, since it is expensive to keep the baremetal-cluster running for a month in AWS, you would need to contact us when you would like to reproduce the results and we will start the machines for you. Also, since we have only one live cluster, artifact evaluators have to proceed one at a time to reproduce the results. However, to reproduce all experiments, we expect only a total time of \~6 hours. To ease evaluation, we have automated the experiments. Please see the below section for more details. If for some reason, you are unable to ssh into the machines listed in external_ips, please contact the authors via hotcrp and we will help you. 

##  Running experiments

We have automated all experiments presented in the paper. You must **ssh into the client machine to start the experiments**.
There are 10 sets of experiments all of which are kept as individual directories inside the results_repro directory.

1. micro-nilext-only: produces Figure-8a in the submission version (estimated time: ~ 35 minutes, graph file name: micro-nilext-only.eps). 
2. micro-nilext-nonnilext: produces Figure-8b(i) (estimated time: ~ 28 minutes, graph file name: micro-nilext-nonnilext.eps).
3. micro-nilext-reads: produces Figure-8b(ii) (estimated time: ~ 40 minutes, graph file names: nilext_read_uniform.eps, nilext_read_zipfian.eps).
4. micro-mixed-all: produces Figure-8b(iii) (estimated time: ~ 20 minutes, graph file name: micro-mixed-all.eps). 
5. ycsb: produces Figure-9a through Figure-9e (estimated time: ~ 26 minutes, file names for Figure 9 are 9a: ycsb-thrpt.eps, 9b: ycsb-a-read.eps, 9c: ycsb-a-all.eps, 9d: ycsb-b-read.eps, and 9e: ycsb-b-all.eps).
6. micro-read-latest: produces Figure-8c (estimated time: ~ 90 minutes, graph file name: exp3.eps). 
7. rocksdb: produces Figure-10 (estimated time: ~ 8 minutes, graph file name: rocks.eps).
8. commutativity-kv: produces Figure-11a, 11b, and 11c (estimated time: ~ 20 minutes, file names for Figure 11 are 11a: kvcurp.eps, 11b: kvcurp-read.eps, and 11c: kvcurp-write.eps). 
9. commutativity-compat: produces Figure-11e (estimated time: ~ 16 minutes, graph file name: compat.eps).
10. commutativity-fs: produces Figure-11d. (estimated time: ~ 6 minutes, graph file name: fscurp.eps).

Each directory has a bunch of scripts but the one you want to run is **"run.sh"**. 

*What run.sh does internally* run.sh first ensures that the sources on the replicas are correct (by internally ssh-ing to the replicas etc). The code for some experiments are on a different branch. For example, the rocksdb experiments run from a branch called "rocksdb". We have a different branch because the storage system being replicated is entirely different from the default one (which is in the master branch). However, we have made it easy to switch between branches. The script will **automatically** checkout the required branch before running an experiment. As an aside, if you happen to modify run.sh for some reason, do git checkout run.sh before you start the next experiment.

Then, the script starts the experiment using the experiments/remote-throughput.py script. This script is the main client-side experiment orchestrator that starts the replica processes, starts the clients, collects results etc. The clients, once started, start sending requests to the cluster. These requests are usually replayed from trace files. These trace files were generated by us and are present on the client machine (most like inside /mnt/data). Once a workload run is completed, the remote-throughput script accumulates the results (stats etc). Finally, the run.sh script invokes the result-processing and graphing scripts. You just need to invoke run.sh and everything is done automatically. At the end, you can see one or more .eps files in the directory which are the graphs. 

To view the graphs, we recommend locally cloning this repo on your local work station. On your local machine, go to experiments. Inside this directory, a script called download_graphs.sh downloads the graphs from the client machine to your local laptop where you can view them. Comment/uncomment the required lines in the download script to download what you are interested in. You may want to run one experiment at a time and download the corresponding results. For example, if you want to run micro-nilext-only, ssh into the client, navigate to experiments/results_repro/micro-nilext-only and start run.sh. Once done, check that the required graph files are produced (\*.eps files) in the directory. Then, from your laptop, invoke download_graphs.sh with "scp -i $pem $user@$client:$results_home/micro-nilext-only/micro-nilext-only.eps" line uncommented.

**If you just want to run all experiments in one go, just ssh into the client machine and run: [./run_all.sh](./run_all.sh) on the client machine.** This will take about 5-6 hours to complete; so do remember to start the run within a **screen** session or something similar. After this script finishes, run [./download_graphs.sh](./download_graphs.sh) from your **local machine** to download all the graphs to your local machine. This will download all the eps files to your local machine. 

## Traces
All our experiments replay requests from trace files that were generated by us. You can find the trace files here: https://zenodo.org/record/5520021

Download all the compressed trace files from the above link and uncompress them to /mnt/data on the client machine; our experiments expect the traces to be available at /mnt/data/.

## To build and run locally

For reproducing the results (for artifact evaluation), you don't need to build or run the cluster on your own. If you would like to test the basic functionality on your local machine, please see [../src/README.md](../src/README.md).

## Code Organization
To understand how Skyros' design is implemented in the code please see [../src/skyros/code-organization.md](../src/skyros/code-organization.md)
