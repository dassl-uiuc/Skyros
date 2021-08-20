// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 * benchmark.cpp:
 *   simple replication benchmark client
 *
 * Copyright 2013 Dan R. K. Ports  <drkp@cs.washington.edu>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************************/

#include "bench/benchmark.h"
#include "common/client.h"
#include "lib/latency.h"
#include "lib/message.h"
#include "lib/transport.h"
#include "lib/timeval.h"

#include <sys/time.h>
#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>

#define OPCODE_SIZE 1
#define KEY_SIZE 24
#define VAL_SIZE 10

namespace specpaxos {

DEFINE_LATENCY(op);

BenchmarkClient::BenchmarkClient(Client &client, Transport &transport,
                                 int numRequests, uint64_t delay,
                                 int warmupSec,
				 int tputInterval,
                 string traceFile,
                 int experimentDuration,
                 string latencyFilename)
    : tputInterval(tputInterval), client(client),
    transport(transport), numRequests(numRequests),
    delay(delay), warmupSec(warmupSec), traceFile(traceFile),
    experimentDuration(experimentDuration), latencyFilename(latencyFilename)
{
    if (delay != 0) {
        Notice("Delay between requests: %ld ms", delay);
    }
    started = false;
    done = false;
    cooldownDone = false;
    _Latency_Init(&latency, "op");
    latencies.reserve(numRequests);
    opcodes.reserve(numRequests);

    Notice("Using tracefile: %s", traceFile.c_str());
    int traceFd = open(traceFile.c_str(), O_RDONLY);
    assert(traceFd);
    struct stat stat_buf;
    int rc = 0;
    assert(!(rc = stat(traceFile.c_str(), &stat_buf)));
    void* vaddr =  mmap(NULL, stat_buf.st_size, PROT_READ, MAP_SHARED , traceFd, 0);
    size_t off = 0;
    char* curr = (char*) vaddr;
    while(off < (uint32_t) stat_buf.st_size) {
        char* op_ptr = curr;
        curr += OPCODE_SIZE + 1; 
        off += OPCODE_SIZE + 1;

        char* key_ptr = curr;
        curr += KEY_SIZE + 1;
        off += KEY_SIZE + 1;

        string op = std::string(op_ptr, OPCODE_SIZE);
        string key = std::string(key_ptr, KEY_SIZE);
        assert(op.length() == OPCODE_SIZE);
        assert(key.length() == KEY_SIZE);
        operations.push_back(std::make_pair(op, key));

        // load 100 more requests from the file 
        // sometimes the benchmark sends slightly more requests than n...
        if(operations.size() >= (uint32_t) numRequests + 100) {
            break;
        }

        // Notice("op: %s key:%s", operations.back().first.c_str(), operations.back().second.c_str());
    }
    Notice("Loaded %lu operations (numrequests = %d) from tracefile: %s",
     operations.size(), numRequests, traceFile.c_str());
}

void
BenchmarkClient::Start()
{
    n = 0;
    transport.Timer(warmupSec * 1000,
                    std::bind(&BenchmarkClient::WarmupDone,
                               this));

    if (tputInterval > 0) {
	msSinceStart = 0;
	opLastInterval = n;
	transport.Timer(tputInterval, std::bind(&BenchmarkClient::TimeInterval,
						this));
    }
    expStartTime = std::chrono::high_resolution_clock::now();
    SendNext();
}

void
BenchmarkClient::TimeInterval()
{
    if (done) {
	return;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    msSinceStart += tputInterval;
    Notice("Completed %d requests at %lu ms", n-opLastInterval, (((tv.tv_sec*1000000+tv.tv_usec)/1000)/10)*10);
    opLastInterval = n;
    transport.Timer(tputInterval, std::bind(&BenchmarkClient::TimeInterval,
					    this));
}

void
BenchmarkClient::WarmupDone()
{
    started = true;
    Notice("Completed warmup period of %d seconds with %d requests",
           warmupSec, n);
    gettimeofday(&startTime, NULL);
    n = 0;
}

void
BenchmarkClient::CooldownDone()
{

    char buf[1024];
    cooldownDone = true;
    Notice("Finished cooldown period.");
    std::vector<uint64_t> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());

    uint64_t ns = sorted[sorted.size()/2];
    LatencyFmtNS(ns, buf);
    Notice("Median latency is %ld ns (%s)", ns, buf);

    ns = 0;
    for (auto latency : sorted) {
        ns += latency;
    }
    ns = ns / sorted.size();
    LatencyFmtNS(ns, buf);
    Notice("Average latency is %ld ns (%s)", ns, buf);

    ns = sorted[sorted.size()*90/100];
    LatencyFmtNS(ns, buf);
    Notice("90th percentile latency is %ld ns (%s)", ns, buf);

    ns = sorted[sorted.size()*95/100];
    LatencyFmtNS(ns, buf);
    Notice("95th percentile latency is %ld ns (%s)", ns, buf);

    ns = sorted[sorted.size()*99/100];
    LatencyFmtNS(ns, buf);
    Notice("99th percentile latency is %ld ns (%s)", ns, buf);
}

void
BenchmarkClient::SendNext()
{
    if(n >= numRequests){
        return;
    }

    std::ostringstream msg;

    msg << operations[n].first << operations[n].second;
    bool isRead = msg.str().c_str()[0] == 'r' || msg.str().c_str()[0] == 'R';
    bool isUpdate = msg.str().c_str()[0] == 'u' || msg.str().c_str()[0] == 'U';
    bool nonNilext = msg.str().c_str()[0] == 'e' || msg.str().c_str()[0] == 'E';

    if(!isRead) {
    	if (isUpdate || nonNilext)
    		msg << string(VAL_SIZE, 'x');
    	else
    		msg << string(VAL_SIZE, 'v');
    }

    Latency_Start(&latency);
    opcodes.push_back(msg.str().c_str()[0]);
    client.Invoke(msg.str(), std::bind(&BenchmarkClient::OnReply,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
}

void
BenchmarkClient::OnReply(const string &request, const string &reply)
{
    if (cooldownDone) {
        return;
    }

    n++;
    if ((started) && (!done) && (n != 0)) {
    	uint64_t ns = Latency_End(&latency);
    	latencies.push_back(ns);
    	if (n >= numRequests) {
    	    Finish();
    	}

        auto current_time = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(current_time 
            - expStartTime).count() >= experimentDuration) {
			Notice("Experiment duration elasped. Exiting.");
            Finish();
        }
    }
    
    if (delay == 0) {
       SendNext();
    } else {
        uint64_t rdelay = rand() % delay*2;
        transport.Timer(rdelay,
                        std::bind(&BenchmarkClient::SendNext, this));
    }
}

void
BenchmarkClient::Finish()
{
    gettimeofday(&endTime, NULL);

    struct timeval diff = timeval_sub(endTime, startTime);

    Notice("Completed %d requests in " FMT_TIMEVAL_DIFF " seconds",
           n, VA_TIMEVAL_DIFF(diff));
    done = true;

    transport.Timer(warmupSec * 1000,
                    std::bind(&BenchmarkClient::CooldownDone,
                              this));


    if (latencyFilename.size() > 0) {
        Latency_FlushTo(latencyFilename.c_str());
    }
}


} // namespace specpaxos
