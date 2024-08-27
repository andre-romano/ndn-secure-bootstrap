// custom-producer.cpp

#include "custom-producer.hpp"

#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"
#include "ns3/ndnSIM/ndn-cxx/util/io.hpp"

#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"

#include <limits>
#include <memory>
#include <string>

NS_LOG_COMPONENT_DEFINE("CustomProducer");

namespace ns3 {
  namespace ndn {

    // Necessary if you are planning to use ndn::AppHelper
    NS_OBJECT_ENSURE_REGISTERED(CustomProducer);

    //////////////////////
    //     PUBLIC
    //////////////////////

    TypeId CustomProducer::GetTypeId() {
      static TypeId tid =
          TypeId("CustomProducer")
              .SetParent<CustomApp>()
              .AddConstructor<CustomProducer>()
              .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                            MakeStringAccessor(&CustomProducer::m_prefix), MakeStringChecker())
              .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                            MakeUintegerAccessor(&CustomProducer::m_virtualPayloadSize),
                            MakeUintegerChecker<uint32_t>())
              .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                            TimeValue(Seconds(0)), MakeTimeAccessor(&CustomProducer::m_freshness),
                            MakeTimeChecker())
              .AddAttribute("IdentityPrefix", "Name of the Identity of the App", StringValue(""),
                            MakeStringAccessor(&CustomProducer::m_identityPrefix), MakeStringChecker());
      return tid;
    }

    CustomProducer::CustomProducer() : CustomApp() {}
    CustomProducer::~CustomProducer() {}

    void CustomProducer::StartApplication() {
      NS_LOG_FUNCTION_NOARGS();
      CustomApp::StartApplication();

      // equivalent to setting interest filter for "/prefix" prefix
      ndn::FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

      // create self-signed certificate/identity and serve it
      m_identityPrefix = (m_identityPrefix == "" ? m_prefix : m_identityPrefix);
      auto &cert = createCertificate(m_identityPrefix);
      ndn::FibHelper::AddRoute(GetNode(), ::ndn::security::v2::extractKeyNameFromCertName(cert.getName()),
                               m_face, 0);
      NS_LOG_DEBUG("Serving Data prefix: " << m_prefix << " - Certificate: " << cert.getName());

      // disable validation until producer cert is OK
      setShouldValidateData(false);

      scheduleSignInterest();    ///< @brief request for certificate signing
      scheduleSubscribeSchema(); ///< @brief subcribe for SCHEMA updates

      printKeyChain();
    }

    void CustomProducer::StopApplication() {
      NS_LOG_FUNCTION_NOARGS();
      CustomApp::StopApplication();
    }

    void CustomProducer::OnInterestKey(std::shared_ptr<const ndn::Interest> interest) {
      CustomApp::OnInterestKey(interest);

      NS_LOG_FUNCTION(interest->getName());
      sendCertificate(interest);
    }

    void CustomProducer::OnInterestContent(std::shared_ptr<const ndn::Interest> interest) {
      CustomApp::OnInterestContent(interest);

      auto dataName = interest->getName();
      NS_LOG_FUNCTION(dataName);

      if(dataName.isPrefixOf(m_schemaPrefix) || dataName.isPrefixOf(m_signPrefix)) {
        NS_LOG_INFO("Dropping interest '" << dataName << "', as producer does not reply for SCHEMA prefixes");
        return;
      }

      auto data = make_shared<Data>();
      data->setName(dataName);
      data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
      data->setContent(make_shared<::ndn::Buffer>(m_virtualPayloadSize));

      // Sign Data packet with default identity
      m_keyChain.sign(*data, m_signingInfo);

      // send packet
      sendData(data);
    }

    void CustomProducer::OnDataKey(std::shared_ptr<const ndn::Data> data) {
      CustomApp::OnDataKey(data);

      NS_LOG_INFO("Received KEY for '" << data->getName() << "'");
      if(data->getName().isPrefixOf(m_prefix + "/KEY") && isDataValid()) {
        ::ndn::security::v2::Certificate cert(*data);
        addCertificate(cert);
      }
    }

    void CustomProducer::OnDataContent(std::shared_ptr<const ndn::Data> data) {
      CustomApp::OnDataContent(data);

      if(data->getName().isPrefixOf(m_signPrefix)) {
        // we have received the signed certificate from trust anchor
        ::ndn::Data newData = *data;
        newData.setName(data->getName().getSubName(m_signPrefix.size()));
        auto newDataPtr = std::make_shared<::ndn::Data>(newData);
        // validate certificate
        setShouldValidateData(true);
        CustomApp::OnData(newDataPtr);
      }
      checkOnDataSchemaProtocol(data);
    }

    void CustomProducer::OnDataValidationFailed(const ndn::Data &data,
                                                const ::ndn::security::v2::ValidationError &error) {
      CustomApp::OnDataValidationFailed(data, error);

      if(data.getName().isPrefixOf(m_prefix + "/KEY")) {
        setShouldValidateData(false);
      }
    }

    //////////////////////
    //     PRIVATE
    //////////////////////

    void CustomProducer::scheduleSignInterest() {
      if(!hasEvent(m_signPrefix)) {
        m_sendEvents[m_signPrefix] =
            Simulator::Schedule(Seconds(0.0), &CustomProducer::sendSignInterest, this);
      } else if(!isEventRunning(m_signPrefix)) {
        m_sendEvents[m_signPrefix] =
            Simulator::Schedule(m_signLifetime, &CustomProducer::sendSignInterest, this);
      }
    }

    void CustomProducer::sendSignInterest() { sendInterest(m_signPrefix, m_signLifetime); }

  } // namespace ndn
} // namespace ns3