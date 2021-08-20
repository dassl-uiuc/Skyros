#!/bin/bash

# i specifies the replica number

killall -s 9 replica
rm -rf /tmp/vrlog*
../bench/replica -i 0 -b 64 -c ./config -m vr > /tmp/vrlog.0 2>&1 &
../bench/replica -i 1 -b 64 -c ./config -m vr > /tmp/vrlog.1 2>&1 &
../bench/replica -i 2 -b 64 -c ./config -m vr > /tmp/vrlog.2 2>&1 &

sleep 2