/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <map>
#include <memory>
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
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-net-device.h"
// #include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

// ENERGY MODULES
#include "ns3/basic-energy-source-helper.h"
#include "ns3/basic-energy-source.h"
#include "ns3/device-energy-model-container.h"
#include "ns3/energy-module.h"
#include "ns3/energy-source-container.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/wifi-radio-energy-model.h"

// topology
#include "ns3/inet-topology-reader.h"
#include "ns3/orbis-topology-reader.h"
#include "ns3/rocketfuel-topology-reader.h"
#include "ns3/topology-reader-helper.h"

// TRACES
// #include "ns3/gnuplot.h"
#include "ns3/ndnSIM/utils/tracers/l2-rate-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-cs-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.hpp"

// NDNSIM NFD / MODEL / ETC
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/best-route-strategy.hpp"
#include "ns3/ndnSIM/NFD/daemon/table/cs-policy-lru.hpp"
#include "ns3/ndnSIM/NFD/daemon/table/cs-policy-priority-fifo.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/model/ndn-net-device-transport.hpp"

// ndn-cxx
#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/util/io.hpp"

// custom and auxiliary
#include "custom-consumer.hpp"
#include "custom-producer.hpp"
#include "custom-tracer.hpp"
#include "custom-zone.hpp"

NS_LOG_COMPONENT_DEFINE("sim_bootsec");

using namespace std;

namespace ns3 {

  std::string constructFaceUri(Ptr<NetDevice> netDevice) {

    std::string uri = "netdev://";
    Address address = netDevice->GetAddress();
    if(Mac48Address::IsMatchingType(address)) {
      uri += "[" + boost::lexical_cast<std::string>(Mac48Address::ConvertFrom(address)) + "]";
    }

    NS_LOG_DEBUG("constructFaceUri( netDev = " << netDevice << ") - URI = " << uri);

    return uri;
  }

  int main(int argc, char *argv[]) {

    // setting default parameters for PointToPoint links and channels
    Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("100Mbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));

    // get commands from user
    CommandLine cmd;
    uint32_t nSimDuration = 11;
    std::string nTraceFile = "results/mobility-trace.ns_movements";
    double nInitialEnergy = 20.0;
    size_t nCsSize = 1;
    size_t n_Forwarders = 1;
    cmd.AddValue("nSimDuration", "Simulation duration ", nSimDuration);
    cmd.AddValue("nTraceFile", "Ns2 movement trace file", nTraceFile);
    cmd.AddValue("nInitialEnergy", "Initial energy of the nodes", nInitialEnergy);
    cmd.AddValue("nCsSize", "Content Store size", nCsSize);
    cmd.AddValue("n_Forwarders", "Number of NDN Forwarders", n_Forwarders);
    cmd.Parse(argc, argv);

    // parse str commands into enums
    NS_LOG_UNCOND("TraceFile = " << nTraceFile);

    //////////////////////
    //     NDN STACK
    //////////////////////
    NS_LOG_INFO("Setup NDN Stack ...");
    ndn::StackHelper ndnHelper;
    // ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId(),
    // MakeCallback(MyNetDeviceFaceCallback));
    ndnHelper.setPolicy("nfd::cs::lru");
    ndnHelper.setCsSize(nCsSize); // forwarder->getCs().setLimit(nCsSize);
    ndnHelper.SetDefaultRoutes(true);

    //////////////////////
    //     NDN ZONES
    //////////////////////
    NS_LOG_INFO("Create NDN Zones ...");
    std::map<string, std::shared_ptr<ndn::CustomZone>> ndnZones;
    // ZONE A => 1 consumer + 1 producer
    std::string zoneName = "/zoneA";
    ndnZones[zoneName] = std::make_shared<ndn::CustomZone>(zoneName, 1, 1);
    // ZONE B => 1 consumer + 1 producer
    // zoneName = "/zoneB";
    // ndnZones[zoneName] = std::make_shared<ndn::CustomZone>(zoneName, 1, 1);

    //////////////////////
    //     NODES
    //////////////////////
    NS_LOG_INFO("Create NS3 Nodes (consumers, fwd, producers) ...");
    NodeContainer nodes, consumers, producers, forwarders, trust_anchors;
    for(const auto &pairNameZone : ndnZones) {
      consumers.Add(*pairNameZone.second->getConsumers());
      producers.Add(*pairNameZone.second->getProducers());
      trust_anchors.Add(*pairNameZone.second->getTrustAnchors());
    }
    // every node in the network can forward NDN packets (full adhoc WiFi)
    if(n_Forwarders > 0) {
      forwarders.Create(n_Forwarders);
    }
    // add all nodes in the network
    nodes.Add(consumers);
    nodes.Add(producers);
    nodes.Add(forwarders);
    nodes.Add(trust_anchors);

    //////////////////////
    //     INSTALL
    //////////////////////

    // 1. Install Links
    NS_LOG_INFO("Installing P2P links ...");
    ::ns3::PointToPointHelper p2p;
    for(auto consumer : consumers) {
      for(auto producer : producers) {
        p2p.Install(consumer, producer);
      }
    }
    for(auto trust_anchor : trust_anchors) {
      for(auto producer : producers) {
        p2p.Install(trust_anchor, producer);
      }
    }
    for(auto trust_anchor : trust_anchors) {
      for(auto forwarder : forwarders) {
        p2p.Install(trust_anchor, forwarder);
      }
    }

    // 4. Install NDN stack
    NS_LOG_INFO("Installing NDN stack ...");
    ndnHelper.Install(nodes);

    // 5. Set fw strategy
    NS_LOG_INFO("Installing NDN Forwarding Strategies ...");
    ndn::StrategyChoiceHelper::Install(consumers, "/", "/localhost/nfd/strategy/multicast");
    ndn::StrategyChoiceHelper::Install(forwarders, "/", "/localhost/nfd/strategy/multicast");
    ndn::StrategyChoiceHelper::Install(producers, "/", "/localhost/nfd/strategy/multicast");
    ndn::StrategyChoiceHelper::Install(trust_anchors, "/", "/localhost/nfd/strategy/multicast");

    // 6.0. Set up TRUST ANCHOR
    NS_LOG_INFO("Installing Trust Anchor (and Schema) Apps in Zones ...");
    for(const auto &pairNameZone : ndnZones) {
      pairNameZone.second->installAllTrustAnchorApps();
    }

    // 6.1. Set up CONSUMER
    NS_LOG_INFO("Installing Consumer Apps in Zones ...");
    for(const auto &pairNameZone : ndnZones) {
      pairNameZone.second->installAllConsumerApps();
    }

    // 6.2. Set up PRODUCER
    NS_LOG_INFO("Installing Producer Apps in Zones ...");
    for(const auto &pairNameZone : ndnZones) {
      pairNameZone.second->installAllProducerApps();
    }

    // 7.1. Simulate link failures (P2P links only)
    // Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    // em->SetAttribute("ErrorRate", DoubleValue(nErrorRate));
    // em->SetAttribute("ErrorUnit",
    // EnumValue(RateErrorModel::ERROR_UNIT_PACKET)); for (auto node :
    // nodes) {
    //     node->SetAttribute("ReceptionErrorModel", PointerValue(em));
    // }

    // 7.2. Monitor data traces and other interesting variables
    NS_LOG_INFO("Installing NDN Tracers ...");
    ndn::AppDelayTracer::InstallAll("results/app-delays-trace.txt");
    ndn::L3RateTracer::InstallAll("results/rate-trace.txt", Seconds(1.0));
    ndn::CsTracer::InstallAll("results/cs-trace.txt", Seconds(1));

    NS_LOG_INFO("Installing Custom Tracers ...");
    auto customTracer = CreateObject<CustomTracer>();
    customTracer->SetAttribute("TraceFilename", StringValue("results/dataCustomCons.dat"));
    customTracer->SetAttribute("NodesToMonitor", NodeContainerValue(consumers));

    auto customTracerFwd = CreateObject<CustomTracer>();
    customTracerFwd->SetAttribute("TraceFilename", StringValue("results/dataCustomFwd.dat"));
    customTracerFwd->SetAttribute("NodesToMonitor", NodeContainerValue(forwarders));

    auto customTracerProd = CreateObject<CustomTracer>();
    customTracerProd->SetAttribute("TraceFilename", StringValue("results/dataCustomProd.dat"));
    customTracerProd->SetAttribute("NodesToMonitor", NodeContainerValue(producers));

    auto customTracerAnchor = CreateObject<CustomTracer>();
    customTracerAnchor->SetAttribute("TraceFilename", StringValue("results/dataCustomAnchor.dat"));
    customTracerAnchor->SetAttribute("NodesToMonitor", NodeContainerValue(trust_anchors));

    // 8. Start simulation
    NS_LOG_INFO("Start simulation!");
    Simulator::Stop(Seconds(nSimDuration));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
  }
} // namespace ns3

int main(int argc, char *argv[]) {
  const int MAX_NUM_SIM = 1; // MAX NUM OF SIMULATIONS
  for(int idx = MAX_NUM_SIM; idx > 0; idx--) {
    // ns3::RngSeedManager::SetRun(idx); // set random run for replications
    ns3::main(argc, argv); // run simulation
  }
  return 0;
}
