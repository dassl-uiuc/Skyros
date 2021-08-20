#!/bin/bash

cd results_repro

cd ./micro-nilext-only
./run.sh
sleep 2

cd ../micro-nilext-nonnilext
./run.sh
sleep 2

cd ../micro-nilext-reads
./run.sh
sleep 2

cd ../micro-mixed-all
./run.sh
sleep 2

cd ../ycsb
./run.sh
sleep 2

cd ../micro-read-latest
./run.sh
sleep 2

cd ../rocksdb
./run.sh
sleep 2

cd ../commutativity-kv
./run.sh
sleep 2

cd ../commutativity-compat
./run.sh
sleep 2

cd ../commutativity-fs
./run.sh
sleep 2

cd ../../