This folder contains the implementation of Skyros appearing in the paper titled "Exploiting Nil-Externality for Fast Replicated Storage" in SOSP 2021. Skyros is a new replication protocol that offers high performance by deferring ordering and executing operations until their effects are externalized. 

This folder also contains the fork of an open-source version of viewstamped replication [1]. Skyros is built by modifying viewstamped replication.

## 1. Getting Started

Skyros is usually run on a cluster of five or seven machines (replicas). We have tested on Skyros on Ubuntu 18.04 (and we recommend similar Linux distributions). 

Requirements on a typical Ubuntu machine:

** If you are an artifact evaluator, you don't have to go through this pain; we have setup clusters for you to use directly. **

1. Install the following base dependency packages: "apt install software-properties-common python-setuptools screen curl ant expect-dev python-dev python-pip protobuf-compiler pkg-config libunwind-dev libssl-dev libprotobuf-dev libevent-dev libgtest-dev g++ cmake libboost-all-dev libevent-dev libdouble-conversion-dev libgoogle-glog-dev libgflags-dev libiberty-dev liblz4-dev liblzma-dev libsnappy-dev make zlib1g-dev binutils-dev libjemalloc-dev libssl-dev pkg-config libunwind-dev libunwind8-dev libelf-dev libdwarf-dev libdouble-conversion-dev libfarmhash-dev libre2-dev libgif-dev libpng-dev libsqlite3-dev libsnappy-dev liblmdb-dev libiberty-dev"

2. We use folly, a concurrent data-structure library. To install, run the following: pushd . ; cd /tmp; git clone https://github.com/facebook/folly.git; cd folly; git checkout 49926b98f5afb5667d0c06807da79d606a6d43c3; git clone https://github.com/fmtlib/fmt.git; cd fmt; mkdir \_build; cd \_build; cmake ..; make -j10; sudo make install; cd /tmp/folly; mkdir \_build; cd \_build; cmake ..; make -j10; sudo make install; popd;

## 2. Building and Running

1. Once you have installed the above dependencies, you are all set to build Skyros: cd skyros; make -j$nproc. 
2. To run Skyros, you need a configuration file for each replica. A sample configuration file can be found in skyros/sample.
3. skyros/sample contains the scripts to start the replicas locally. To start the replicas: cd skyros/sample; ./startReplicas.sh; Confirm that the replicas are running by ps aux|grep replica. You must see three replicas running.
4. To run run a sample workload on the cluster invoke ./runClient.sh. This internally uses a trace file called test which is also present in the same directory. If everything went correctly, then you must see two files: lat.1 and latencies.1.raw. The first one contains statistics about the run and the second one contains the latencies of individual operations (in ns). 

Note that the test scripts and configuration provided are for a local cluster where all the replicas run on the same machine. This is typically used for development purposes. To run experiments on a distributed cluster, you need to modify the configuration files with the correct ip addresses. Note: Before running workloads on a distributed cluster, you need to run the following on all machines: "sudo sysctl -w net.core.rmem_max=12582912; sudo sysctl -w net.core.wmem_max=12582912; sudo sysctl -w net.core.netdev_max_backlog=5000; sudo ifconfig <YOUR_NW_INTERFACE> txqueuelen 10000;"

## 3. Credits and Acknowledgements

Please contact Aishwarya Ganesan and Ramnatthan Alagappan if you have any questions. Skyros is built by modifying an open-source implementation of viewstamped replication (forked off from [1]). Viewstamped replication (VR) is a two-RTT consensus protocol equivalent to Multi-Paxos. Skyros is a 1-RTT replication protocol that improves upon VR by exploiting nilexternality. Credits must be given to the authors and contributors of [1].

[1]. https://github.com/UWSysLab/NOPaxos
