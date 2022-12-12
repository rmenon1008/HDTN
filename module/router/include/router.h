#ifndef ROUTER_H
#define ROUTER_H

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time.hpp>
#include <boost/program_options.hpp>
#include <boost/make_unique.hpp>
#include <cstdlib>
#include <iostream>
#include "message.hpp"
#include "Environment.h"
#include "JsonSerializable.h"
#include "HdtnConfig.h"
#include "zmq.hpp"

typedef std::unique_ptr<boost::asio::deadline_timer> SmartDeadlineTimer;

class Router {
public:
    static const boost::filesystem::path DEFAULT_FILE;
    Router();
    ~Router();
    bool Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler);
    int ComputeOptimalRoute(const boost::filesystem::path & jsonEventFilePath,
                            uint64_t sourceNode, uint64_t finalDestNodeId);

    static boost::filesystem::path GetFullyQualifiedFilename(const boost::filesystem::path& filename) {
        return (Environment::GetPathHdtnSourceRoot() / "module/scheduler/src/") / filename;
    }
    volatile bool m_timersFinished;
private:
    void RouteUpdate(const boost::system::error_code& e, uint64_t nextHopNodeId,
        uint64_t finalDestNodeId, std::string event, zmq::socket_t * ptrSocket);
    HdtnConfig m_hdtnConfig;
    void MonitorExitKeypressThreadFunction();
    volatile bool m_runningFromSigHandler;
    uint64_t m_latestTime;
    std::map<uint64_t, uint64_t> m_routeTable;
};
#endif // ROUTER_H
