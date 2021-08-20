#!/bin/bash

rm -rf lat.*

# option 'k' specifies the trace file for the client to replay. It is of the following format: opcode key; opcodes can be one of the following: I - nilext-insert, R - read, U - nilext-update, E - non-nilext
# 'e' specfies the runtime for the client
# 'n' specifies the number of operations to replay from the trace file
# latencies.1.raw will contain the time taken for each operation in nanoseconds.
# lat.1 will contain the performance summary 

../bench/client -s ./config -c ./config.client -e 60 -m vr -k ./test -n 10 -l latencies.1 >./lat.1 2>&1 &
