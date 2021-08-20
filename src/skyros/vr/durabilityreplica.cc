// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 *
 * Copyright 2021 Aishwarya Ganesan and Ramnatthan Alagappan
 *
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

#include "common/replica.h"
#include "vr/durabilityreplica.h"
#include "vr/vr-proto.pb.h"

#include "lib/assert.h"
#include "lib/configuration.h"
#include "lib/latency.h"
#include "lib/message.h"
#include "lib/transport.h"

#include <algorithm>
#include <random>

#define RDebug(fmt, ...) Debug("[%d] " fmt, this->replicaIdx, ##__VA_ARGS__)
#define RNotice(fmt, ...) Notice("[%d] " fmt, this->replicaIdx, ##__VA_ARGS__)
#define RWarning(fmt, ...) Warning("[%d] " fmt, this->replicaIdx, ##__VA_ARGS__)
#define RPanic(fmt, ...) Panic("[%d] " fmt, this->replicaIdx, ##__VA_ARGS__)

namespace specpaxos {
namespace vr {

using namespace proto;
VRDurabilityReplica:: VRDurabilityReplica(Configuration clientConfig, int myIdx, bool initialize,
              Transport *clientTransport, int batchSize,
              AppReplica *app, Configuration internalConfig, Transport *internalTransport)
    : Replica(clientConfig, 0, myIdx, initialize, clientTransport, app),
    internalConfig(internalConfig),
    internalTransport(internalTransport)
{
    this->status = STATUS_NORMAL;
    this->view = 0;
    this->batchSize = batchSize;

    // we are a client to the consensus group
    // so register like a client would do
    internalTransport->Register(this, internalConfig, -1, -1);

    MakeDurabilityServerKnownMessage mdsm;
    mdsm.set_replicaidx(this->replicaIdx);
    internalTransport->SendMessageToReplica(this, this->replicaIdx, mdsm);
}

VRDurabilityReplica::~VRDurabilityReplica()
{
    
}

bool
VRDurabilityReplica::AmLeader() const
{
    return (configuration.GetLeaderIndex(view) == this->replicaIdx);
}

void
VRDurabilityReplica::ReceiveMessage(const TransportAddress &remote,
                          const string &type, const string &data,
                          void *meta_data)
{
    static RequestMessage request;
    static proto::ReplyMessage reply;
    static proto::StatusUpdateMessage sum;

    if (type == request.GetTypeName()) {
        request.ParseFromString(data);
        HandleRequest(remote, request);
    } else if (type == reply.GetTypeName()) {
        reply.ParseFromString(data);
        HandleConsensusReply(remote, reply);
    } else if (type == sum.GetTypeName()) {
        sum.ParseFromString(data);
        HandleStatusUpdate(remote, sum);
    } else {
        RPanic("Received unexpected message type in VR proto: %s",
              type.c_str());
    }
}

void
VRDurabilityReplica::HandleConsensusReply(const TransportAddress &remote,
                      const proto::ReplyMessage &msg)
{
    auto iter = clientAddresses.find(msg.clientid());
    if (iter != clientAddresses.end()) {
        ReplyMessage reply;
        reply.set_reply(msg.reply());
        reply.set_view(msg.view());
        reply.set_opnum(msg.opnum()); 
        reply.set_clientreqid(msg.clientreqid());
        reply.set_replicaidx(msg.replicaidx());
        transport->SendMessage(this, *iter->second, reply);
        return;
    }


	Notice("Unknown client %lu; current table is:", msg.clientid());    	
    for (auto const& pair: clientAddresses) {
        Notice("clientid: %lu", pair.first);
    }

    // for reply for which we don't know the client?
    // that is fishy...crash.
    assert(0);
}

void 
VRDurabilityReplica::HandleStatusUpdate(const TransportAddress &remote,
                      const proto::StatusUpdateMessage &msg) {
	Notice("Received a statusupdate message with view %lu, status %u", msg.view(), msg.status());
	this->view = msg.view();
	this->status = static_cast<ReplicaStatus>(msg.status());
}

void VRDurabilityReplica::HandleRequest(const TransportAddress &remote,
		const RequestMessage &msg) {

	static int syncPathRead = 0;

	if (status != STATUS_NORMAL) {
		RNotice("Ignoring request due to abnormal status");
		return;
	}

	bool isNilext = app->IsNilext(msg);
	if (!isNilext) {
		if(!AmLeader())
			return;
	}

	bool syncOrder = false;
	string readRes = "";

	app->AppUpcall(msg, syncOrder, readRes);

	if (syncOrder) {
	// Order the operation now; add to consensus log by sending an internal message to consensus.
		if (readRes.compare("ordernowread!") == 0) {
			syncPathRead++;
			if (syncPathRead%5000 == 0) {
				Notice("syncPathRead: %d", syncPathRead);
			}	
			
			// Save the client's address
			app->clientAddresses.insert_or_assign(msg.req().clientid(),
			     std::unique_ptr<TransportAddress>(remote.clone()));

            RequestMessage *msg2 = new RequestMessage();
            Request request;
            request.set_op(msg.req().op());
            request.set_clientid(msg.req().clientid());
            request.set_clientreqid(msg.req().clientreqid());
            request.set_syncread(1);
            msg2->set_allocated_req(&request);

			internalTransport->SendMessageToReplica(this, this->replicaIdx, *msg2);
		}

		return;
	} else {
		// nilext write or fast read directly respond
		ReplyMessage reply;
		reply.set_reply(readRes);
		reply.set_view(this->view);
		reply.set_opnum(0); //cannot order now!
		reply.set_clientreqid(msg.req().clientreqid());
		reply.set_replicaidx(this->replicaIdx);
		transport->SendMessage(this, remote, reply);
	}

	if (AmLeader()) {
		if (isNilext) {
			// Add to a queue for background replication.
			app->AddToQueue(msg);
			internalTransport->TriggerManualEvent(1);
		}
	}
}
}
}
