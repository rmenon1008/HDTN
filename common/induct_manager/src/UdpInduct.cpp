/**
 * @file UdpInduct.cpp
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright � 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 */

#include "UdpInduct.h"
#include <iostream>
#include <boost/make_unique.hpp>
#include <memory>
#include "ThreadNamer.h"

UdpInduct::UdpInduct(const InductProcessBundleCallback_t & inductProcessBundleCallback, const induct_element_config_t & inductConfig) :
    Induct(inductProcessBundleCallback, inductConfig)
{
    m_udpBundleSinkPtr = boost::make_unique<UdpBundleSink>(m_ioService, inductConfig.boundPort,
        m_inductProcessBundleCallback,
        m_inductConfig.numRxCircularBufferElements,
        m_inductConfig.numRxCircularBufferBytesPerElement,
        boost::bind(&UdpInduct::ConnectionReadyToBeDeletedNotificationReceived, this));
    

    m_ioServiceThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_ioService));
    ThreadNamer::SetIoServiceThreadName(m_ioService, "ioServiceUdpInduct");
}
UdpInduct::~UdpInduct() {
    
    m_udpBundleSinkPtr.reset();
    
    if (m_ioServiceThreadPtr) {
        m_ioServiceThreadPtr->join();
        m_ioServiceThreadPtr.reset(); //delete it
    }
}


void UdpInduct::RemoveInactiveConnection() {
    if (m_udpBundleSinkPtr && m_udpBundleSinkPtr->ReadyToBeDeleted()) {
        m_udpBundleSinkPtr.reset();
    }
}

void UdpInduct::ConnectionReadyToBeDeletedNotificationReceived() {
    boost::asio::post(m_ioService, boost::bind(&UdpInduct::RemoveInactiveConnection, this));
}