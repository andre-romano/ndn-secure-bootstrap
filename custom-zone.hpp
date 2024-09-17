#ifndef CUSTOM_ZONE_H
#define CUSTOM_ZONE_H

// default libraries
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <memory>

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
#include "custom-trust-anchor.hpp"

namespace ns3 {
  namespace ndn {

    class CustomZone {

    public:
      CustomZone(std::string zoneName);
      CustomZone(std::string zoneName, int n_Producers, int n_Consumers);
      ~CustomZone();

      std::shared_ptr<NodeContainer> getTrustAnchors();
      std::shared_ptr<NodeContainer> getProducers();
      std::shared_ptr<NodeContainer> getConsumers();

      void addProducers(int n);
      void addConsumers(int n);

      void installAllTrustAnchorApps();
      void installAllProducerApps();
      void installAllConsumerApps();

    private:
      void addTrustAnchor();

      void signProducerCertificates(std::shared_ptr<ns3::ApplicationContainer> producerApps);

      void installTrustAnchorApp(double freshness);
      void installProducerApp(string prefix, double freshness, string payloadSize);
      void installConsumerApp(string prefix, string lifetime, double pktFreq, string randomize);

    private:
      std::string m_zoneName;
      std::string m_schemaPrefix;
      std::string m_signPrefix;

      std::string m_trustAnchorCert;
      std::string m_validatorConf;

      std::shared_ptr<NodeContainer> m_consumers;
      std::shared_ptr<NodeContainer> m_producers;
      std::shared_ptr<NodeContainer> m_trust_anchors;

      std::shared_ptr<::ndn::security::v2::KeyChain> m_keyChain;
    };
  } // namespace ndn
} // namespace ns3

#endif /* CUSTOM_ZONE_H */