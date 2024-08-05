
#include "custom-zone.hpp"

NS_LOG_COMPONENT_DEFINE("CustomZone");

using namespace std;

namespace ns3 {
  namespace ndn {

    CustomZone::CustomZone(string zoneName)
        : m_zoneName(zoneName), m_schemaPrefix(m_zoneName + "/SCHEMA"),
          m_trustAnchorCert("scratch/sim_bootsec/config" + m_zoneName + "_trustanchor.cert"),
          m_validatorConf("scratch/sim_bootsec/config" + m_zoneName + "_validator.conf"),
          m_consumers(make_shared<NodeContainer>()), m_producers(make_shared<NodeContainer>()),
          m_trust_anchors(make_shared<NodeContainer>()),
          m_keyChain(std::make_shared<::ndn::security::v2::KeyChain>("pib-memory:", "tpm-memory:")) {
      addTrustAnchor();
    }
    CustomZone::CustomZone(string zoneName, int n_Producers, int n_Consumers) : CustomZone(zoneName) {
      addProducers(n_Producers);
      addConsumers(n_Consumers);
    }

    CustomZone::~CustomZone() {}

    shared_ptr<NodeContainer> CustomZone::getTrustAnchors() { return m_trust_anchors; }
    shared_ptr<NodeContainer> CustomZone::getProducers() { return m_producers; }
    shared_ptr<NodeContainer> CustomZone::getConsumers() { return m_consumers; }

    void CustomZone::addConsumers(int n) { m_consumers->Create(n); }
    void CustomZone::addProducers(int n) { m_producers->Create(n); }

    void CustomZone::installAllTrustAnchorApps() {
      // create TRUST ANCHOR and SCHEMA for zone
      installTrustAnchorApp(2.0);
    }
    void CustomZone::installAllProducerApps() {
      // create PRODUCER for prefix "/test/prefix"
      installProducerApp("/test/prefix", 2.0, "1480");
    }
    void CustomZone::installAllConsumerApps() {
      // create CONSUMER for prefix "/test/prefix"
      installConsumerApp("/test/prefix", "1s", 10, "uniform");
    }

    //////////////////////
    //     PRIVATE
    //////////////////////

    void CustomZone::addTrustAnchor() {
      m_trust_anchors->Create(1);
      // TODO remove code below
      NS_LOG_INFO("Creating Trust Anchor for '" << m_zoneName << "' zone ...");
      auto rootIdentity = m_keyChain->createIdentity(m_zoneName);
      ::ndn::io::save(rootIdentity.getDefaultKey().getDefaultCertificate(), m_trustAnchorCert);
      //   NS_LOG_INFO("--> Root Identity: " << rootIdentity);
      NS_LOG_INFO("--> Root Key Type: " << rootIdentity.getDefaultKey().getKeyType());
      NS_LOG_INFO(rootIdentity.getDefaultKey().getDefaultCertificate());
    }

    void CustomZone::signProducerCertificates(std::shared_ptr<ns3::ApplicationContainer> producerApps) {
      //   sign producer certificates with trust anchor
      auto certValidity = 365; // in days
      auto rootIdentity =
          std::make_shared<::ndn::security::pib::Identity>(m_keyChain->getPib().getIdentity(m_zoneName));
      auto thisKeychain = m_keyChain;
      for(auto it = producerApps->Begin(); it != producerApps->End(); it++) {
        auto customProducerApp = DynamicCast<ndn::CustomProducer>(*it);
        if(!customProducerApp) {
          NS_LOG_LOGIC("CustomProducer App not found!");
          continue;
        }
        // this callback signs the certificate as it is created by the Producer
        // (upon Producer App start in NS3)
        customProducerApp->setSignCallback([=](::ndn::security::v2::Certificate cert) {
          ::ndn::SignatureInfo signatureInfo;
          signatureInfo.setValidityPeriod(::ndn::security::ValidityPeriod(
              ndn::time::system_clock::TimePoint(),
              ndn::time::system_clock::now() + ndn::time::days(certValidity)));
          thisKeychain->sign(cert,
                             ::ndn::security::SigningInfo(*rootIdentity).setSignatureInfo(signatureInfo));
          return cert;
        });
      }
    }

    void CustomZone::installConsumerApp(string prefix, string lifetime, double pktFreq, string randomize) {
      auto prefixApp = m_zoneName + prefix;
      NS_LOG_INFO("Installing Consumer App for '" << prefixApp << "' ...");
      // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
      ndn::AppHelper consumerHelper("CustomConsumer");
      consumerHelper.SetAttribute("LifeTime", StringValue(lifetime));
      // consumer CBR interest/second
      consumerHelper.SetAttribute("Frequency", DoubleValue(pktFreq));
      // consumer randomize send time
      consumerHelper.SetAttribute("Randomize", StringValue(randomize));
      consumerHelper.SetAttribute("SchemaPrefix", StringValue(m_schemaPrefix));
      consumerHelper.SetAttribute("ValidatorConf", StringValue(m_validatorConf));
      consumerHelper.SetPrefix(prefixApp);
      // set random start time
      for(auto &consumer : *m_consumers) {
        Ptr<UniformRandomVariable> start_time = CreateObject<UniformRandomVariable>();
        // consumer RANDOM start time interval (MIN, MAX)
        consumerHelper.Install(consumer).Start(Seconds(start_time->GetValue(0.2, 0.75)));
      }
    }

    void CustomZone::installProducerApp(string prefix, double freshness, string payloadSize) {
      auto prefixApp = m_zoneName + prefix;
      // ndn::AppHelper producerHelper("ns3::ndn::Producer");
      NS_LOG_INFO("Installing Producer App for '" << prefixApp << "' ...");
      ndn::AppHelper producerHelper("CustomProducer");
      producerHelper.SetAttribute("Freshness", TimeValue(Seconds(freshness)));
      producerHelper.SetAttribute("PayloadSize", StringValue(payloadSize)); // payload MTU
      producerHelper.SetPrefix(prefixApp);                                  // ndn prefix
      auto producersApps = std::make_shared<ns3::ApplicationContainer>(producerHelper.Install(*m_producers));
      producersApps->Start(Seconds(0.1)); // producers start time
      // Sign Certificates with Trust Anchor (after they are created inside the Producers)
      signProducerCertificates(producersApps);
    }

    void CustomZone::installTrustAnchorApp(double freshness) {
      // ndn::AppHelper producerHelper("ns3::ndn::Producer");
      NS_LOG_INFO("Installing Trust Anchor App for zone '" << m_zoneName << "' ...");
      ndn::AppHelper trustAnchorHelper("CustomTrustAnchor");
      trustAnchorHelper.SetAttribute("Freshness",
                                     TimeValue(Seconds(freshness))); // freshness for trust schema file
      trustAnchorHelper.SetAttribute("TrustAnchorCert",
                                     StringValue(m_trustAnchorCert)); // trust anchor .cert FILE
      trustAnchorHelper.SetAttribute("ValidatorConf", StringValue(m_validatorConf)); // validator .conf FILE
      trustAnchorHelper.SetAttribute("SchemaPrefix", StringValue(m_schemaPrefix));   // SCHEMA prefix
      trustAnchorHelper.SetPrefix(m_zoneName);                                       // ndn ZONE prefix
      auto trustAnchorApps =
          std::make_shared<ns3::ApplicationContainer>(trustAnchorHelper.Install(*m_trust_anchors));
      trustAnchorApps->Start(Seconds(0.05)); // trust anchors start time
    }

  } // namespace ndn
} // namespace ns3