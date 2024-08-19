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
                            MakeNameAccessor(&CustomTrustAnchor::m_zonePrefix), MakeNameChecker())
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

      // clear validation rules , create trust anchor cert file , write trust schema
      clearValidationRules();
      createTrustAnchor();
      writeValidationRules();

      // disable validation temporarily
      // TODO fix validation in trust anchor
      setShouldValidateData(false);

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
      NS_LOG_FUNCTION(interest->getName());
      ndn::CustomApp::OnInterestKey(interest);

      // define Data packet response
      auto dataName = interest->getName();
      auto data = make_shared<Data>();
      data->setName(dataName);

      if(m_signPrefix.isPrefixOf(dataName)) {
        // request key from producer
        auto keyName = dataName.getSubName(m_signPrefix.size(), dataName.size() - m_signPrefix.size());
        NS_LOG_INFO("Sending KEY request for '" << keyName << "' ...");
        sendInterest(keyName, m_signLifetime, true);
        return;
      }
    }

    void CustomTrustAnchor::OnInterestContent(std::shared_ptr<const ndn::Interest> interest) {
      NS_LOG_FUNCTION(interest->getName());
      ndn::CustomApp::OnInterestContent(interest);

      // define Data packet response
      auto dataName = interest->getName();
      auto data = make_shared<Data>();
      data->setName(dataName);

      if(m_schemaContentPrefix.isPrefixOf(dataName)) {
        NS_LOG_INFO("Sending SCHEMA content for '" << m_schemaContentPrefix << "' ...");
        data->setFreshnessPeriod(::ndn::time::milliseconds(m_schemaFreshness.GetMilliSeconds()));
        std::string schemaRules = getValidationRules();
        auto buffer = std::make_shared<::ndn::Buffer>(schemaRules.begin(), schemaRules.end());
        data->setContent(buffer);

        // Sign Data with default identity, send packet
        m_keyChain.sign(*data, m_signingInfo);
        sendData(data);
        return;
      }
    }

    void CustomTrustAnchor::OnDataCertificate(std::shared_ptr<const ndn::Data> data) {
      NS_LOG_FUNCTION(data->getName());
      CustomApp::OnDataCertificate(data);

      // Sign certificate with default identity
      auto cert = std::make_shared<::ndn::Data>(*data);
      m_keyChain.sign(*cert, m_signingInfo);

      // get keyname and buffer info
      auto keyName = ::ndn::security::v2::extractKeyNameFromCertName(cert->getName());
      auto identityName = ::ndn::security::v2::extractIdentityFromKeyName(keyName);

      // create SIGN Data packet
      // TODO fix code below (data packet not being recv in neighbors)
      auto newData = std::make_shared<::ndn::Data>();
      // newData->setName("/zoneA/SIGN/zoneA/test/prefix/KEY");
      newData->setName(m_signPrefix.deepCopy().append(cert->getName()));
      newData->setFreshnessPeriod(::ndn::time::milliseconds(2000));
      // newData->setFreshnessPeriod(::ndn::time::milliseconds(m_signLifetime.GetMilliSeconds()));
      // newData->setContent(cert->wireEncode());
      newData->setContent(std::make_shared<::ndn::Buffer>(1024));

      // Sign Data with default identity, send packet
      m_keyChain.sign(*newData, m_signingInfo);
      // Simulator::Schedule(Seconds(0.0), &CustomTrustAnchor::sendData, this, newData);
      sendData(newData);

      // updateSchema
      addProducerSchema(identityName);
    }

    void CustomTrustAnchor::OnDataContent(std::shared_ptr<const ndn::Data> data) {
      NS_LOG_FUNCTION(data->getName());
      CustomApp::OnDataContent(data);
    }

    //////////////////////
    //     PRIVATE
    //////////////////////

    void CustomTrustAnchor::createTrustAnchor() {
      NS_LOG_INFO("Creating Trust Anchor .cert file for '" << m_zonePrefix << "' zone ...");
      ::ndn::io::save(createCertificate(m_zonePrefix), m_trustAnchorCert);
      addTrustAnchor(m_trustAnchorCert);
    }

    void CustomTrustAnchor::addProducerSchema(::ndn::Name identityName) {
      std::string dataRegex = "^" + getValidationRegex(identityName) + getKeySuffixRegex();
      std::string keyLocatorRegex = "^" + getValidationRegex(m_zonePrefix) + getKeySuffixRegex();
      addValidationRule(dataRegex, keyLocatorRegex);

      // inform the network about the changes in the schema
      sendDataSubscribe();
    }

    // inform network about SCHEMA UPDATES
    void CustomTrustAnchor::sendDataSubscribe() {
      auto data = std::make_shared<::ndn::Data>();
      data->setName(m_schemaSubscribePrefix);
      data->setFreshnessPeriod(::ndn::time::milliseconds(m_schemaFreshness.GetMilliSeconds()));
      data->setContent(std::make_shared<::ndn::Buffer>());

      // Sign Data with default identity, send packet
      m_keyChain.sign(*data, m_signingInfo);
      sendData(data);
    }

  } // namespace ndn
} // namespace ns3