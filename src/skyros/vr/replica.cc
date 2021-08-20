// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 * Copyright 2021 Aishwarya Ganesan and Ramnatthan Alagappan
 *
 * Significant changes made to the code to implement Skyros
 *
 * *************************************************************
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

#include "common/replica.h"
#include "vr/replica.h"
#include "vr/vr-proto.pb.h"

#include "lib/assert.h"
#include "lib/configuration.h"
#include "lib/latency.h"
#include "lib/message.h"
#include "lib/transport.h"
#include "lib/udptransport.h"

#include <algorithm>
#include <random>
#include <boost/config.hpp>
#include <vector>
#include <deque>
#include <iostream>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/adjacency_list.hpp>

#define RDebug(fmt, ...) Debug("[%d] " fmt, this->replicaIdx, ##__VA_ARGS__)
#define RNotice(fmt, ...) Notice("[%d] " fmt, this->replicaIdx, ##__VA_ARGS__)
#define RWarning(fmt, ...) Warning("[%d] " fmt, this->replicaIdx, ##__VA_ARGS__)
#define RPanic(fmt, ...) Panic("[%d] " fmt, this->replicaIdx, ##__VA_ARGS__)

namespace specpaxos {
namespace vr {

using namespace proto;
using namespace boost;

VRReplica::VRReplica(Configuration config, int myIdx,
                     bool initialize,
                     Transport *transport, int batchSize,
                     AppReplica *app)
    : Replica(config, 0, myIdx, initialize, transport, app),
      batchSize(batchSize),
      log(false),
      prepareOKQuorum(config.QuorumSize()-1),
      startViewChangeQuorum(config.QuorumSize()-1),
      doViewChangeQuorum(config.QuorumSize()-1),
      recoveryResponseQuorum(config.QuorumSize())
{
    this->status = STATUS_NORMAL;
    this->view = 0;
    this->lastOp = 0;
    this->lastCommitted = 0;
    this->lastRequestStateTransferView = 0;
    this->lastRequestStateTransferOpnum = 0;
    lastBatchEnd = 0;
    batchComplete = true;
    this->nilextCount = this->nonNilextCount = 0;

    if (batchSize > 1) {
        Notice("Batching enabled; batch size %d", this->batchSize);
    }

    this->viewChangeTimeout = new Timeout(transport, 5000, [this,myIdx]() {
            RWarning("Have not heard from leader; starting view change");
            StartViewChange(view+1);
        });
    this->nullCommitTimeout = new Timeout(transport, 1000, [this]() {
            SendNullCommit();
        });
    this->stateTransferTimeout = new Timeout(transport, 1000, [this]() {
            this->lastRequestStateTransferView = 0;
            this->lastRequestStateTransferOpnum = 0;
        });
    this->stateTransferTimeout->Start();
    this->resendPrepareTimeout = new Timeout(transport, 500, [this]() {
            ResendPrepare();
        });
    this->closeBatchTimeout = new Timeout(transport, 300, [this]() {
            CloseBatch();
        });
    this->recoveryTimeout = new Timeout(transport, 5000, [this]() {
            SendRecoveryMessages();
        });
    this->bgReplTimeout = new Timeout(transport, 1, [this]() {
                BgRepl();
            });
    this->manualEV = new ManualCallBack(transport, [this]() {
    	ManualCB();
    });
    _Latency_Init(&requestLatency, "request");
    _Latency_Init(&executeAndReplyLatency, "executeAndReply");

    if (initialize) {
        if (AmLeader()) {
        	this->bgReplTimeout->Start();
            nullCommitTimeout->Start();
            this->manualEV->Start();
        } else {
            viewChangeTimeout->Start();
        }
    } else {
        this->status = STATUS_RECOVERING;
        this->recoveryNonce = GenerateNonce();
        SendRecoveryMessages();
        recoveryTimeout->Start();
    }

    SendStatusUpdateWaitAck(this->view, this->status);
}

VRReplica::~VRReplica()
{
    Latency_Dump(&requestLatency);
    Latency_Dump(&executeAndReplyLatency);

    delete viewChangeTimeout;
    delete nullCommitTimeout;
    delete stateTransferTimeout;
    delete resendPrepareTimeout;
    delete closeBatchTimeout;
    delete recoveryTimeout;
    delete bgReplTimeout;

    for (auto &kv : pendingPrepares) {
        delete kv.first;
    }
}

uint64_t
VRReplica::GenerateNonce() const
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

bool
VRReplica::AmLeader() const
{
    return (configuration.GetLeaderIndex(view) == this->replicaIdx);
}

void
VRReplica::CommitUpTo(opnum_t upto)
{
    while (lastCommitted < upto) {
        Latency_Start(&executeAndReplyLatency);

        lastCommitted++;

        /* Find operation in log */
        const LogEntry *entry = log.Find(lastCommitted);
        if (!entry) {
            RPanic("Did not find operation " FMT_OPNUM " in log", lastCommitted);
        }

        /* Execute it */
        RDebug("Executing request " FMT_OPNUM, lastCommitted);
        ReplyMessage reply;
        Execute(lastCommitted, entry->request, reply);

        reply.set_view(entry->viewstamp.view);
        reply.set_opnum(entry->viewstamp.opnum);
        reply.set_clientreqid(entry->request.clientreqid());
        reply.set_replicaidx(this->replicaIdx);
        reply.set_clientid(entry->request.clientid());

        /* Mark it as committed */
        log.SetStatus(lastCommitted, LOG_STATE_COMMITTED);

	// for debugging
        if(lastCommitted %100000 == 0) {
            Notice("lastcommitted %lu", lastCommitted);
        }

        // Store reply in the client table
        ClientTableEntry &cte =
            clientTable[entry->request.clientid()];
        if (cte.lastReqId <= entry->request.clientreqid()) {
            cte.lastReqId = entry->request.clientreqid();
            cte.replied = true;
            cte.reply = reply;
        } else {
            // We've subsequently prepared another operation from the
            // same client. So this request must have been completed
            // at the client, and there's no need to record the
            // result.
        }

        /* Send reply */
        // send consensus reply to the durability server
	// for kv store app, this must be a read
        if (entry->request.needreply() == 1) {
            if(entry->request.syncread() == 1) {
                auto iter = app->clientAddresses.find(entry->request.clientid());
                if (iter != app->clientAddresses.end()) {
                    transport->SendMessage(this, *iter->second, reply);
                } 
            } else {
                auto iter = clientAddresses.find(entry->request.clientid());
                if (iter != clientAddresses.end()) {
                    transport->SendMessage(this, *iter->second, reply);
                }    
            }
        }

		Latency_End(&executeAndReplyLatency);
	}
}

void
VRReplica::SendPrepareOKs(opnum_t oldLastOp)
{
    /* Send PREPAREOKs for new uncommitted operations */
    for (opnum_t i = oldLastOp; i <= lastOp; i++) {
        /* It has to be new *and* uncommitted */
        if (i <= lastCommitted) {
            continue;
        }

        const LogEntry *entry = log.Find(i);
        if (!entry) {
            RPanic("Did not find operation " FMT_OPNUM " in log", i);
        }
        ASSERT(entry->state == LOG_STATE_PREPARED);
        UpdateClientTable(entry->request);

        PrepareOKMessage reply;
        reply.set_view(view);
        reply.set_opnum(i);
        reply.set_replicaidx(this->replicaIdx);

        RDebug("Sending PREPAREOK " FMT_VIEWSTAMP " for new uncommitted operation",
               reply.view(), reply.opnum());

        if (!(transport->SendMessageToReplica(this,
                                              configuration.GetLeaderIndex(view),
                                              reply))) {
            RWarning("Failed to send PrepareOK message to leader");
        }
    }
}

void VRReplica::BgRepl()
{
	std::queue<RequestMessage> msgs = app->GetAndDeleteFromQueue();
	while(!msgs.empty()) {
		RequestMessage msg = msgs.front();
		//Notice("Dequeued a message: %s", msg.req().op().c_str());
		HandleRequestBg(msg);
		msgs.pop();
	}
	this->bgReplTimeout->Reset();
}

void VRReplica::ManualCB()
{
	//Notice("ManualCB");
	BgRepl();
}

void
VRReplica::SendRecoveryMessages()
{
    RecoveryMessage m;
    m.set_replicaidx(this->replicaIdx);
    m.set_nonce(recoveryNonce);

    RNotice("Requesting recovery");
    if (!transport->SendMessageToAll(this, m)) {
        RWarning("Failed to send Recovery message to all replicas");
    }
}

void
VRReplica::RequestStateTransfer()
{
    RequestStateTransferMessage m;
    m.set_view(view);
    m.set_opnum(lastCommitted);

    if ((lastRequestStateTransferOpnum != 0) &&
        (lastRequestStateTransferView == view) &&
        (lastRequestStateTransferOpnum == lastCommitted)) {
        RDebug("Skipping state transfer request " FMT_VIEWSTAMP
               " because we already requested it", view, lastCommitted);
        return;
    }

    RNotice("Requesting state transfer: " FMT_VIEWSTAMP, view, lastCommitted);

    this->lastRequestStateTransferView = view;
    this->lastRequestStateTransferOpnum = lastCommitted;

    if (!transport->SendMessageToAll(this, m)) {
        RWarning("Failed to send RequestStateTransfer message to all replicas");
    }
}

void
VRReplica::SendStatusUpdateWaitAck(view_t view, ReplicaStatus status)
{
    StatusUpdateMessage sum;
    sum.set_view(view);
    sum.set_status(status);
    uint64_t statusUpdateNonce = GenerateNonce();
    sum.set_nonce(statusUpdateNonce);

    auto iter = clientAddresses.find(1);
    if (iter != clientAddresses.end()) {
        Notice("Sending status update message to the durability server");
        transport->SendMessage(this, *iter->second, sum);
	//TODO: fix this race, we have to wait for ack from dr server
    }
}

void
VRReplica::EnterView(view_t newview)
{
    RNotice("Entering new view " FMT_VIEW, newview);

    view = newview;
    status = STATUS_NORMAL;
    SendStatusUpdateWaitAck(view, status);
    lastBatchEnd = lastOp;
    batchComplete = true;

    recoveryTimeout->Stop();

    if (AmLeader()) {
        viewChangeTimeout->Stop();
        nullCommitTimeout->Start();
        bgReplTimeout->Start();
        manualEV->Start();
    } else {
        viewChangeTimeout->Start();
        nullCommitTimeout->Stop();
        resendPrepareTimeout->Stop();
        closeBatchTimeout->Stop();
        //clear tosendqueue in all replicas except leader
        app->clearQueue();
    }

    prepareOKQuorum.Clear();
    startViewChangeQuorum.Clear();
    doViewChangeQuorum.Clear();
    recoveryResponseQuorum.Clear();
}

void
VRReplica::StartViewChange(view_t newview)
{
    RNotice("Starting view change for view " FMT_VIEW, newview);

    view = newview;
    status = STATUS_VIEW_CHANGE;
    SendStatusUpdateWaitAck(view, status);

    viewChangeTimeout->Reset();
    nullCommitTimeout->Stop();
    resendPrepareTimeout->Stop();
    closeBatchTimeout->Stop();
    bgReplTimeout->Stop();

    StartViewChangeMessage m;
    m.set_view(newview);
    m.set_replicaidx(this->replicaIdx);
    m.set_lastcommitted(lastCommitted);

    if (!transport->SendMessageToAll(this, m)) {
        RWarning("Failed to send StartViewChange message to all replicas");
    }
}

void
VRReplica::SendNullCommit()
{
    CommitMessage cm;
    cm.set_view(this->view);
    cm.set_opnum(this->lastCommitted);

    ASSERT(AmLeader());

    if (!(transport->SendMessageToAll(this, cm))) {
        RWarning("Failed to send null COMMIT message to all replicas");
    }
}

void
VRReplica::UpdateClientTable(const Request &req)
{
    ClientTableEntry &entry = clientTable[req.clientid()];

    ASSERT(entry.lastReqId <= req.clientreqid());

    if (entry.lastReqId == req.clientreqid()) {
        return;
    }

    entry.lastReqId = req.clientreqid();
    entry.replied = false;
    entry.reply.Clear();
}

void
VRReplica::ResendPrepare()
{
    ASSERT(AmLeader());
    if (lastOp == lastCommitted) {
        return;
    }
    RNotice("Resending prepare");
    if (!(transport->SendMessageToAll(this, lastPrepare))) {
        RWarning("Failed to ressend prepare message to all replicas");
    }
}

void
VRReplica::CloseBatch()
{
    ASSERT(AmLeader());
    ASSERT(lastBatchEnd < lastOp);

    opnum_t batchStart = lastBatchEnd+1;

    RDebug("Sending batched prepare from " FMT_OPNUM
           " to " FMT_OPNUM,
           batchStart, lastOp);
    /* Send prepare messages */
    PrepareMessage p;
    p.set_view(view);
    p.set_opnum(lastOp);
    p.set_batchstart(batchStart);

    for (opnum_t i = batchStart; i <= lastOp; i++) {
        Request *r = p.add_request();
        const LogEntry *entry = log.Find(i);
        ASSERT(entry != NULL);
        ASSERT(entry->viewstamp.view == view);
        ASSERT(entry->viewstamp.opnum == i);
        *r = entry->request;
    }
    lastPrepare = p;

    if (!(transport->SendMessageToAll(this, p))) {
        RWarning("Failed to send prepare message to all replicas");
    }
    lastBatchEnd = lastOp;
    batchComplete = false;

    resendPrepareTimeout->Reset();
    closeBatchTimeout->Stop();
}

void
VRReplica::ReceiveMessage(const TransportAddress &remote,
                          const string &type, const string &data,
                          void *meta_data)
{
    static RequestMessage request;
    static UnloggedRequestMessage unloggedRequest;
    static PrepareMessage prepare;
    static PrepareOKMessage prepareOK;
    static CommitMessage commit;
    static RequestStateTransferMessage requestStateTransfer;
    static StateTransferMessage stateTransfer;
    static StartViewChangeMessage startViewChange;
    static DoViewChangeMessage doViewChange;
    static StartViewMessage startView;
    static RecoveryMessage recovery;
    static RecoveryResponseMessage recoveryResponse;
    static MakeDurabilityServerKnownMessage makeDurabilityServerKnown;

    if (type == request.GetTypeName()) {
        request.ParseFromString(data);
        HandleRequest(remote, request);
    } else if (type == unloggedRequest.GetTypeName()) {
        unloggedRequest.ParseFromString(data);
        HandleUnloggedRequest(remote, unloggedRequest);
    } else if (type == prepare.GetTypeName()) {
        prepare.ParseFromString(data);
        HandlePrepare(remote, prepare);
    } else if (type == prepareOK.GetTypeName()) {
        prepareOK.ParseFromString(data);
        HandlePrepareOK(remote, prepareOK);
    } else if (type == commit.GetTypeName()) {
        commit.ParseFromString(data);
        HandleCommit(remote, commit);
    } else if (type == requestStateTransfer.GetTypeName()) {
        requestStateTransfer.ParseFromString(data);
        HandleRequestStateTransfer(remote, requestStateTransfer);
    } else if (type == stateTransfer.GetTypeName()) {
        stateTransfer.ParseFromString(data);
        HandleStateTransfer(remote, stateTransfer);
    } else if (type == startViewChange.GetTypeName()) {
        startViewChange.ParseFromString(data);
        HandleStartViewChange(remote, startViewChange);
    } else if (type == doViewChange.GetTypeName()) {
        doViewChange.ParseFromString(data);
        HandleDoViewChange(remote, doViewChange);
    } else if (type == startView.GetTypeName()) {
        startView.ParseFromString(data);
        HandleStartView(remote, startView);
    } else if (type == recovery.GetTypeName()) {
        recovery.ParseFromString(data);
        HandleRecovery(remote, recovery);
    } else if (type == recoveryResponse.GetTypeName()) {
        recoveryResponse.ParseFromString(data);
        HandleRecoveryResponse(remote, recoveryResponse);
    } else if (type == makeDurabilityServerKnown.GetTypeName()) {
        makeDurabilityServerKnown.ParseFromString(data);
        HandleMakeDurabilityServerKnown(remote, makeDurabilityServerKnown);
    } else {
        RPanic("Received unexpected message type in VR proto: %s",
              type.c_str());
    }
}

void
VRReplica::HandleRequest(const TransportAddress &remote,
                         const RequestMessage &msg)
{
    viewstamp_t v;
    Latency_Start(&requestLatency);

    if (status != STATUS_NORMAL) {
        RNotice("Ignoring request due to abnormal status");
        Latency_EndType(&requestLatency, 'i');
        return;
    }

    if (!AmLeader()) {
        RDebug("Ignoring request because I'm not the leader");
        Latency_EndType(&requestLatency, 'i');
        return;
    }

    BgRepl();
    // Save the client's address -  this is really the durability event loop
    clientAddresses.erase(msg.req().clientid());
    clientAddresses.insert(
        std::pair<uint64_t, std::unique_ptr<TransportAddress> >(
            msg.req().clientid(),
            std::unique_ptr<TransportAddress>(remote.clone())));

    // Check the client table to see if this is a duplicate request
    auto kv = clientTable.find(msg.req().clientid());
    if (kv != clientTable.end()) {
        const ClientTableEntry &entry = kv->second;
        if (msg.req().clientreqid() < entry.lastReqId) {
            RNotice("Ignoring stale request");
            Latency_EndType(&requestLatency, 's');
            return;
        }
        if (msg.req().clientreqid() == entry.lastReqId) {
            // This is a duplicate request. Resend the reply if we
            // have one. We might not have a reply to resend if we're
            // waiting for the other replicas; in that case, just
            // discard the request.
            if (entry.replied) {
                RNotice("Received duplicate request; resending reply");
                if (!(transport->SendMessage(this, remote,
                                             entry.reply))) {
                    RWarning("Failed to resend reply to client");
                }
                Latency_EndType(&requestLatency, 'r');
                return;
            } else {
                RNotice("Received duplicate request but no reply available; ignoring");
                Latency_EndType(&requestLatency, 'd');
                return;
            }
        }
    }

    // Update the client table
    UpdateClientTable(msg.req());

    ClientTableEntry &cte =
        clientTable[msg.req().clientid()];

    // Check whether this request should be committed to replicas
    
    Request request;
    request.set_op(msg.req().op());
    request.set_clientid(msg.req().clientid());
    request.set_clientreqid(msg.req().clientreqid());
    request.set_syncread(msg.req().syncread());
    request.set_needreply(1);
    /* Assign it an opnum */
    ++this->lastOp;
    v.view = this->view;
    v.opnum = this->lastOp;

    RDebug("Received REQUEST, assigning " FMT_VIEWSTAMP, VA_VIEWSTAMP(v));

    /* Add the request to my log */
    log.Append(v, request, LOG_STATE_PREPARED);

    // This condition says when to close the current batch
    bool condition = false;

    // two kinds of operations can come to this function: a sync_read or a non-nilext update
    // we need to reduce the latency of reads, so immediately close the batch for a read
    if(request.syncread() == 1) {
        condition = true;    
    } else {
        // this is a non-nilext update
        nonNilextCount++;
        // run has stabilized
        if(nilextCount + nonNilextCount > 1000) 
        {
            if(nilextCount > nonNilextCount) {
                // so far, it has been nilext heavy
                // so makes sense to close the batch now
                condition = true;
            } else {
                // non-nilext heavy
                // don't close the batch immediately
                condition = batchComplete ||
                (lastOp - lastBatchEnd+1 > (unsigned int)batchSize);
            }
        } else {
            condition = true;
        }
    }

    if (condition) {
        CloseBatch();
    } else {
        RDebug("Keeping in batch");
        if (!closeBatchTimeout->Active()) {
            closeBatchTimeout->Start();
        }
    }

    nullCommitTimeout->Reset();
    Latency_End(&requestLatency);    
}

void
VRReplica::HandleRequestBg(const RequestMessage &msg)
{
    viewstamp_t v;
    Latency_Start(&requestLatency);

    if (status != STATUS_NORMAL) {
        RNotice("Ignoring request due to abnormal status");
        Latency_EndType(&requestLatency, 'i');
        return;
    }

    if (!AmLeader()) {
        RDebug("Ignoring request because I'm not the leader");
        Latency_EndType(&requestLatency, 'i');
        return;
    }

    nilextCount++;
    // Check the client table to see if this is a duplicate request
    auto kv = clientTable.find(msg.req().clientid());
    if (kv != clientTable.end()) {
        const ClientTableEntry &entry = kv->second;
        if (msg.req().clientreqid() < entry.lastReqId) {
            RNotice("Ignoring stale request");
            Latency_EndType(&requestLatency, 's');
            return;
        }
        if (msg.req().clientreqid() == entry.lastReqId) {
            if (entry.replied) {
                RNotice("Received duplicate request; resending reply");
                return;
            } else {
                RNotice("Received duplicate request but no reply available; ignoring");
                return;
            }
        }
    }

    // Update the client table
    UpdateClientTable(msg.req());

    ClientTableEntry &cte =
        clientTable[msg.req().clientid()];
    Request request;
	request.set_op(msg.req().op());
	request.set_clientid(msg.req().clientid());
	request.set_clientreqid(msg.req().clientreqid());

	/* Assign it an opnum */
	++this->lastOp;
	v.view = this->view;
	v.opnum = this->lastOp;

	RDebug("Received REQUEST, assigning " FMT_VIEWSTAMP, VA_VIEWSTAMP(v));

	/* Add the request to my log */
	log.Append(v, request, LOG_STATE_PREPARED);

	if (batchComplete ||
		(lastOp - lastBatchEnd+1 > (unsigned int)batchSize)) {
		CloseBatch();
	} else {
		RDebug("Keeping in batch");
		if (!closeBatchTimeout->Active()) {
			closeBatchTimeout->Start();
		}
	}

	nullCommitTimeout->Reset();
	Latency_End(&requestLatency);
}


void
VRReplica::HandleUnloggedRequest(const TransportAddress &remote,
                                 const UnloggedRequestMessage &msg)
{
    if (status != STATUS_NORMAL) {
        RNotice("Ignoring unlogged request due to abnormal status");
        return;
    }

    UnloggedReplyMessage reply;

    Debug("Received unlogged request %s", (char *)msg.req().op().c_str());

    ExecuteUnlogged(msg.req(), reply);

    if (!(transport->SendMessage(this, remote, reply)))
        Warning("Failed to send reply message");
}

void
VRReplica::HandlePrepare(const TransportAddress &remote,
                         const PrepareMessage &msg)
{
    RDebug("Received PREPARE <" FMT_VIEW "," FMT_OPNUM "-" FMT_OPNUM ">",
           msg.view(), msg.batchstart(), msg.opnum());

    if (this->status != STATUS_NORMAL) {
        RDebug("Ignoring PREPARE due to abnormal status");
        return;
    }

    if (msg.view() < this->view) {
        RDebug("Ignoring PREPARE due to stale view");
        return;
    }

    if (msg.view() > this->view) {
        Notice("Requesting state transfer. Reason-1. %lu > %lu", msg.view(), this->view);
        RequestStateTransfer();
        pendingPrepares.push_back(std::pair<TransportAddress *, PrepareMessage>(remote.clone(), msg));
        return;
    }

    if (AmLeader()) {
        RPanic("Unexpected PREPARE: I'm the leader of this view");
    }

    ASSERT(msg.batchstart() <= msg.opnum());
    ASSERT_EQ(msg.opnum()-msg.batchstart()+1, (unsigned int)msg.request_size());

    viewChangeTimeout->Reset();

    if (msg.opnum() <= this->lastOp) {
        RDebug("Ignoring PREPARE; already prepared that operation");
        // Resend the prepareOK message
        PrepareOKMessage reply;
        reply.set_view(msg.view());
        reply.set_opnum(msg.opnum());
        reply.set_replicaidx(this->replicaIdx);
        if (!(transport->SendMessageToReplica(this,
                                              configuration.GetLeaderIndex(view),
                                              reply))) {
            RWarning("Failed to send PrepareOK message to leader");
        }
        return;
    }

    if (msg.batchstart() > this->lastOp+1) {
        Notice("Requesting state transfer. Reason-2. %lu > %lu", msg.batchstart(), this->lastOp+1);
        RequestStateTransfer();
        pendingPrepares.push_back(std::pair<TransportAddress *, PrepareMessage>(remote.clone(), msg));
        return;
    }

    /* Add operations to the log */
    opnum_t op = msg.batchstart()-1;
    for (auto &req : msg.request()) {
        op++;
        if (op <= lastOp) {
            continue;
        }
        this->lastOp++;
        log.Append(viewstamp_t(msg.view(), op),
                   req, LOG_STATE_PREPARED);
        UpdateClientTable(req);
    }
    ASSERT(op == msg.opnum());

    /* Build reply and send it to the leader */
    PrepareOKMessage reply;
    reply.set_view(msg.view());
    reply.set_opnum(msg.opnum());
    reply.set_replicaidx(this->replicaIdx);

    if (!(transport->SendMessageToReplica(this,
                                          configuration.GetLeaderIndex(view),
                                          reply))) {
        RWarning("Failed to send PrepareOK message to leader");
    }
}

void
VRReplica::HandlePrepareOK(const TransportAddress &remote,
                           const PrepareOKMessage &msg)
{

    RDebug("Received PREPAREOK <" FMT_VIEW ", "
           FMT_OPNUM  "> from replica %d",
           msg.view(), msg.opnum(), msg.replicaidx());

    if (this->status != STATUS_NORMAL) {
        RDebug("Ignoring PREPAREOK due to abnormal status");
        return;
    }

    if (msg.view() < this->view) {
        RDebug("Ignoring PREPAREOK due to stale view");
        return;
    }

    if (msg.view() > this->view) {
        Notice("Requesting state transfer. Reason-3. %lu > %lu", msg.view(), this->view);
        RequestStateTransfer();
        return;
    }

    if (!AmLeader()) {
        RWarning("Ignoring PREPAREOK because I'm not the leader");
        return;
    }

    viewstamp_t vs = { msg.view(), msg.opnum() };
    if (auto msgs =
        (prepareOKQuorum.AddAndCheckForQuorum(vs, msg.replicaidx(), msg))) {
        /*
         * We have a quorum of PrepareOK messages for this
         * opnumber. Execute it and all previous operations.
         *
         * (Note that we might have already executed it. That's fine,
         * we just won't do anything.)
         *
         * This also notifies the client of the result.
         */
        CommitUpTo(msg.opnum());

        if (msgs->size() >= (unsigned int)configuration.QuorumSize()) {
            return;
        }

        /*
         * Send COMMIT message to the other replicas.
         *
         * This can be done asynchronously, so it really ought to be
         * piggybacked on the next PREPARE or something.
         */
        CommitMessage cm;
        cm.set_view(this->view);
        cm.set_opnum(this->lastCommitted);

        if (!(transport->SendMessageToAll(this, cm))) {
            RWarning("Failed to send COMMIT message to all replicas");
        }

        nullCommitTimeout->Reset();

        // XXX Adaptive batching -- make this configurable
        if (lastBatchEnd == msg.opnum()) {
            batchComplete = true;
            if  (lastOp > lastBatchEnd) {
                CloseBatch();
            }
        }
    }
}

void
VRReplica::HandleCommit(const TransportAddress &remote,
                        const CommitMessage &msg)
{
    RDebug("Received COMMIT " FMT_VIEWSTAMP, msg.view(), msg.opnum());

    if (this->status != STATUS_NORMAL) {
        RDebug("Ignoring COMMIT due to abnormal status");
        return;
    }

    if (msg.view() < this->view) {
        RDebug("Ignoring COMMIT due to stale view");
        return;
    }

    if (msg.view() > this->view) {
        Notice("Requesting state transfer. Reason-4. %lu > %lu", msg.view(), this->view);
        RequestStateTransfer();
        return;
    }

    if (AmLeader()) {
        RPanic("Unexpected COMMIT: I'm the leader of this view");
    }

    viewChangeTimeout->Reset();

    if (msg.opnum() <= this->lastCommitted) {
        RDebug("Ignoring COMMIT; already committed that operation");
        return;
    }

    if (msg.opnum() > this->lastOp) {
        Notice("Requesting state transfer. Reason-5. %lu > %lu", msg.opnum(), this->lastOp);
        RequestStateTransfer();
        return;
    }

    CommitUpTo(msg.opnum());
}


void
VRReplica::HandleRequestStateTransfer(const TransportAddress &remote,
                                      const RequestStateTransferMessage &msg)
{
    RDebug("Received REQUESTSTATETRANSFER " FMT_VIEWSTAMP,
           msg.view(), msg.opnum());

    if (status != STATUS_NORMAL) {
        RDebug("Ignoring REQUESTSTATETRANSFER due to abnormal status");
        return;
    }

    if (msg.view() > view) {
        Notice("Requesting state transfer. Reason-6. %lu > %lu", msg.view(), this->view);
        RequestStateTransfer();
        return;
    }

    RNotice("Sending state transfer from " FMT_VIEWSTAMP " to "
            FMT_VIEWSTAMP,
            msg.view(), msg.opnum(), view, lastCommitted);

    StateTransferMessage reply;
    reply.set_view(view);
    reply.set_opnum(lastCommitted);

    log.Dump(msg.opnum()+1, reply.mutable_entries());

    transport->SendMessage(this, remote, reply);
}

void
VRReplica::HandleStateTransfer(const TransportAddress &remote,
                               const StateTransferMessage &msg)
{
    RDebug("Received STATETRANSFER " FMT_VIEWSTAMP, msg.view(), msg.opnum());

    if (msg.view() < view) {
        RWarning("Ignoring state transfer for older view");
        return;
    }

    opnum_t oldLastOp = lastOp;

    /* Install the new log entries */
    for (auto newEntry : msg.entries()) {
        if (newEntry.opnum() <= lastCommitted) {
            // Already committed this operation; nothing to be done.
#if PARANOID
            const LogEntry *entry = log.Find(newEntry.opnum());
            ASSERT(entry->viewstamp.opnum == newEntry.opnum());
            ASSERT(entry->viewstamp.view == newEntry.view());
//          ASSERT(entry->request == newEntry.request());
#endif
        } else if (newEntry.opnum() <= lastOp) {
            // We already have an entry with this opnum, but maybe
            // it's from an older view?
            const LogEntry *entry = log.Find(newEntry.opnum());
            ASSERT(entry->viewstamp.opnum == newEntry.opnum());
            ASSERT(entry->viewstamp.view <= newEntry.view());

            if (entry->viewstamp.view == newEntry.view()) {
                // We already have this operation in our log.
                ASSERT(entry->state == LOG_STATE_PREPARED);
#if PARANOID
//              ASSERT(entry->request == newEntry.request());
#endif
            } else {
                // Our operation was from an older view, so obviously
                // it didn't survive a view change. Throw out any
                // later log entries and replace with this one.
                ASSERT(entry->state != LOG_STATE_COMMITTED);
                log.RemoveAfter(newEntry.opnum());
                lastOp = newEntry.opnum();
                oldLastOp = lastOp;

                viewstamp_t vs = { newEntry.view(), newEntry.opnum() };
                log.Append(vs, newEntry.request(), LOG_STATE_PREPARED);
            }
        } else {
            // This is a new operation to us. Add it to the log.
            ASSERT(newEntry.opnum() == lastOp+1);

            lastOp++;
            viewstamp_t vs = { newEntry.view(), newEntry.opnum() };
            log.Append(vs, newEntry.request(), LOG_STATE_PREPARED);
        }
    }


    if (msg.view() > view) {
        EnterView(msg.view());
    }

    /* Execute committed operations */
    ASSERT(msg.opnum() <= lastOp);
    CommitUpTo(msg.opnum());
    SendPrepareOKs(oldLastOp);

    // Process pending prepares
    std::list<std::pair<TransportAddress *, PrepareMessage> >pending = pendingPrepares;
    pendingPrepares.clear();
    for (auto & msgpair : pending) {
        RDebug("Processing pending prepare message");
        HandlePrepare(*msgpair.first, msgpair.second);
        delete msgpair.first;
    }
}

void
VRReplica::HandleStartViewChange(const TransportAddress &remote,
                                 const StartViewChangeMessage &msg)
{
    RDebug("Received STARTVIEWCHANGE " FMT_VIEW " from replica %d",
           msg.view(), msg.replicaidx());

    if (msg.view() < view) {
        RDebug("Ignoring STARTVIEWCHANGE for older view");
        return;
    }

    if ((msg.view() == view) && (status != STATUS_VIEW_CHANGE)) {
        RDebug("Ignoring STARTVIEWCHANGE for current view");
        return;
    }

    if ((status != STATUS_VIEW_CHANGE) || (msg.view() > view)) {
        RWarning("Received StartViewChange for view " FMT_VIEW
                 "from replica %d", msg.view(), msg.replicaidx());
        StartViewChange(msg.view());
    }

    ASSERT(msg.view() == view);

    if (auto msgs =
        startViewChangeQuorum.AddAndCheckForQuorum(msg.view(),
                                                   msg.replicaidx(),
                                                   msg)) {
        int leader = configuration.GetLeaderIndex(view);
        // Don't try to send a DoViewChange message to ourselves
        if (leader != this->replicaIdx) {
            DoViewChangeMessage dvc;
            dvc.set_view(view);
            dvc.set_lastnormalview(log.LastViewstamp().view);
            dvc.set_lastop(lastOp);
            dvc.set_lastcommitted(lastCommitted);
            dvc.set_replicaidx(this->replicaIdx);

            // Figure out how much of the log to include
            opnum_t minCommitted = std::min_element(
                msgs->begin(), msgs->end(),
                [](decltype(*msgs->begin()) a,
                   decltype(*msgs->begin()) b) {
                    return a.second.lastcommitted() < b.second.lastcommitted();
                })->second.lastcommitted();
            minCommitted = std::min(minCommitted, lastCommitted);

            log.Dump(minCommitted,
                     dvc.mutable_entries());

            // get the sorted durability log from application and
            // pass it as part of the DVC message
            for (auto &it : app->GetDurabilityLogInOrder()) {
            	Request *r = dvc.add_durlogentries();
            	*r = it;
            	//Notice("Adding durability request to DVC message: %lu, %lu", r->clientid(), r->clientreqid());
            }

            if (!(transport->SendMessageToReplica(this, leader, dvc))) {
                RWarning("Failed to send DoViewChange message to leader of new view");
            }
        }
    }
}


std::vector<Request> VRReplica::ConstructOrderedDurabilityLog(
		std::vector<DoViewChangeMessage> dvcs) {

	// union of all the requests in the durability logs from f+1 replicas including self
	std::map<CXID, Request> durabilitySet;
	std::vector<int> votingReplicas;

	//add entries from self
	votingReplicas.push_back(this->replicaIdx);
	std::vector<Request> selfSortedDurabilityLog = app->GetDurabilityLogInOrder();
	for (auto &it : selfSortedDurabilityLog) {
		durabilitySet.insert_or_assign(std::make_pair(it.clientid(), it.clientreqid()), it);
	}

	//add entries from f DVC messages
	for (auto &dvc : dvcs) {
		votingReplicas.push_back(dvc.replicaidx());
		for (auto &it : dvc.durlogentries()) {
			durabilitySet.insert_or_assign(std::make_pair(it.clientid(), it.clientreqid()), it);
		}
	}

	Notice("Voting replicas are:");
	for (auto it : votingReplicas) {
		Notice("%d", it);
	}

	Notice("Durability set is:");
	for (auto it : durabilitySet) {
		Notice("%lu, %lu", it.first.first, it.first.second);
	}

	// we consider every *ordered* pair of requests (a,b) and
	// see in how many durability logs a appears before b

	std::map<std::pair<CXID, CXID>, int> durableEntriesVotes;

	for (auto i : votingReplicas) {

		//currentDurabilityLog will contain a mapping of CXID to position, request
		std::map<CXID, std::pair<uint64_t, Request>> currentDurabilityLog;
		int position = 0;
		if (i == this->replicaIdx) {
			for (auto &it : selfSortedDurabilityLog) {
				currentDurabilityLog.insert_or_assign(
						std::make_pair(it.clientid(), it.clientreqid()),
						std::make_pair(position++, it));
			}
		} else {
			for (auto dvc : dvcs) {
				if ((int) dvc.replicaidx() == i) {
					for (auto &it : dvc.durlogentries()) {
						currentDurabilityLog.insert_or_assign(
								std::make_pair(it.clientid(), it.clientreqid()),
								std::make_pair(position++, it));
					}
				}
			}
		}

		// for every pair (r1, r2) find if this log can support r1->r2
		for (auto r1 : durabilitySet) {
			for (auto r2 : durabilitySet) {
				if (r1.first != r2.first) {

					// find position of r1 in current durability log
					int r1pos = -1;
					if (currentDurabilityLog.find(r1.first) != currentDurabilityLog.end()) {
						r1pos = currentDurabilityLog.find(r1.first)->second.first;
					}

					// find position of r2 in current durability log
					int r2pos = -1;
					if (currentDurabilityLog.find(r2.first) != currentDurabilityLog.end()) {
						r2pos = currentDurabilityLog.find(r2.first)->second.first;
					}

					//support if r1->r2 in log or only r1 appears.
					if (r1pos < r2pos || (r1pos != -1 && r2pos == -1)) {
						durableEntriesVotes[std::make_pair(r1.first, r2.first)] += 1;
					}
				}
			}
		}
	}

	Notice("durableEntriesVotes are:");
	for (auto it : durableEntriesVotes) {
		auto cxid1 = it.first.first;
		auto cxid2 = it.first.second;
		//Notice("%lu,%lu->%lu,%lu:%d", cxid1.first, cxid1.second, cxid2.first, cxid2.second, it.second);
	}

	//topological sort using boost

	//we want to find a topological sort of CXIDs in durabilityset.
	// AFAIK boost topological sort takes integer as vertices.
	// so we map CXID to an integer.


	std::map<CXID, uint64_t> vertices;
	int id = 0;

	//Notice("CXID -> vertexid mapping:");
	for (auto it : durabilitySet) {
		vertices.insert_or_assign(it.first, id++);
		//Notice("%lu,%lu:%d", it.first.first, it.first.second, id-1);
	}

	adjacency_list < listS, vecS, directedS > DAG(vertices.size());


	int requiredVotes = this->configuration.FastQuorumSize() - this->configuration.f;
	//Notice("Edges are:");
	for (auto it : durableEntriesVotes) {
		if (it.second >= requiredVotes) {

			//add an edge to the DAG
			auto it2 = vertices.find(it.first.first);
			assert(it2 != vertices.end());

			auto it3 = vertices.find(it.first.second);
			assert(it3 != vertices.end());

			add_edge(it2->second, it3->second, DAG);
			//Notice("%lu,%lu", it2->second, it3->second);

		}
	}

	std::deque<int> topologicalOrder;
	topological_sort(DAG, std::front_inserter(topologicalOrder), vertex_index_map(identity_property_map()));


	//Notice("sortedTopologicalOrdering of vertex ids is:");
	std::vector<Request> sortedTopologicalOrdering;

	int n = 1;
	for (std::deque<int>::iterator i = topologicalOrder.begin(); i != topologicalOrder.end(); ++i, ++n) {
		CXID cxidNULL = std::make_pair(-1, -1);
		CXID cxid = cxidNULL;
		for (auto it : vertices) {
			if ((int)it.second == *i) {
				cxid = it.first;
			}
		}

		//Notice("%d", *i);
		assert(cxid != cxidNULL);
		sortedTopologicalOrdering.push_back(durabilitySet.find(cxid)->second);

	}

	Notice("sortedTopologicalOrdering is:");
	for (auto it : sortedTopologicalOrdering) {
		Notice("%lu,%lu", it.clientid(), it.clientreqid());
	}

	return sortedTopologicalOrdering;

}

void
VRReplica::HandleDoViewChange(const TransportAddress &remote,
                              const DoViewChangeMessage &msg)
{
    RDebug("Received DOVIEWCHANGE " FMT_VIEW " from replica %d, "
           "lastnormalview=" FMT_VIEW " op=" FMT_OPNUM " committed=" FMT_OPNUM,
           msg.view(), msg.replicaidx(),
           msg.lastnormalview(), msg.lastop(), msg.lastcommitted());

    if (msg.view() < view) {
        RDebug("Ignoring DOVIEWCHANGE for older view");
        return;
    }

    if ((msg.view() == view) && (status != STATUS_VIEW_CHANGE)) {
        RDebug("Ignoring DOVIEWCHANGE for current view");
        return;
    }

    if ((status != STATUS_VIEW_CHANGE) || (msg.view() > view)) {
        // It's superfluous to send the StartViewChange messages here,
        // but harmless...
        RWarning("Received DoViewChange for view " FMT_VIEW
                 "from replica %d", msg.view(), msg.replicaidx());
        StartViewChange(msg.view());
    }

    ASSERT(configuration.GetLeaderIndex(msg.view()) == this->replicaIdx);

    auto msgs = doViewChangeQuorum.AddAndCheckForQuorum(msg.view(),
                                                        msg.replicaidx(),
                                                        msg);
    if (msgs != NULL) {
        // Find the response with the most up to date log, i.e. the
        // one with the latest viewstamp
        view_t latestView = log.LastViewstamp().view;
        opnum_t latestOp = log.LastViewstamp().opnum;

        DoViewChangeMessage *latestMsg = NULL;

        for (auto kv : *msgs) {
            DoViewChangeMessage &x = kv.second;

            if ((x.lastnormalview() > latestView) ||
                (((x.lastnormalview() == latestView) &&
                  (x.lastop() > latestOp)))) {
                latestView = x.lastnormalview();
                latestOp = x.lastop();
                latestMsg = &x;
            }
        }


        std::vector<DoViewChangeMessage> dvcs;
		for (auto kv : *msgs) {
			if (kv.second.lastnormalview() == latestView) {
			dvcs.push_back(kv.second);
			}
		}

        // Install the new log. We might not need to do this, if our
        // log was the most current one.
        if (latestMsg != NULL) {
            RDebug("Selected log from replica %d with lastop=" FMT_OPNUM,
                   latestMsg->replicaidx(), latestMsg->lastop());
            if (latestMsg->entries_size() == 0) {
                // There weren't actually any entries in the
                // log. That should only happen in the corner case
                // that everyone already had the entire log, maybe
                // because it actually is empty.
                ASSERT(lastCommitted == msg.lastcommitted());
                ASSERT(msg.lastop() == msg.lastcommitted());
            } else {
                if (latestMsg->entries(0).opnum() > lastCommitted+1) {
                    RPanic("Received log that didn't include enough entries to install it");
                }

                log.RemoveAfter(latestMsg->lastop()+1);
                log.Install(latestMsg->entries().begin(),
                            latestMsg->entries().end());
            }
        } else {
            RDebug("My log is most current, lastnormalview=" FMT_VIEW " lastop=" FMT_OPNUM,
                   log.LastViewstamp().view, lastOp);
        }

        lastOp = latestOp;
        // merge all the durability logs from f DVC messages and self
       	std::vector<Request> mergedAndOrderedDurabilityLog = ConstructOrderedDurabilityLog(dvcs);

		//set leader's durability log to mergedAndOrderedDurabilityLog
		app->clearDurabilityLog();
		app->addToDurabilityLogInOrder(mergedAndOrderedDurabilityLog);
		app->clearQueue();
		// add entries from durlog to consensus log
		for (auto req : mergedAndOrderedDurabilityLog) {
			auto kv = clientTable.find(req.clientid());
			if (kv != clientTable.end()) {
				const ClientTableEntry &entry = kv->second;
				if (req.clientreqid() <= entry.lastReqId) {
					continue;
				}
			}
			UpdateClientTable(req);
			++this->lastOp;
			viewstamp_t v;
			v.view = this->view;
			v.opnum = this->lastOp;
		    log.Append(v, req, LOG_STATE_PREPARED);
		}

        // How much of the log should we include when we send the
        // STARTVIEW message? Start from the lowest committed opnum of
        // any of the STARTVIEWCHANGE or DOVIEWCHANGE messages we got.
        //
        // We need to compute this before we enter the new view
        // because the saved messages will go away.
        auto svcs = startViewChangeQuorum.GetMessages(view);
        opnum_t minCommittedSVC = std::min_element(
            svcs.begin(), svcs.end(),
            [](decltype(*svcs.begin()) a,
               decltype(*svcs.begin()) b) {
                return a.second.lastcommitted() < b.second.lastcommitted();
            })->second.lastcommitted();
        opnum_t minCommittedDVC = std::min_element(
            msgs->begin(), msgs->end(),
            [](decltype(*msgs->begin()) a,
               decltype(*msgs->begin()) b) {
                return a.second.lastcommitted() < b.second.lastcommitted();
            })->second.lastcommitted();
        opnum_t minCommitted = std::min(minCommittedSVC, minCommittedDVC);
        minCommitted = std::min(minCommitted, lastCommitted);

        EnterView(msg.view());

        ASSERT(AmLeader());


        if (latestMsg != NULL) {
            CommitUpTo(latestMsg->lastcommitted());
        }

        // Send a STARTVIEW message with the new log
        StartViewMessage sv;
        sv.set_view(view);
        sv.set_lastop(lastOp);
        sv.set_lastcommitted(lastCommitted);

        log.Dump(minCommitted, sv.mutable_entries());

        if (!(transport->SendMessageToAll(this, sv))) {
            RWarning("Failed to send StartView message to all replicas");
        }
    }
}

void
VRReplica::HandleStartView(const TransportAddress &remote,
                           const StartViewMessage &msg)
{
    RNotice("Received STARTVIEW " FMT_VIEW
          " op=" FMT_OPNUM " committed=" FMT_OPNUM " entries=%d",
          msg.view(), msg.lastop(), msg.lastcommitted(), msg.entries_size());
    RDebug("Currently in view " FMT_VIEW " op " FMT_OPNUM " committed " FMT_OPNUM,
          view, lastOp, lastCommitted);

    if (msg.view() < view) {
        RWarning("Ignoring STARTVIEW for older view");
        return;
    }

    if ((msg.view() == view) && (status != STATUS_VIEW_CHANGE)) {
        RWarning("Ignoring STARTVIEW for current view");
        return;
    }

    ASSERT(configuration.GetLeaderIndex(msg.view()) != this->replicaIdx);

    if (msg.entries_size() == 0) {
        ASSERT(msg.lastcommitted() == lastCommitted);
        ASSERT(msg.lastop() == msg.lastcommitted());
    } else {
        if (msg.entries(0).opnum() > lastCommitted+1) {
            RPanic("Not enough entries in STARTVIEW message to install new log");
        }

        // Install the new log
        log.RemoveAfter(msg.lastop()+1);
        log.Install(msg.entries().begin(),
                    msg.entries().end());
    }


    EnterView(msg.view());
    opnum_t oldLastOp = lastOp;
    lastOp = msg.lastop();

    ASSERT(!AmLeader());

    CommitUpTo(msg.lastcommitted());
    SendPrepareOKs(oldLastOp);
}

void
VRReplica::HandleRecovery(const TransportAddress &remote,
                          const RecoveryMessage &msg)
{
    RDebug("Received RECOVERY from replica %d", msg.replicaidx());

    if (status != STATUS_NORMAL) {
        RDebug("Ignoring RECOVERY due to abnormal status");
        return;
    }

    RecoveryResponseMessage reply;
    reply.set_replicaidx(this->replicaIdx);
    reply.set_view(view);
    reply.set_nonce(msg.nonce());
    if (AmLeader()) {
        reply.set_lastcommitted(lastCommitted);
        reply.set_lastop(lastOp);
        log.Dump(0, reply.mutable_entries());
		Notice("Sending durability request as part of RecoveryResponseMessage in leader:");
		for (auto &it : app->GetDurabilityLogInOrder()) {
			Request *r = reply.add_durlogentries();
			*r = it;
			Notice("%lu, %lu", r->clientid(), r->clientreqid());
		}
    }

    if (!(transport->SendMessage(this, remote, reply))) {
        RWarning("Failed to send recovery response");
    }
    return;
}

void
VRReplica::HandleRecoveryResponse(const TransportAddress &remote,
                                  const RecoveryResponseMessage &msg)
{
    RDebug("Received RECOVERYRESPONSE from replica %d",
           msg.replicaidx());

    if (status != STATUS_RECOVERING) {
        RDebug("Ignoring RECOVERYRESPONSE because we're not recovering");
        return;
    }

    if (msg.nonce() != recoveryNonce) {
        RNotice("Ignoring recovery response because nonce didn't match");
        return;
    }

    auto msgs = recoveryResponseQuorum.AddAndCheckForQuorum(msg.nonce(),
                                                            msg.replicaidx(),
                                                            msg);
    if (msgs != NULL) {
        view_t highestView = 0;
        for (const auto &kv : *msgs) {
            if (kv.second.view() > highestView) {
                highestView = kv.second.view();
            }
        }

        int leader = configuration.GetLeaderIndex(highestView);
        ASSERT(leader != this->replicaIdx);
        auto leaderResponse = msgs->find(leader);
        if ((leaderResponse == msgs->end()) ||
            (leaderResponse->second.view() != highestView)) {
            RDebug("Have quorum of RECOVERYRESPONSE messages, "
                   "but still need to wait for one from the leader");
            return;
        }

        Notice("Recovery completed");

        log.Install(leaderResponse->second.entries().begin(),
                    leaderResponse->second.entries().end());

    	// set replica's durability log to that sent by leader
    	app->clearDurabilityLog();
    	std::vector<Request> tmpDurLog;
    	for (auto it:leaderResponse->second.durlogentries())
    	{
    		tmpDurLog.push_back(it);
    	}
    	app->addToDurabilityLogInOrder(tmpDurLog);
        EnterView(leaderResponse->second.view());
        lastOp = leaderResponse->second.lastop();
        CommitUpTo(leaderResponse->second.lastcommitted());
    }
}

void
VRReplica::HandleMakeDurabilityServerKnown(const TransportAddress &remote,
                                  const MakeDurabilityServerKnownMessage &msg)
{
	// save the address of the durability server
	// this is done when the durability server starts
	// this is required for relaying status-update messages to the durability server
	Notice("Received Makeknown message from the durability server");
    clientAddresses.erase(1);
    clientAddresses.insert(
        std::pair<uint64_t, std::unique_ptr<TransportAddress> >(
			1,
            std::unique_ptr<TransportAddress>(remote.clone())));
    
    // first time, inform the status and view
    SendStatusUpdateWaitAck(this->view, this->status);
}

} // namespace specpaxos::vr
} // namespace specpaxos
