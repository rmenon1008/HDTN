/***************************************************************************
 * NASA Glenn Research Center, Cleveland, OH
 * Released under the NASA Open Source Agreement (NOSA)
 * May  2021
 *
 ***************************************************************************
 */

#ifndef _HDTN_LOG_H
#define _HDTN_LOG_H

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <string>
#include <memory>
#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/value_ref.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/config/detail/suffix.hpp>
#include "log_lib_export.h"

namespace hdtn{

/**
 * Log levels. These match the Boost trivial level definitions
 */
#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_FATAL 5

/**
 * _NO_OP_STREAM_LOGGER is a macro to efficiently discard a streaming expression. Since the "else"
 * branch is never taken, the compiler should optimize it out.
 */
#define _NO_OP_STREAM_LOGGER if (true) {} else std::cout

/**
 * The following macros provide access for logging at various levels
 */
#if LOG_LEVEL > LOG_LEVEL_TRACE
    #define LOG_TRACE(subprocess) _NO_OP_STREAM_LOGGER
#else
    #define LOG_TRACE(subprocess) _LOG_INTERNAL(subprocess, boost::log::trivial::severity_level::trace)
#endif
   
#if LOG_LEVEL > LOG_LEVEL_DEBUG
    #define LOG_DEBUG(subprocess) _NO_OP_STREAM_LOGGER
#else
    #define LOG_DEBUG(subprocess) _LOG_INTERNAL(subprocess, boost::log::trivial::severity_level::debug)
#endif

#if LOG_LEVEL > LOG_LEVEL_INFO
    #define LOG_INFO(subprocess) _NO_OP_STREAM_LOGGER
#else
    #define LOG_INFO(subprocess) _LOG_INTERNAL(subprocess, boost::log::trivial::severity_level::info)
#endif

#if LOG_LEVEL > LOG_LEVEL_WARNING
    #define LOG_WARNING(subprocess) _NO_OP_STREAM_LOGGER
#else
    #define LOG_WARNING(subprocess) _LOG_INTERNAL(subprocess, boost::log::trivial::severity_level::warning)
#endif

#if LOG_LEVEL > LOG_LEVEL_ERROR
    #define LOG_ERROR(subprocess) _NO_OP_STREAM_LOGGER
#else
    #define LOG_ERROR(subprocess) _LOG_INTERNAL(subprocess, boost::log::trivial::severity_level::error)
#endif

#if LOG_LEVEL > LOG_LEVEL_FATAL
    #define LOG_FATAL(subprocess) _NO_OP_STREAM_LOGGER
#else
    #define LOG_FATAL(subprocess) _LOG_INTERNAL(subprocess, boost::log::trivial::severity_level::fatal)
#endif

#define _LOG_INTERNAL(subprocess, lvl)\
    hdtn::Logger::ensureInitialized();\
    BOOST_LOG_STREAM_CHANNEL_SEV(hdtn::Logger::m_severityChannelLogger, subprocess, lvl)\
        << boost::log::add_value("File", __FILE__)\
        << boost::log::add_value("Line", __LINE__)


typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> sink_t;

/**
 * @brief Logger class used to create log files and log messages.
 */
class Logger
{
public:
    /**
     * Initializes the logger if it hasn't been created yet. This is intended to be called from
     * the LOG_* macros.
     */
    LOG_LIB_EXPORT static void ensureInitialized();

    /**
     * Represents processes that Logger supports. New processes using the logger
     * should be added to this list.
     */
    enum class Process {
        bpgen,
        bping,
        bpreceivefile,
        bpsendfile,
        bpsink,
        ltpfiletransfer,
        egress,
        telem,
        hdtnoneprocess,
        ingress,
        router,
        scheduler,
        storage,
        releasemessagesender,
        storagespeedtest,
        udpdelaysim,
        unittest,
        none
    };

    /**
     * Represents sub-processes that Logger supports. New sub-processes using the logger
     * should be added to this list.
     */
    enum class SubProcess {
        egress,
        ingress,
        router,
        scheduler,
        storage,
        telem,
        gui,
        unittest,
        none
    };


    /**
     * Converts a Process enum value to its string reresentation.
     * @param process the Process enum value to convert
     */
    LOG_LIB_EXPORT static std::string toString(Logger::Process process);

    /**
     * Converts a SubProcess enum value to its string representation.
     * @param subprocess the Process enum value to convert
     */
    LOG_LIB_EXPORT static std::string toString(Logger::SubProcess subProcess);

    /**
     * Process attribute
     */
    typedef boost::log::attributes::constant<Logger::Process> process_attr_t;
    LOG_LIB_EXPORT static Logger::process_attr_t process_attr;

    /*
     * Initializes the logger with a process identifier to be
     * used in all messages
     */
    LOG_LIB_EXPORT static void initializeWithProcess(Logger::Process process);

    /**
     * Underlying boost logger 
     */
    typedef boost::log::sources::severity_channel_logger_mt<
        boost::log::trivial::severity_level,
        Logger::SubProcess
    > severity_channel_logger_t; //mt for multithreaded
    LOG_LIB_EXPORT static Logger::severity_channel_logger_t m_severityChannelLogger;

    // Deprecated -- use LOG_* macros instead.
    LOG_LIB_EXPORT static SubProcess fromString(std::string subprocess);
    LOG_LIB_EXPORT static Logger* getInstance();
    LOG_LIB_EXPORT void logInfo(const std::string & subprocess, const std::string & message);
    LOG_LIB_EXPORT void logNotification(const std::string & subprocess, const std::string & message);
    LOG_LIB_EXPORT void logWarning(const std::string & subprocess, const std::string & message);
    LOG_LIB_EXPORT void logError(const std::string & subprocess, const std::string & message);
    LOG_LIB_EXPORT void logCritical(const std::string & subprocess, const std::string & message);
    // End deprecation.

    LOG_LIB_EXPORT ~Logger();

private:
    LOG_LIB_EXPORT Logger();
    LOG_LIB_EXPORT Logger(Logger const&) = delete;
    LOG_LIB_EXPORT Logger& operator=(Logger const&) = delete;
    

    /** 
     * Initializes the logger
     */
    LOG_LIB_EXPORT void init();

    /**
     * Registers attributes to be used in log messages
     */
    LOG_LIB_EXPORT void registerAttributes();

    /**
     * Creates a new log file sink for the requested process.
     * @param process The process of the logs stored in this file.
     */
    LOG_LIB_EXPORT void createFileSinkForProcess(Logger::Process process);

    /**
     * Creates a new log file sink for the requested sub-process.
     * @param subprocess The sub-process of the logs stored in this file.
     */
    LOG_LIB_EXPORT void createFileSinkForSubProcess(Logger::SubProcess subprocess);

    /**
     * Creates a new log file sink for the requested severity level.
     * @param level The severity level of the logs stored in this file.
     */ 
    LOG_LIB_EXPORT void createFileSinkForLevel(boost::log::trivial::severity_level level);

    /**
     * Creates a new sink for writing messages to stdout
     */
    LOG_LIB_EXPORT void createStdoutSink();

    /**
     * Extracts the process attribute value
     */
    LOG_LIB_EXPORT Logger::Process getProcessAttributeVal();

    /**
     * Formatter used for messages displayed in the console 
     */
    LOG_LIB_EXPORT static boost::log::formatter consoleFormatter();

    /**
     * Formatter used for messages displayed in the level log file
     */
    LOG_LIB_EXPORT static boost::log::formatter levelFileFormatter();

    /**
     * Formatter used for messages displayed in the process log file
     */
    LOG_LIB_EXPORT static boost::log::formatter processFileFormatter();

    static std::unique_ptr<Logger> logger_; //singleton instance
    static boost::mutex mutexSingletonInstance_;
    static volatile bool loggerSingletonFullyInitialized_;
};
}

#endif 
