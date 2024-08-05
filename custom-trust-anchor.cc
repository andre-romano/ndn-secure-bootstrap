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
              .SetParent<CustomProducer>()
              .AddConstructor<CustomTrustAnchor>()
              .AddAttribute("SchemaPrefix", "Schema prefix (to request trust schema file)",
                            StringValue("/SCHEMA"), MakeStringAccessor(&CustomTrustAnchor::m_schemaPrefix),
                            MakeStringChecker())
              .AddAttribute("TrustAnchorCert", "Trust Anchor .cert filename",
                            StringValue("./scratch/sim_bootsec/config/trustanchor.cert"),
                            MakeStringAccessor(&CustomTrustAnchor::m_trustAnchorCert), MakeStringChecker())
              .AddAttribute("ValidatorConf", "Validator config filename",
                            StringValue("./scratch/sim_bootsec/config/validator.conf"),
                            MakeStringAccessor(&CustomTrustAnchor::m_validatorFilename), MakeStringChecker());
      return tid;
    }

    CustomTrustAnchor::CustomTrustAnchor() : CustomProducer() {}

    CustomTrustAnchor::~CustomTrustAnchor() {}

    void CustomTrustAnchor::StartApplication() {
      CustomProducer::StartApplication();

      // create trust anchor cert file (public key)
      setupTrustAnchorCertFile();

      // serve trust schema file content to consumers
      m_schemaPrefix.append("/content");
      ndn::FibHelper::AddRoute(GetNode(), m_schemaPrefix, m_face, 0);
      NS_LOG_INFO("Monitoring: " << m_schemaPrefix);

      // NS_LOG_DEBUG(cert);
      printKeyChain();
    }

    void CustomTrustAnchor::StopApplication() {
      CustomProducer::StopApplication();
      // no additional code needed
    }

    void CustomTrustAnchor::OnInterestKey(std::shared_ptr<const ndn::Interest> interest) {
      NS_LOG_FUNCTION(interest->getName());
      // serve trust anchor public certificate
      CustomProducer::OnInterestKey(interest);
      // no additional code needed
    }

    void CustomTrustAnchor::OnInterestContent(std::shared_ptr<const ndn::Interest> interest) {
      NS_LOG_FUNCTION(interest->getName());

      Name dataName(interest->getName());
      if(!dataName.isPrefixOf(m_schemaPrefix)) {
        // only process TRUST SCHEMA requests
        return;
      }
      // send trust schema to nodes
      auto data = make_shared<Data>();
      data->setName(dataName);
      data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

      NS_LOG_INFO("Reading trust schema file '" << m_validatorFilename << "' ...");
      auto fileStream = std::ifstream(m_validatorFilename, std::ios::in);
      data->setContent(::ndn::io::loadBuffer(fileStream, ::ndn::io::IoEncoding::NO_ENCODING));

      // Sign Data packet with default identity and create real wire encoding
      m_keyChain.sign(*data, m_signingInfo);
      data->wireEncode();

      // Call trace (for logging purposes)
      NS_LOG_INFO("Sending Data packet: " << data->getName());
      m_transmittedDatas(data, this, m_face);
      m_appLink->onReceiveData(*data);
    }

    //////////////////////
    //     PRIVATE
    //////////////////////

    void CustomTrustAnchor::setupTrustAnchorCertFile() {
      // NS_LOG_INFO("Creating Trust Anchor .cert file for '" << m_identityPrefix << "' zone ...");
      // auto rootIdentity = m_keyChain.getPib().getIdentity(m_identityPrefix);
      // ::ndn::io::save(rootIdentity.getDefaultKey().getDefaultCertificate(), m_trustAnchorCert);
    }

  } // namespace ndn
} // namespace ns3