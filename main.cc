/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <string>

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
#include "custom-consumer-boot.hpp"
#include "custom-producer-boot.hpp"
#include "custom-tracer-boot.hpp"

NS_LOG_COMPONENT_DEFINE("sim_bootsec");

using namespace std;

namespace ns3 {

    std::string
    constructFaceUri(Ptr<NetDevice> netDevice) {

        std::string uri = "netdev://";
        Address address = netDevice->GetAddress();
        if (Mac48Address::IsMatchingType(address)) {
            uri += "[" + boost::lexical_cast<std::string>(Mac48Address::ConvertFrom(address)) + "]";
        }

        NS_LOG_DEBUG("constructFaceUri( netDev = " << netDevice << ") - URI = " << uri);

        return uri;
    }

    shared_ptr<nfd::face::Face>
    addFaceWifiNetDeviceCallback(Ptr<Node> node, Ptr<ndn::L3Protocol> ndn,
                                 Ptr<NetDevice> netDevice) {
        // Create an ndnSIM-specific transport instance
        ::nfd::face::GenericLinkService::Options opts;
        opts.allowFragmentation     = true;
        opts.allowReassembly        = true;
        opts.allowCongestionMarking = true;
        // NS_LOG_DEBUG("CREATING WIFI NETDEV FACE");

        auto linkService = make_unique<::nfd::face::GenericLinkService>(opts);
        // auto linkService = make_unique<::nfd::face::LinkService>(opts);

        auto linkType = ::ndn::nfd::LINK_TYPE_POINT_TO_POINT;
        if (netDevice->IsPointToPoint() == 0)
            linkType = ::ndn::nfd::LINK_TYPE_MULTI_ACCESS;
        // check for Adhoc communication
        auto wifiNetDev = dynamic_cast<ns3::WifiNetDevice *>(&(*netDevice));
        if (wifiNetDev != NULL) {
            auto wifiMac = wifiNetDev->GetMac();
            if (dynamic_cast<ns3::AdhocWifiMac *>(&(*wifiMac)) != NULL)
                linkType = ::ndn::nfd::LINK_TYPE_AD_HOC;
        }

        auto transport =
            make_unique<ndn::NetDeviceTransport>(node, netDevice, constructFaceUri(netDevice),
                                                 "netdev://[ff:ff:ff:ff:ff:ff]",
                                                 ::ndn::nfd::FACE_SCOPE_NON_LOCAL,
                                                 ::ndn::nfd::FACE_PERSISTENCY_PERSISTENT, linkType);

        auto face = std::make_shared<nfd::face::Face>(std::move(linkService), std::move(transport));
        face->setMetric(1);

        ndn->addFace(face);
        // NS_LOG_LOGIC("Node " << node->GetId() << ": added Face as face #" << face->getLocalUri()
        //                      << " - remoteURI = " << face->getRemoteUri() << " - linkType " << linkType);

        return face;
    }

    int main(int argc, char *argv[]) {
        // disable fragmentation
        StringValue wifiRate("OfdmRate24Mbps");
        Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
        Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", wifiRate);

        // get commands from user
        CommandLine cmd;
        uint32_t nSimDuration        = 11;
        std::string nTraceFile       = "results/mobility-trace.ns_movements";
        std::string nValidatorConf   = "scratch/sim_bootsec/config/validator.conf";
        std::string nTrustAnchorCert = "scratch/sim_bootsec/config/trustanchor.cert";
        double nInitialEnergy        = 20.0;
        double nPktFreq              = 20.0;
        std::string nLifetime        = "2s";
        double nFreshness            = 2.0;
        uint32_t nNodes              = 20;
        uint32_t nConsumers          = 2;
        uint32_t nProducers          = 1;
        size_t nCsSize               = 1000;
        cmd.AddValue("nSimDuration", "Simulation duration ", nSimDuration);
        cmd.AddValue("nTraceFile", "Ns2 movement trace file", nTraceFile);
        cmd.AddValue("nValidatorConf", "Validator config filename", nValidatorConf);
        cmd.AddValue("nTrustAnchorCert", "Trust anchor certificate filename", nTrustAnchorCert);
        cmd.AddValue("nInitialEnergy", "Initial energy of the nodes", nInitialEnergy);
        cmd.AddValue("nPktFreq", "Interest send frequency", nPktFreq);
        cmd.AddValue("nLifetime", "NDN Interest lifetime", nLifetime);
        cmd.AddValue("nFreshness", "NDN Data freshness", nFreshness);
        cmd.AddValue("nNodes", "Number of Nodes", nNodes);
        cmd.AddValue("nConsumers", "Number of Consumers", nConsumers);
        cmd.AddValue("nProducers", "Number of Producers", nProducers);
        cmd.AddValue("nCsSize", "Content Store size", nCsSize);
        cmd.Parse(argc, argv);

        // parse str commands into enums
        NS_LOG_UNCOND("TraceFile = " << nTraceFile);

        //////////////////////
        //     WIRELESS
        //////////////////////
        NS_LOG_INFO("Setup Wi-Fi Stack ...");
        WifiHelper wifi;
        wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", wifiRate);
        // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

        YansWifiChannelHelper wifiChannel;  // = YansWifiChannelHelper::Default ();
        wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
        wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
        wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel");
        // wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel");

        YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default();
        wifiPhyHelper.SetChannel(wifiChannel.Create());
        // wifiPhyHelper.SetErrorRateModel("ns3::YansErrorRateModel");
        // wifiPhyHelper.SetErrorRateModel("ns3::RateErrorModel",
        //                                 "ErrorRate", DoubleValue(nErrorRate),
        //                                 "ErrorUnit", EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
        // wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
        // wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));

        WifiMacHelper wifiMacHelper;
        wifiMacHelper.SetType("ns3::AdhocWifiMac");

        //////////////////////
        //     MOBILITY
        //////////////////////
        NS_LOG_INFO("Setup Mobility Model ...");
        const int MAP_SIZE = 200;  // map size (in meters)
        MobilityHelper mobility;
        mobility.SetPositionAllocator(
            "ns3::RandomDiscPositionAllocator",                                                            // random position based on polar radius
            "X", StringValue(to_string(MAP_SIZE / 2)),                                                     // center in x axis of the map
            "Y", StringValue(to_string(MAP_SIZE / 2)),                                                     // center in y axis of the map
            "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + to_string(MAP_SIZE / 2) + "]"));  // delta change of the map
        mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                  "Mode", StringValue("Time"),                                          // change speed based on time
                                  "Time", StringValue("2s"),                                            // change every X seconds
                                  "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),  // speed will vary between min & max
                                  "Bounds", StringValue("0|" + to_string(MAP_SIZE) + "|0|" + to_string(MAP_SIZE)));

        //////////////////////
        //     ENERGY MODEL
        //////////////////////
        NS_LOG_INFO("Setup Energy Model ...");
        /* energy source */
        BasicEnergySourceHelper basicSourceHelper;
        // configure energy source - DEAULT IS 10.0
        basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(nInitialEnergy));
        /* device energy model */
        WifiRadioEnergyModelHelper radioEnergyHelper;
        // configure radio energy model
        // radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));

        //////////////////////
        //     NDN STACK
        //////////////////////
        NS_LOG_INFO("Setup NDN Stack ...");
        ndn::StackHelper ndnHelper;
        // ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback(MyNetDeviceFaceCallback));
        ndnHelper.setPolicy("nfd::cs::lru");
        ndnHelper.setCsSize(nCsSize);  // forwarder->getCs().setLimit(nCsSize);
        ndnHelper.SetDefaultRoutes(true);
        ndnHelper.AddFaceCreateCallback(WifiNetDevice::GetTypeId(), MakeCallback(&addFaceWifiNetDeviceCallback));

        //////////////////////
        //     NODES
        //////////////////////
        NS_LOG_INFO("Setup NS3 Nodes (consumers, fwd, producers) ...");
        NodeContainer nodes, consumers, producers, forwarders;
        producers.Create(nProducers);
        consumers.Create(nConsumers);
        if (nNodes - nConsumers - nProducers > 0)
            forwarders.Create(nNodes - nConsumers - nProducers);
        nodes.Add(producers);
        nodes.Add(consumers);
        nodes.Add(forwarders);

        //////////////////////
        //     INSTALL
        //////////////////////

        // 1. Install Wifi
        NS_LOG_INFO("Installing Wi-Fi Stack ...");
        NetDeviceContainer wifiNetDevices = wifi.Install(wifiPhyHelper, wifiMacHelper, nodes);
        NetDeviceContainer fwdNetDevices;
        for (auto fwdNode : forwarders) {
            fwdNetDevices.Add(fwdNode->GetDevice(0));
        }

        // 2. Install Mobility model
        NS_LOG_INFO("Installing Mobility Model ...");
        if (nTraceFile.empty()) {
            mobility.Install(nodes);
        } else {
            Ns2MobilityHelper mobilityTrace(nTraceFile);
            mobilityTrace.Install();
        }

        // Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&CourseChange));  // monitor for chg in pos

        // 3. Install Energy model
        NS_LOG_INFO("Installing Energy Model ...");
        EnergySourceContainer sources           = basicSourceHelper.Install(forwarders);
        DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install(fwdNetDevices, sources);
        // // TODO TEST CODE BELOW TO FORCE SPECIFIC NODES' SELECTION
        // NetDeviceContainer tempNetDevices;
        // NodeContainer tempContainer;
        // for (auto fwdNode : forwarders) {
        //     if (fwdNode->GetId() != 8 && fwdNode->GetId() != 5) {  //&& fwdNode->GetId() != 9) {
        //         tempContainer.Add(fwdNode);
        //         tempNetDevices.Add(fwdNode->GetDevice(0));
        //     }
        // }
        // EnergySourceContainer sources           = basicSourceHelper.Install(tempContainer);
        // DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install(tempNetDevices, sources);

        // 4. Install NDN stack
        NS_LOG_INFO("Installing NDN stack ...");
        ndnHelper.Install(nodes);

        // 5. Set fw strategy
        NS_LOG_INFO("Installing NDN Forwarding Strategies ...");
        // ndn::StrategyChoiceHelper::Install(producers, "/", "/localhost/nfd/strategy/multicast");
        ndn::StrategyChoiceHelper::Install(consumers, "/", "/localhost/nfd/strategy/multicast");
        ndn::StrategyChoiceHelper::Install(forwarders, "/", "/localhost/nfd/strategy/multicast");
        ndn::StrategyChoiceHelper::Install(producers, "/", "/localhost/nfd/strategy/multicast");

        // 6.0 Set up Trust Anchor
        auto rootIdentityPrefix = "/test";
        NS_LOG_INFO("Installing Trust Anchor ...");
        ::ndn::security::v2::KeyChain rootKeyChain("pib-memory:", "tpm-memory:");
        auto rootIdentity = rootKeyChain.createIdentity(rootIdentityPrefix);
        ::ndn::io::save(rootIdentity.getDefaultKey().getDefaultCertificate(), nTrustAnchorCert);
        NS_LOG_INFO("Root Identity: " << rootIdentity);
        NS_LOG_INFO("Root Key Type: " << rootIdentity.getDefaultKey().getKeyType());
        NS_LOG_INFO(rootIdentity.getDefaultKey().getDefaultCertificate());

        // 6.1. Set up CONSUMER
        auto prefixName = "/test/prefix";
        NS_LOG_INFO("Installing Consumer App ...");
        // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
        ndn::AppHelper consumerHelper("CustomConsumerBoot");
        consumerHelper.SetAttribute("LifeTime", StringValue(nLifetime));
        consumerHelper.SetAttribute("Frequency", DoubleValue(nPktFreq));   // consumer CBR interest/second
        consumerHelper.SetAttribute("Randomize", StringValue("uniform"));  // consumer randomize send time
        consumerHelper.SetAttribute("ValidatorConf", StringValue(nValidatorConf));
        consumerHelper.SetPrefix(prefixName);
        // set random start time
        for (uint32_t i = 0; i < nConsumers; i++) {
            Ptr<UniformRandomVariable> start_time = CreateObject<UniformRandomVariable>();
            consumerHelper.Install(consumers[i]).Start(Seconds(start_time->GetValue(0.2, 0.75)));  // consumer start time
        }

        // 6.2. Set up PRODUCER
        // ndn::AppHelper producerHelper("ns3::ndn::Producer");
        NS_LOG_INFO("Installing Producer App ...");
        ndn::AppHelper producerHelper("CustomProducerBoot");
        producerHelper.SetAttribute("Freshness", TimeValue(Seconds(nFreshness)));
        producerHelper.SetAttribute("PayloadSize", StringValue("1480"));  // payload MTU
        producerHelper.SetPrefix(prefixName);                             // ndn prefix
        auto producersApps = producerHelper.Install(producers);
        producersApps.Start(Seconds(0.1));  // producers start time
        for (auto it = producersApps.Begin(); it != producersApps.End(); it++) {
            auto customProducerApp = DynamicCast<ndn::CustomProducerBoot>(*it);
            if (!customProducerApp) continue;
            customProducerApp->setSignCallback([&](::ndn::security::v2::Certificate cert) {
                ::ndn::SignatureInfo signatureInfo;
                signatureInfo.setValidityPeriod(::ndn::security::ValidityPeriod(
                    ndn::time::system_clock::TimePoint(), ndn::time::system_clock::now() + ndn::time::days(365)));
                rootKeyChain.sign(cert, ::ndn::security::SigningInfo(rootIdentity).setSignatureInfo(signatureInfo));
                return cert;
            });
        }

        // 7.1. Simulate link failures (P2P links only)
        // Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
        // em->SetAttribute("ErrorRate", DoubleValue(nErrorRate));
        // em->SetAttribute("ErrorUnit", EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
        // for (auto node : nodes) {
        //     node->SetAttribute("ReceptionErrorModel", PointerValue(em));
        // }

        // 7.2. Monitor data traces and other interesting variables
        NS_LOG_INFO("Installing NDN Tracers ...");
        ndn::AppDelayTracer::InstallAll("results/app-delays-trace.txt");
        ndn::L3RateTracer::InstallAll("results/rate-trace.txt", Seconds(1.0));
        ndn::CsTracer::InstallAll("results/cs-trace.txt", Seconds(1));

        NS_LOG_INFO("Installing Custom Tracers ...");
        auto customTracer = CreateObject<CustomTracerBoot>();
        customTracer->SetAttribute("TraceFilename", StringValue("results/dataCustomCons.dat"));
        customTracer->SetAttribute("NodesToMonitor", NodeContainerValue(consumers));

        auto customTracerFwd = CreateObject<CustomTracerBoot>();
        customTracerFwd->SetAttribute("TraceFilename", StringValue("results/dataCustomFwd.dat"));
        customTracerFwd->SetAttribute("NodesToMonitor", NodeContainerValue(forwarders));

        auto customTracerProd = CreateObject<CustomTracerBoot>();
        customTracerProd->SetAttribute("TraceFilename", StringValue("results/dataCustomProd.dat"));
        customTracerProd->SetAttribute("NodesToMonitor", NodeContainerValue(producers));

        // 8. Start simulation
        NS_LOG_INFO("Start simulation!");
        Simulator::Stop(Seconds(nSimDuration));
        Simulator::Run();
        Simulator::Destroy();
        return 0;
    }
}  // namespace ns3

int main(int argc, char *argv[]) {
    const int MAX_NUM_SIM = 1;  // MAX NUM OF SIMULATIONS
    for (int idx = MAX_NUM_SIM; idx > 0; idx--) {
        // ns3::RngSeedManager::SetRun(idx); // set random run for replications
        ns3::main(argc, argv);  // run simulation
    }
    return 0;
}
