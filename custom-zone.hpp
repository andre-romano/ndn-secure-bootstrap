#ifndef CUSTOM_ZONE_H
#define CUSTOM_ZONE_H

// default libraries
#include <functional>
#include <map>
#include <string>
#include <vector>

// ns3 modules
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-net-device.h"
// #include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

// ndn-cxx
#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/util/io.hpp"

// custom and auxiliary
#include "custom-consumer.hpp"
#include "custom-producer.hpp"

namespace ns3 {
  namespace ndn {

    class CustomZone {

    public:
      CustomZone(std::string zoneName);
      CustomZone(std::string zoneName, int n_Consumers, int n_Producers);
      ~CustomZone();

      std::shared_ptr<NodeContainer> getConsumers();
      std::shared_ptr<NodeContainer> getProducers();

      void addConsumers(int n);
      void addProducers(int n);

      void installAllConsumerApps();
      void installAllProducerApps();

    private:
      void setupTrustAnchor();
      void signProducerCertificates(std::shared_ptr<ns3::ApplicationContainer> producerApps);

      void installConsumerApp(string prefix, string lifetime, double pktFreq, string randomize);
      void installProducerApp(string prefix, double freshness, string payloadSize);

    private:
      std::string m_zoneName;

      std::string m_trustAnchorCert;
      std::string m_validatorConf;

      std::shared_ptr<NodeContainer> m_consumers;
      std::shared_ptr<NodeContainer> m_producers;

      // Name m_identityPrefix;
      // Name m_trustAnchorIdentityPrefix;
      // std::string m_trustAnchorCertFilename;

      std::shared_ptr<::ndn::security::v2::KeyChain> m_keyChain;
      // ::ndn::security::SigningInfo m_signingInfo;
    };
  } // namespace ndn
} // namespace ns3

#endif /* CUSTOM_ZONE_H */