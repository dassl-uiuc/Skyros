// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 * Copyright 2021 Aishwarya Ganesan and Ramnatthan Alagappan
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

#ifndef _VR_DURABILITYREPLICA_H_
#define _VR_DURABILITYREPLICA_H_

#include "lib/configuration.h"
#include "lib/latency.h"
#include "common/log.h"
#include "common/replica.h"
#include "common/quorumset.h"
#include "vr/vr-proto.pb.h"

#include <thread>
#include <map>
#include <queue>
#include <memory>
#include <list>

namespace specpaxos {
namespace vr {

class VRDurabilityReplica : public Replica
{
public:
    VRDurabilityReplica(Configuration clientConfig, int myIdx, bool initialize,
              Transport *clientTransport, int batchSize,
              AppReplica *app,
              Configuration internalConfig,
              Transport *internalTransport);
    ~VRDurabilityReplica();

    void ReceiveMessage(const TransportAddress &remote,
                        const string &type, const string &data,
                        void *meta_data) override;

private:
    int batchSize;
    view_t view;
    Configuration internalConfig;
    Transport* internalTransport;
    std::map<uint64_t, std::unique_ptr<TransportAddress> > clientAddresses;
    struct ClientTableEntry
    {
        uint64_t lastReqId;
        bool replied;
    };
    std::map<uint64_t, ClientTableEntry> clientTable;    
    void HandleRequest(const TransportAddress &remote,
                       const proto::RequestMessage &msg);
    void HandleConsensusReply(const TransportAddress &remote,
                      const proto::ReplyMessage &msg);
    void HandleStatusUpdate(const TransportAddress &remote,
                      const proto::StatusUpdateMessage &msg);
    bool AmLeader() const;
};

}
}

#endif  /* _VR_DURABILITYREPLICA_H_ */
