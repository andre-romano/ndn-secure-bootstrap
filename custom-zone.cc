
#include "custom-zone.hpp"

NS_LOG_COMPONENT_DEFINE("CustomZone");

using namespace std;

namespace ns3 {
  namespace ndn {

    CustomZone::CustomZone(string zoneName)
        : m_zoneName(zoneName), m_schemaPrefix(m_zoneName + "/SCHEMA"), m_signPrefix(m_zoneName + "/SIGN"),
          m_trustAnchorCert("/ndnSIM/ns-3/scratch/sim_bootsec/config" + m_zoneName + "_trustanchor.cert"),
          m_validatorConf("/ndnSIM/ns-3/scratch/sim_bootsec/config" + m_zoneName + "_validator.conf"),
          m_consumers(make_shared<NodeContainer>()), m_producers(make_shared<NodeContainer>()),
          m_trust_anchors(make_shared<NodeContainer>()) {
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

    void CustomZone::installConsumerApp(string prefix, string lifetime, double pktFreq, string randomize,
                                        uint32_t consumerID) {
      auto prefixApp = m_zoneName + prefix;
      NS_LOG_INFO("Installing Consumer App for '" << prefixApp << "' ...");
      // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
      ndn::AppHelper consumerHelper("CustomConsumer");
      // consumer CBR interest/second
      consumerHelper.SetPrefix(prefixApp);
      consumerHelper.SetAttribute("Frequency", DoubleValue(pktFreq));
      consumerHelper.SetAttribute("LifeTime", StringValue(lifetime));
      // consumer randomize send time
      consumerHelper.SetAttribute("Randomize", StringValue(randomize));
      // (inherited - CustomApp)
      consumerHelper.SetAttribute("SignPrefix", StringValue(m_signPrefix));
      consumerHelper.SetAttribute("SchemaPrefix", StringValue(m_schemaPrefix));
      consumerHelper.SetAttribute("ValidatorConf", StringValue(m_validatorConf));
      // set random start time
      Ptr<UniformRandomVariable> start_time = CreateObject<UniformRandomVariable>();
      // consumer RANDOM start time interval (MIN, MAX)
      auto consumerPtr = m_consumers->Get(consumerID);
      consumerHelper.Install(consumerPtr).Start(Seconds(start_time->GetValue(0.2, 0.75)));
    }

    void CustomZone::installProducerApp(string prefix, double freshness, string payloadSize,
                                        uint32_t producerID) {
      auto prefixApp = m_zoneName + prefix;
      // ndn::AppHelper producerHelper("ns3::ndn::Producer");
      NS_LOG_INFO("Installing Producer App for '" << prefixApp << "' ...");
      ndn::AppHelper producerHelper("CustomProducer");
      producerHelper.SetPrefix(prefixApp);                                  // ndn prefix
      producerHelper.SetAttribute("PayloadSize", StringValue(payloadSize)); // payload MTU
      producerHelper.SetAttribute("Freshness", TimeValue(Seconds(freshness)));
      // (inherited - CustomApp)
      producerHelper.SetAttribute("SignPrefix", StringValue(m_signPrefix));
      producerHelper.SetAttribute("SchemaPrefix", StringValue(m_schemaPrefix));
      producerHelper.SetAttribute("ValidatorConf", StringValue(m_validatorConf));
      auto producerPtr = m_producers->Get(producerID);
      auto producersApps = std::make_shared<ns3::ApplicationContainer>(producerHelper.Install(producerPtr));
      producersApps->Start(Seconds(0.1)); // producers start time
    }

    void CustomZone::installTrustAnchorApp(double freshness) {
      // ndn::AppHelper producerHelper("ns3::ndn::Producer");
      NS_LOG_INFO("Installing Trust Anchor App for zone '" << m_zoneName << "' ...");
      ndn::AppHelper trustAnchorHelper("CustomTrustAnchor");
      trustAnchorHelper.SetAttribute("ZonePrefix", StringValue(m_zoneName));
      trustAnchorHelper.SetAttribute("TrustAnchorCert",
                                     StringValue(m_trustAnchorCert)); // trust anchor .cert FILE
      // trustAnchorHelper.SetAttribute("SchemaFreshness",
      //                                TimeValue(Seconds(freshness))); // freshness for trust schema file
      // // (inherited - CustomApp)
      trustAnchorHelper.SetAttribute("SignPrefix", StringValue(m_signPrefix));
      trustAnchorHelper.SetAttribute("SchemaPrefix", StringValue(m_schemaPrefix));
      trustAnchorHelper.SetAttribute("ValidatorConf", StringValue(m_validatorConf));
      auto trustAnchorApps =
          std::make_shared<ns3::ApplicationContainer>(trustAnchorHelper.Install(*m_trust_anchors));
      trustAnchorApps->Start(Seconds(0.05)); // trust anchors start time
    }

    //////////////////////
    //     PRIVATE
    //////////////////////

    void CustomZone::addTrustAnchor() { m_trust_anchors->Create(1); }

  } // namespace ndn
} // namespace ns3