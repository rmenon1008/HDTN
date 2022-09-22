#ifndef _HDTN_EGRESS_ASYNC_H
#define _HDTN_EGRESS_ASYNC_H


#include <iostream>
#include <string>

#include "message.hpp"
#include "zmq.hpp"
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <map>
#include <queue>
#include "HdtnConfig.h"
#include "OutductManager.h"
#include "CircularIndexBufferSingleProducerSingleConsumerConfigurable.h"
#include "Logger.h"
#include "Telemetry.h"
#include "egress_async_lib_export.h"

#define HEGR_NAME_SZ (32)
#define HEGR_ENTRY_COUNT (1 << 20)
#define HEGR_ENTRY_SZ (256)
#define HEGR_FLAG_ACTIVE (0x0001)
#define HEGR_FLAG_UP (0x0002)
#define HEGR_HARD_IFG (0x0004)
#define HEGR_FLAG_UDP (0x0010)
#define HEGR_FLAG_STCPv1 (0x0020)
#define HEGR_FLAG_LTP (0x0040)
#define HEGR_FLAG_TCPCLv3 (0x0080)

namespace hdtn {


class HegrManagerAsync {
public:
    EGRESS_ASYNC_LIB_EXPORT HegrManagerAsync();
    EGRESS_ASYNC_LIB_EXPORT ~HegrManagerAsync();
    EGRESS_ASYNC_LIB_EXPORT void Stop();
    EGRESS_ASYNC_LIB_EXPORT void Init(const HdtnConfig & hdtnConfig, zmq::context_t * hdtnOneProcessZmqInprocContextPtr = NULL);

    //telemetry
    EgressTelemetry_t m_telemetry;

    std::unique_ptr<zmq::context_t> m_zmqCtxPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPullSock_boundIngressToConnectingEgressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPushSock_connectingEgressToBoundIngressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPushSock_connectingEgressBundlesOnlyToBoundIngressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPullSock_connectingStorageToBoundEgressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPushSock_boundEgressToConnectingStoragePtr;
    std::unique_ptr<zmq::socket_t> m_zmqSubSock_boundRouterToConnectingEgressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPubSock_boundEgressToConnectingSchedulerPtr;

    std::unique_ptr<zmq::socket_t> m_zmqRepSock_connectingGuiToFromBoundEgressPtr;

    std::unique_ptr<zmq::socket_t> m_zmqPullSignalInprocSockPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPushSignalInprocSockPtr;
    EGRESS_ASYNC_LIB_EXPORT void RouterEventHandler();
private:
    EGRESS_ASYNC_LIB_NO_EXPORT void ReadZmqThreadFunc();
    EGRESS_ASYNC_LIB_NO_EXPORT void OnSuccessfulBundleAck(uint64_t outductUuidIndex);
    EGRESS_ASYNC_LIB_NO_EXPORT void WholeBundleReadyCallback(padded_vector_uint8_t & wholeBundleVec);
    EGRESS_ASYNC_LIB_NO_EXPORT void OnFailedBundleVecSendCallback(std::vector<uint8_t>& movableBundle, uint64_t outductUuid);
    EGRESS_ASYNC_LIB_NO_EXPORT void OnFailedBundleZmqSendCallback(zmq::message_t& movableBundle, uint64_t outductUuid);
    EGRESS_ASYNC_LIB_NO_EXPORT void OnSuccessfulBundleSendCallback(std::vector<uint8_t>& userData, uint64_t outductUuid);
    EGRESS_ASYNC_LIB_NO_EXPORT void OnOutductLinkStatusChangedCallback(bool isLinkDownEvent, uint64_t outductUuid);

    EGRESS_ASYNC_LIB_EXPORT void DoLinkStatusUpdate(bool isLinkDownEvent, uint64_t outductUuid);

    OutductManager m_outductManager;
    HdtnConfig m_hdtnConfig;

    typedef std::pair<std::vector<uint8_t>, zmq::message_t> vec8_or_zmq_bundle_pair_t;
    typedef std::queue<vec8_or_zmq_bundle_pair_t> failed_bundle_queue_t;
    failed_bundle_queue_t m_failedBundleQueue;

    boost::mutex m_mutexFailedBundleQueue;
    boost::mutex m_mutexPushSignal;
    boost::mutex m_mutexPushBundleToIngress;
    boost::mutex m_mutexLinkStatusUpdate;
    volatile bool m_needToSendSignal;
    volatile std::size_t m_totalEgressInprocSignalsSent;

    std::unique_ptr<boost::thread> m_threadZmqReaderPtr;
    volatile bool m_running;
};

}  // namespace hdtn

#endif
