// custom-trust-anchor.cpp

#include "custom-trust-anchor.hpp"

#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
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
              .AddAttribute("TrustAnchorCert", "Trust Anchor .cert filename",
                            StringValue("/ndnSIM/ns-3/scratch/sim_bootsec/config/trustanchor.cert"),
                            MakeStringAccessor(&CustomTrustAnchor::m_trustAnchorCert), MakeStringChecker());
      // .AddAttribute("SchemaFreshness",
      //               "Freshness of Schema data packets, if 0, then unlimited "
      //               "freshness",
      //               TimeValue(MicroSeconds(1)),
      //               MakeTimeAccessor(&CustomTrustAnchor::m_schemaFreshness), MakeTimeChecker())
      return tid;
    }

    CustomTrustAnchor::CustomTrustAnchor() : CustomApp() {}
    CustomTrustAnchor::~CustomTrustAnchor() {}

    void CustomTrustAnchor::StartApplication() {
      NS_LOG_FUNCTION_NOARGS();
      ndn::CustomApp::StartApplication();

      // define zone KEY prefix
      m_zoneKeyPrefix = m_zonePrefix.deepCopy().append("KEY");

      // clear validation rules , create trust anchor cert file , write trust schema
      clearValidationRules();
      createTrustAnchor();
      writeValidationRules();

      // disable validation temporarily
      // TODO fix this to enable AUTH protocol authentication
      setShouldValidateData(false);

      // SCHEMA to CONSUMERS / PRODUCERS
      ndn::FibHelper::AddRoute(GetNode(), m_schemaPrefix, m_face, 0);
      NS_LOG_INFO("Monitoring prefix '" << m_schemaPrefix << "'");

      // SIGN certificates to PRODUCERS
      ndn::FibHelper::AddRoute(GetNode(), m_signPrefix, m_face, 0);
      NS_LOG_INFO("Monitoring prefix '" << m_signPrefix << "'");

      // reply for trust anchor KEY requests
      ndn::FibHelper::AddRoute(GetNode(), m_zoneKeyPrefix, m_face, 0);
      NS_LOG_INFO("Monitoring prefix '" << m_zoneKeyPrefix << "'");

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
        InterestOptions opts;
        opts.canBePrefix = true;
        opts.mustBeFresh = true;
        sendInterest(keyName, m_signLifetime, opts);
        return;
      } else if(m_zoneKeyPrefix.isPrefixOf(dataName)) {
        sendCertificate(interest);
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
        data->setFreshnessPeriod(::ndn::time::milliseconds(1));
        // data->setFreshnessPeriod(::ndn::time::milliseconds(m_schemaFreshness.GetMilliSeconds()));
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

      // get keyname and buffer info
      auto keyName = ::ndn::security::v2::extractKeyNameFromCertName(data->getName());
      auto identityName = ::ndn::security::v2::extractIdentityFromKeyName(keyName);

      // Change certificate signing name to /<prefix>/KEY/keyID/signerID/versionID
      auto rand = CreateObject<::ns3::UniformRandomVariable>();
      auto cert = std::make_shared<::ndn::Data>(*data);
      auto signerID = std::to_string((uint32_t)GetNode()->GetId());
      auto versionID = std::to_string((uint32_t)rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
      cert->setName(keyName.deepCopy().append(signerID).append(versionID));

      // sign certificate with default identity
      m_keyChain.sign(*cert, m_signingInfo);

      // create SIGN Data packet
      auto signCertPrefix = std::make_shared<::ndn::Name>(m_signPrefix.deepCopy().append(data->getName()));
      auto newData = std::make_shared<::ndn::Data>();
      newData->setName(*signCertPrefix);
      newData->setFreshnessPeriod(::ndn::time::milliseconds(1));
      newData->setContent(cert->wireEncode());

      // Sign Data with default identity, send packet
      m_keyChain.sign(*newData, m_signingInfo);
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
      // declare regexes
      std::string dataRegex, keyLocatorRegex;

      // create TRUST ANCHOR file
      NS_LOG_INFO("Creating Trust Anchor .cert file for '" << m_zonePrefix << "' zone ...");
      ::ndn::io::save(createCertificate(m_zonePrefix), m_trustAnchorCert);
      addTrustAnchor(m_trustAnchorCert);

      // add SIGN protocols
      dataRegex = "^" + getValidationRegex(m_signPrefix) + "<>*$";
      keyLocatorRegex = "^" + getValidationRegex(m_zonePrefix) + "<KEY><>{1,3}$";
      addValidationRule(dataRegex, keyLocatorRegex);

      // add SCHEMA protocols
      dataRegex = "^" + getValidationRegex(m_schemaPrefix) + "<>*$";
      keyLocatorRegex = "^" + getValidationRegex(m_zonePrefix) + "<KEY><>{1,3}$";
      addValidationRule(dataRegex, keyLocatorRegex);
    }

    void CustomTrustAnchor::addProducerSchema(const ::ndn::Name &identityName) {
      // declare regexes
      std::string dataRegex, keyLocatorRegex;

      // add Producer KEY signing verification
      NS_LOG_FUNCTION("Identity = " << identityName);
      dataRegex = "^" + getValidationRegex(identityName) + "<KEY><>{1,3}$";
      keyLocatorRegex = "^" + getValidationRegex(m_zonePrefix) + "<KEY><>{1,3}$";
      addValidationRule(dataRegex, keyLocatorRegex);

      // add Producer APP signing verification
      dataRegex = "^" + getValidationRegex(identityName) + "[^<KEY>]*$";
      keyLocatorRegex = "^" + getValidationRegex(identityName) + "<KEY><>{1,3}$";
      addValidationRule(dataRegex, keyLocatorRegex);

      // inform the network about the changes in the schema
      sendDataSubscribe();
    }

    // reply with SCHEMA/SUBCRIBE
    void CustomTrustAnchor::sendDataSubscribe() {
      auto data = std::make_shared<::ndn::Data>();
      data->setName(m_schemaSubscribePrefix);
      data->setFreshnessPeriod(::ndn::time::milliseconds(1));
      data->setContent(std::make_shared<::ndn::Buffer>());

      // Sign Data with default identity, send packet
      m_keyChain.sign(*data, m_signingInfo);
      sendData(data);
    }

  } // namespace ndn
} // namespace ns3