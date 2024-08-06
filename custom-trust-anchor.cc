// custom-trust-anchor.cpp

#include "custom-trust-anchor.hpp"

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

NS_LOG_COMPONENT_DEFINE("CustomTrustAnchor");

namespace ns3 {
  namespace ndn {

    // Necessary if you are planning to use ndn::AppHelper
    NS_OBJECT_ENSURE_REGISTERED(CustomTrustAnchor);

    //////////////////////
    //     PUBLIC
    //////////////////////

    TypeId CustomTrustAnchor::GetTypeId() {
      static TypeId tid =
          TypeId("CustomTrustAnchor")
              .SetParent<CustomApp>()
              .AddConstructor<CustomTrustAnchor>()
              .AddAttribute("ZonePrefix", "Zone name prefix", StringValue("/"),
                            MakeStringAccessor(&CustomTrustAnchor::m_zonePrefix), MakeStringChecker())
              .AddAttribute("SchemaFreshness",
                            "Freshness of Schema data packets, if 0, then unlimited "
                            "freshness",
                            TimeValue(Seconds(0)), MakeTimeAccessor(&CustomTrustAnchor::m_schemaFreshness),
                            MakeTimeChecker())
              .AddAttribute("TrustAnchorCert", "Trust Anchor .cert filename",
                            StringValue("./scratch/sim_bootsec/config/trustanchor.cert"),
                            MakeStringAccessor(&CustomTrustAnchor::m_trustAnchorCert), MakeStringChecker());
      return tid;
    }

    CustomTrustAnchor::CustomTrustAnchor() : CustomApp() {}
    CustomTrustAnchor::~CustomTrustAnchor() {}

    void CustomTrustAnchor::StartApplication() {
      NS_LOG_FUNCTION_NOARGS();
      ndn::CustomApp::StartApplication();

      // create trust anchor identity, cert file (public key)
      createTrustAnchor();
      readValidationRules();

      // SCHEMA to CONSUMERS / PRODUCERS
      ndn::FibHelper::AddRoute(GetNode(), m_schemaPrefix, m_face, 0);
      NS_LOG_INFO("Monitoring prefix '" << m_schemaPrefix << "'");

      // SIGN certificates to PRODUCERS
      ndn::FibHelper::AddRoute(GetNode(), m_signPrefix, m_face, 0);
      NS_LOG_INFO("Monitoring prefix '" << m_signPrefix << "'");

      printKeyChain();
    }

    void CustomTrustAnchor::StopApplication() {
      ndn::CustomApp::StopApplication();
      // no additional code needed
    }

    void CustomTrustAnchor::OnInterestKey(std::shared_ptr<const ndn::Interest> interest) {
      ndn::CustomApp::OnInterestKey(interest);
      // empty on purpose
    }

    void CustomTrustAnchor::OnInterestContent(std::shared_ptr<const ndn::Interest> interest) {
      ndn::CustomApp::OnInterestContent(interest);

      auto dataName = interest->getName();
      auto data = make_shared<Data>();
      data->setName(dataName);

      if(dataName.isPrefixOf(m_schemaPrefix)) {
        if(dataName.isPrefixOf(m_schemaContentPrefix)) {
          data->setFreshnessPeriod(::ndn::time::milliseconds(m_schemaFreshness.GetMilliSeconds()));
          auto buffer = std::make_shared<::ndn::Buffer>(m_schemaRules.begin(), m_schemaRules.end());
          data->setContent(buffer);

          // Sign Data with default identity, send packet
          m_keyChain.sign(*data, m_signingInfo);
          sendData(data);
          return;
        }
      } else if(dataName.isPrefixOf(m_signPrefix)) {
        // request key from producer
        auto keyName = dataName.getSubName(m_signPrefix.size()).toUri();
        sendInterest(keyName, m_signLifetime);
        return;
      }
    }

    void CustomTrustAnchor::OnDataKey(std::shared_ptr<const ndn::Data> data) {
      CustomApp::OnDataKey(data);

      ::ndn::Data newData = *data;
      auto newDataPtr = std::make_shared<::ndn::Data>(newData);

      // Sign Data with default identity, send packet
      m_keyChain.sign(*newDataPtr, m_signingInfo);

      // change name to match SIGN request
      ::ndn::Name newName = m_signPrefix;
      newName.append(newDataPtr->getName());
      newDataPtr->setName(newName);

      // send data
      sendData(newDataPtr);

      // updateSchema
      auto identityName = ::ndn::security::v2::extractIdentityFromKeyName(data->getName()).toUri();
      addProducerSchema(identityName);
    }

    void CustomTrustAnchor::OnDataContent(std::shared_ptr<const ndn::Data> data) {
      CustomApp::OnDataContent(data);
    }

    //////////////////////
    //     PRIVATE
    //////////////////////

    void CustomTrustAnchor::createTrustAnchor() {
      NS_LOG_INFO("Creating Trust Anchor .cert file for '" << m_zonePrefix << "' zone ...");
      ::ndn::io::save(createCertificate(m_zonePrefix), m_trustAnchorCert);
    }

    void CustomTrustAnchor::readValidationRules() {
      // TODO read file mValidationConf into  m_schemaRules
    }

    void CustomTrustAnchor::addProducerSchema(std::string identityName) {
      // TODO update trust schema with new producer

      // inform the network about the changes in the schema
      sendDataSubscribe();
    }

    // inform network about SCHEMA UPDATES
    void CustomTrustAnchor::sendDataSubscribe() {
      auto data = std::make_shared<::ndn::Data>();
      data->setName(m_schemaSubscribePrefix);
      data->setFreshnessPeriod(::ndn::time::milliseconds(m_schemaFreshness.GetMilliSeconds()));
      data->setContent(std::make_shared<::ndn::Buffer>());
      sendData(data);
    }

  } // namespace ndn
} // namespace ns3