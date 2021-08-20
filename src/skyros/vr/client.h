// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 * vr/client.h:
 * 
 * Copyright 2021 Aishwarya Ganesan and Ramnatthan Alagappan
 *
 * Small changes made to the code to implement Skyros
 *
 * *************************************************************
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

#ifndef _VR_CLIENT_H_
#define _VR_CLIENT_H_

#include "common/client.h"
#include "lib/configuration.h"
#include "vr/vr-proto.pb.h"

namespace specpaxos {
namespace vr {

class VRClient : public Client
{
public:
    VRClient(const Configuration &config,
             Transport *transport,
             uint64_t clientid = 0);
    virtual ~VRClient();
    virtual void Invoke(const string &request,
                        continuation_t continuation) override;
    virtual void InvokeUnlogged(int replicaIdx,
                                const string &request,
                                continuation_t continuation,
                                timeout_continuation_t timeoutContinuation = nullptr,
                                uint32_t timeout = DEFAULT_UNLOGGED_OP_TIMEOUT) override;
    virtual void ReceiveMessage(const TransportAddress &remote,
                                const string &type, const string &data,
                                void *meta_data) override;

protected:
    int view;
    int opnumber;
    uint64_t lastReqId;
    std::map<int, int> responses;
    std::map<int, int> leader_acked;
    struct PendingRequest
    {
        string request;
        uint64_t clientReqId;
        continuation_t continuation;
        timeout_continuation_t timeoutContinuation;
        inline PendingRequest(string request, uint64_t clientReqId,
                              continuation_t continuation)
            : request(request), clientReqId(clientReqId),
              continuation(continuation) { }
    };
    PendingRequest *pendingRequest;
    int quorum;
    PendingRequest *pendingUnloggedRequest;
    Timeout *requestTimeout;
    Timeout *unloggedRequestTimeout;

    void SendRequest();
    void ResendRequest();
    void HandleReply(const TransportAddress &remote,
                     const proto::ReplyMessage &msg);
    void HandleUnloggedReply(const TransportAddress &remote,
                             const proto::UnloggedReplyMessage &msg);
    void UnloggedRequestTimeoutCallback();
};

} // namespace specpaxos::vr
} // namespace specpaxos

#endif  /* _VR_CLIENT_H_ */
