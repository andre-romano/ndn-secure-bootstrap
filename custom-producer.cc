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
                            MakeNameAccessor(&CustomProducer::m_prefix), MakeNameChecker())
              .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                            MakeUintegerAccessor(&CustomProducer::m_virtualPayloadSize),
                            MakeUintegerChecker<uint32_t>())
              .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                            TimeValue(Seconds(0)), MakeTimeAccessor(&CustomProducer::m_freshness),
                            MakeTimeChecker())
              .AddAttribute("IdentityPrefix", "Name of the Identity of the App", StringValue(""),
                            MakeNameAccessor(&CustomProducer::m_identityPrefix), MakeNameChecker());
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
      m_identityPrefix = (m_identityPrefix == "" ? m_prefix.deepCopy() : m_identityPrefix);
      auto &cert = createCertificate(m_identityPrefix);
      ndn::FibHelper::AddRoute(GetNode(), ::ndn::security::v2::extractKeyNameFromCertName(cert.getName()),
                               m_face, 0);
      NS_LOG_DEBUG("Serving Data prefix: " << m_prefix << " - Certificate: " << cert.getName());

      // enable packet validation
      // --> ASSUMPTION 01: trust anchor .CERT out-of-band distribution
      setShouldValidateData(true);

      scheduleSignInterest();    ///< @brief request for certificate signing
      scheduleSubscribeSchema(); ///< @brief subcribe for SCHEMA updates

      // request current trust schema
      InterestOptions opts;
      opts.canBePrefix = false;
      opts.mustBeFresh = true;
      sendInterest(m_schemaContentPrefix, m_schemaSubscribeLifetime, opts);

      printKeyChain();
    }

    void CustomProducer::StopApplication() {
      NS_LOG_FUNCTION_NOARGS();
      CustomApp::StopApplication();
    }

    void CustomProducer::OnInterestKey(std::shared_ptr<const ndn::Interest> interest) {
      CustomApp::OnInterestKey(interest);
      NS_LOG_FUNCTION(interest->getName());

      // disable caching for certificates
      DataOptions opts;
      sendCertificate(interest, opts);
    }

    void CustomProducer::OnInterestContent(std::shared_ptr<const ndn::Interest> interest) {
      CustomApp::OnInterestContent(interest);

      auto dataName = interest->getName();
      NS_LOG_FUNCTION(dataName);

      if(!m_prefix.isPrefixOf(dataName)) {
        NS_LOG_INFO("Dropping interest '" << dataName << "', - Producer only replies to content '" << m_prefix
                                          << "'");
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

    void CustomProducer::OnDataCertificate(std::shared_ptr<const ndn::Data> data) {
      NS_LOG_FUNCTION(data->getName());
      CustomApp::OnDataCertificate(data);

      NS_LOG_INFO("Received CERTIFICATE for '" << data->getName() << "'");
      if(m_signPrefix.isPrefixOf(data->getName())) {
        if(!isDataValid()) {
          // received the signed certificate from trust anchor, but we need
          // to validate it
          NS_LOG_INFO("Validating SIGN packet ...");
          setShouldValidateData(true);
          CustomApp::OnData(data);
        } else {
          NS_LOG_INFO("Opening CERTIFICATE payload of  '" << data->getName() << "' ...");
          auto certPtr =
              std::make_shared<::ndn::security::v2::Certificate>(data->getContent().blockFromValue());
          CustomProducer::OnDataCertificate(certPtr);
        }
      } else if(m_identityPrefix.isPrefixOf(data->getName()) && isDataValid()) {
        NS_LOG_INFO("Parsing CERTIFICATE '" << data->getName() << "' ...");
        ::ndn::security::v2::Certificate cert(*data);
        addCertificate(cert);
      }
    }

    void CustomProducer::OnDataContent(std::shared_ptr<const ndn::Data> data) {
      NS_LOG_FUNCTION(data->getName());
      CustomApp::OnDataContent(data);

      // onData(SCHEMA/CONTENT)
      if(m_schemaContentPrefix.isPrefixOf(data->getName())) {
        readValidationRules(data);
        // onData(SCHEMA/SUBSCRIBE)
      } else if(m_schemaSubscribePrefix.isPrefixOf(data->getName())) {
        NS_LOG_INFO("Sending SCHEMA content Interest for '" << m_schemaContentPrefix << "' ... ");
        InterestOptions opts;
        opts.canBePrefix = false;
        opts.mustBeFresh = true;
        sendInterest(m_schemaContentPrefix, m_schemaSubscribeLifetime, opts);
      }
    }

    // void CustomProducer::OnDataValidationFailed(const ndn::Data &data,
    //                                             const ::ndn::security::v2::ValidationError &error) {
    //   CustomApp::OnDataValidationFailed(data, error);
    //   ::ndn::Name keyPrefix = m_prefix;
    //   keyPrefix.append("KEY");
    //   if(keyPrefix.isPrefixOf(data.getName())) {
    //     setShouldValidateData(false);
    //   }
    // }

    //////////////////////
    //     PRIVATE
    //////////////////////

    void CustomProducer::scheduleSignInterest() {
      auto signPrefixStr = m_signPrefix.toUri();
      if(!hasEvent(signPrefixStr)) {
        m_sendEvents[signPrefixStr] =
            Simulator::Schedule(Seconds(0.0), &CustomProducer::sendSignInterest, this);
      } else if(!isEventRunning(signPrefixStr)) {
        m_sendEvents[signPrefixStr] =
            Simulator::Schedule(m_signLifetime, &CustomProducer::sendSignInterest, this);
      }
    }

    void CustomProducer::sendSignInterest() {
      auto keyName = m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getName();
      ::ndn::Name producerKeySignPrefix = m_signPrefix.deepCopy().append(keyName);
      InterestOptions opts;
      opts.canBePrefix = true;
      opts.mustBeFresh = true;
      sendInterest(producerKeySignPrefix, m_signLifetime, opts);
    }

  } // namespace ndn
} // namespace ns3