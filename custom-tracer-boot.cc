
#include "custom-tracer-boot.hpp"

// ns3 modules
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
// #include "ns3/point-to-point-module.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/model/ndn-net-device-transport.hpp"

// ENERGY MODULES
#include "ns3/basic-energy-source-helper.h"
#include "ns3/basic-energy-source.h"
#include "ns3/device-energy-model-container.h"
#include "ns3/energy-module.h"
#include "ns3/energy-source-container.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/wifi-radio-energy-model.h"

// TRACES
#include "ns3/ndnSIM/utils/tracers/l2-rate-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-cs-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.hpp"

// ndn-cxx
#include "ns3/ndnSIM/ndn-cxx/detail/common.hpp"
#include "ns3/ndnSIM/ndn-cxx/detail/packet-base.hpp"
#include "ns3/ndnSIM/ndn-cxx/encoding/buffer.hpp"
#include "ns3/ndnSIM/ndn-cxx/impl/lp-field-tag.hpp"
#include "ns3/ndnSIM/ndn-cxx/tag.hpp"

// NS3 NDNSIM MODULES
#include "ns3/ndnSIM/NFD/daemon/common/global.hpp"
#include "ns3/ndnSIM/NFD/daemon/common/logger.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/face.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/algorithm.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/model/ndn-net-device-transport.hpp"

NS_LOG_COMPONENT_DEFINE ("CustomTracerBoot");

namespace ns3 {

ATTRIBUTE_HELPER_CPP (NodeContainer);

std::ostream &
operator<< (std::ostream &os, const NodeContainer &intMetrics)
{
  os << "[";
  for (auto item : intMetrics)
    {
      os << item << ",";
    }
  os << "]";
  return os;
}
std::istream &
operator>> (std::istream &is, NodeContainer &nodeContainer)
{
  std::string temp;
  char data;
  while (is >> data)
    {
      if (data != ',')
        {
          temp.push_back (data);
        }
      else
        {
          nodeContainer.Add (temp);
          temp.clear ();
        }
    }
  return is;
}

NS_OBJECT_ENSURE_REGISTERED (CustomTracerBoot);

// register NS-3 type
TypeId
CustomTracerBoot::GetTypeId ()
{
  static TypeId tid =
      TypeId ("CustomTracerBoot")
          .SetParent<ns3::Object> ()
          .AddConstructor<CustomTracerBoot> ()
          .AddAttribute (
              "TraceFilename", "Filename to save trace", StringValue ("results/dataCustom.dat"),
              MakeStringAccessor (&CustomTracerBoot::mTraceFilename), MakeStringChecker ())
          .AddAttribute ("NodesToMonitor", "Nodes to keep track of", NodeContainerValue (),
                         MakeNodeContainerAccessor (&CustomTracerBoot::mNodesToMonitor),
                         MakeNodeContainerChecker ());
  return tid;
}

CustomTracerBoot::CustomTracerBoot ()
{
  Simulator::Schedule (Seconds (0), &CustomTracerBoot::doInstall, this);
}

void
CustomTracerBoot::doInstall ()
{
  resetTraceCounters ();
  updateCounters ();
  AsciiTraceHelper asciiTraceHelper;
  this->ostream = asciiTraceHelper.CreateFileStream (mTraceFilename);
  *ostream->GetStream () << "Time(1)"
                         << "\tPitSize(2)"
                         << "\tCsSize(3)"
                         << "\tCsHits(4)"
                         << "\tCsMisses(5)"
                         << "\tinData(6)"
                         << "\toutData(7)"
                         << "\tinInt(8)"
                         << "\toutInt(9)"
                         << "\tinNack(10)"
                         << "\toutNacks(11)"
                         << "\tloopedInt(12)"
                         << "\tsatInt(13)"
                         << "\tunsatInt(14)"
                         << "\tremEnergy(15)"
                         << "\tconsEnergy(16)"
                         << "\tinitEnergy(17)"
                         << "\tphyRxDropped(18)"
                         << "\tphyTxDropped(19)"
                         << "\tavgFirstDelay(20)"
                         << "\tavgLastDelay(21)"
                         << "\tavgHopCount(22)"
                         << "\tdroppedInt(23)"
                         << "\tretxInt(24)"
                         << "\tuniqData(25)"
                         << "\n";

  for (auto node : mNodesToMonitor)
    {
      auto nodeIdConn = "/NodeList/" + to_string (node->GetId ());
      Config::Connect (nodeIdConn + "/DeviceList/*/$ns3::WifiNetDevice/Phy/State/State",
                       MakeCallback (&CustomTracerBoot::PhyStateChg, this));
      Config::Connect (nodeIdConn + "/DeviceList/*/$ns3::WifiNetDevice/Phy/State/RxError",
                       MakeCallback (&CustomTracerBoot::PhyRxError, this));
      Config::Connect (nodeIdConn + "/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop",
                       MakeCallback (&CustomTracerBoot::PhyTxDrop, this));
      Config::Connect (nodeIdConn + "/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop",
                       MakeCallback (&CustomTracerBoot::PhyRxDrop, this));
      Config::ConnectWithoutContext (
          nodeIdConn + "/ApplicationList/*/$ns3::ndn::Consumer/FirstInterestDataDelay",
          MakeCallback (&CustomTracerBoot::FirstInterestDataDelay, this));
      Config::ConnectWithoutContext (
          nodeIdConn + "/ApplicationList/*/$ns3::ndn::Consumer/LastRetransmittedInterestDataDelay",
          MakeCallback (&CustomTracerBoot::LastRetransmittedInterestDataDelay, this));

      // monitor NDN delay and hop count - NOT PRECISE, BEST OT USE FIRSTINTERESTDATADELAY from consumers
      // auto l3         = node->GetObject<ns3::ndn::L3Protocol>();
      // auto ndnFwd     = l3->getForwarder();
      // calculate delay & hopCount
      // ndnFwd->beforeSatisfyInterest.connect([&delayStr, &hopCountStr](const nfd::pit::Entry &pitEntry, const nfd::face::Face &ingressFace, const ndn::Data &data) {
      //     for (auto iter = pitEntry.out_begin(); iter != pitEntry.out_end(); iter++) {
      //         auto &outRecord = *iter;
      //         if (outRecord.getFace().getId() != ingressFace.getId()) continue;
      //         // calcualte delay
      //         double currDelayMs = ndn::time::duration_cast<ndn::time::nanoseconds>(ndn::time::steady_clock::now() - outRecord.getLastRenewed()).count() / 1e6;
      //         delayStr.update(currDelayMs);
      //         // calculate hopcount
      //         auto hopCount = data.getTag<nfd::lp::HopCountTag>();
      //         if (hopCount != NULL) {
      //             hopCountStr.update(*hopCount);
      //         }
      //     }
      // });
    }

  // call function to collect Data
  this->collectData ();
}

void
CustomTracerBoot::collectData ()
{
  updateCounters ();
  // for (uint32_t i = 0; i < NodeList::GetNNodes(); i++) {
  //     auto node          = NodeList::GetNode(i);
  for (auto node : mNodesToMonitor)
    {
      auto l3 = node->GetObject<ns3::ndn::L3Protocol> ();
      auto ndnFwd = l3->getForwarder ();
      auto &faceTable = l3->getFaceTable ();

      nfd::pit::Pit &pit = ndnFwd->getPit ();
      nfd::cs::Cs &cs = ndnFwd->getCs ();
      pitSize += pit.size ();
      csSize += cs.size ();

      csHits.current += ndnFwd->getCounters ().nCsHits;
      csMisses.current += ndnFwd->getCounters ().nCsMisses;
      inData.current += ndnFwd->getCounters ().nInData;
      inInt.current += ndnFwd->getCounters ().nInInterests;
      inNack.current += ndnFwd->getCounters ().nInNacks;
      // loopedInt.current += ndnFwd->getCounters().nLoopedInterests;
      outData.current += ndnFwd->getCounters ().nOutData;
      outInt.current += ndnFwd->getCounters ().nOutInterests;
      outNacks.current += ndnFwd->getCounters ().nOutNacks;
      satInt.current += ndnFwd->getCounters ().nSatisfiedInterests;
      unsatInt.current += ndnFwd->getCounters ().nUnsatisfiedInterests;

      // calculate face queue usage and dropped interests
      // size_t queue_count = 0;
      for (auto &face : faceTable)
        {
          // dropped interests
          auto linkService = face.getLinkService ();
          droppedInt.current += linkService->getCounters ().nDroppedInterests;
        }

      // calculate energy consumption
      auto energySrcCont = node->GetObject<ns3::EnergySourceContainer> ();
      if (energySrcCont != NULL)
        {
          auto energySrc = energySrcCont->Get (0);
          remainingEnergy += energySrc->GetRemainingEnergy ();
          initialEnergy += energySrc->GetInitialEnergy ();
          energyConsumption = initialEnergy - remainingEnergy;
        }
    }
  // calculate averages
  pitSize = pitSize / mNodesToMonitor.GetN ();
  csSize = csSize / mNodesToMonitor.GetN ();
  remainingEnergy = remainingEnergy / mNodesToMonitor.GetN ();
  initialEnergy = initialEnergy / mNodesToMonitor.GetN ();
  energyConsumption = energyConsumption / mNodesToMonitor.GetN ();

  // save data
  saveDataToFile ();

  // clear direct trace counters
  resetTraceCounters ();

  // schedule next trace measure
  Simulator::Schedule (Seconds (1), &CustomTracerBoot::collectData, this);
}

void
CustomTracerBoot::saveDataToFile ()
{
  *ostream->GetStream () << setprecision (2) << fixed // setprecision(6) << defaultfloat
                         << int (Simulator::Now ().GetSeconds ()) << "\t" << pitSize << "\t"
                         << csSize << "\t" << csHits.getDiff () << "\t" << csMisses.getDiff ()
                         << "\t" << inData.getDiff () << "\t" << outData.getDiff () << "\t"
                         << inInt.getDiff () << "\t" << outInt.getDiff () << "\t"
                         << inNack.getDiff () << "\t" << outNacks.getDiff () << "\t"
                         << loopedInt.getDiff () << "\t" << satInt.getDiff () << "\t"
                         << unsatInt.getDiff () << "\t" << remainingEnergy << "\t"
                         << energyConsumption << "\t" << initialEnergy << "\t" << phyRxDropped
                         << "\t" << phyTxDropped << "\t" << firstDelayStr.get () << "\t"
                         << lastDelayStr.get () << "\t" << hopCountStr.get () << "\t"
                         << droppedInt.getDiff () << "\t" << retxInt << "\t" << uniqData << "\n";
}

void
CustomTracerBoot::resetTraceCounters ()
{
  phyRxDropped = phyTxDropped = 0;
  retxInt = uniqData = 0;
  firstDelayStr.clear ();
  firstDelayStr.update (std::numeric_limits<double>::max ());
  lastDelayStr.clear ();
  lastDelayStr.update (std::numeric_limits<double>::max ());
  hopCountStr.clear ();
  hopCountStr.update (std::numeric_limits<double>::max ());
}

void
CustomTracerBoot::updateCounters ()
{
  remainingEnergy = energyConsumption = initialEnergy = 0;
  pitSize = csSize = 0;
  csHits.update ();
  csMisses.update ();
  inData.update ();
  inInt.update ();
  inNack.update ();
  loopedInt.update ();
  outData.update ();
  outInt.update ();
  outNacks.update ();
  satInt.update ();
  unsatInt.update ();
  droppedInt.update ();
}

std::string
CustomTracerBoot::decodeCurrentWifiState (const WifiPhyState &state)
{
  switch (state)
    {
    case WifiPhyState::CCA_BUSY:
      return "CCA_BUSY";
    case WifiPhyState::IDLE:
      return "IDLE";
    case WifiPhyState::OFF:
      return "OFF";
    case WifiPhyState::RX:
      return "RX";
    case WifiPhyState::SLEEP:
      return "SLEEP";
    case WifiPhyState::SWITCHING:
      return "SWITCHING";
    case WifiPhyState::TX:
      return "TX";
    }
  return "?";
}

std::string
CustomTracerBoot::decodeWifiRxFailureReason (const WifiPhyRxfailureReason &reason)
{
  switch (reason)
    {
    case WifiPhyRxfailureReason::ERRONEOUS_FRAME:
      return "ERRONEOUS_FRAME";
    case WifiPhyRxfailureReason::FRAME_CAPTURE_PACKET_SWITCH:
      return "FRAME_CAPTURE_PACKET_SWITCH";
    case WifiPhyRxfailureReason::L_SIG_FAILURE:
      return "L_SIG_FAILURE";
    case WifiPhyRxfailureReason::MPDU_WITHOUT_PHY_HEADER:
      return "MPDU_WITHOUT_PHY_HEADER";
    case WifiPhyRxfailureReason::NOT_ALLOWED:
      return "NOT_ALLOWED";
    case WifiPhyRxfailureReason::OBSS_PD_CCA_RESET:
      return "OBSS_PD_CCA_RESET";
    case WifiPhyRxfailureReason::PREAMBLE_DETECT_FAILURE:
      return "PREAMBLE_DETECT_FAILURE";
    case WifiPhyRxfailureReason::PREAMBLE_DETECTION_PACKET_SWITCH:
      return "PREAMBLE_DETECTION_PACKET_SWITCH";
    case WifiPhyRxfailureReason::SIG_A_FAILURE:
      return "SIG_A_FAILURE";
    case WifiPhyRxfailureReason::UNKNOWN:
      return "UNKNOWN";
    case WifiPhyRxfailureReason::UNSUPPORTED_SETTINGS:
      return "UNSUPPORTED_SETTINGS";
    }
  return "?";
}

void
CustomTracerBoot::FirstInterestDataDelay (Ptr<ndn::App> app, uint32_t seqno, Time delay,
                                          uint32_t retx_count, int32_t hop_count)
{
  // NFD_LOG_DEBUG("Seq: " << seqno << " - Delay: " << delay.ToDouble(Time::MS) << " - RetxCount: " << retx_count << " - HopCount: " << hop_count);
  if (firstDelayStr.get () == std::numeric_limits<double>::max ())
    {
      firstDelayStr.clear ();
    }
  firstDelayStr.update (delay.ToDouble (Time::MS));
  if (hopCountStr.get () == std::numeric_limits<double>::max ())
    {
      hopCountStr.clear ();
    }
  hopCountStr.update (hop_count);
  retxInt += retx_count;
  uniqData++;
}

void
CustomTracerBoot::LastRetransmittedInterestDataDelay (Ptr<ndn::App> app, uint32_t seqno, Time delay,
                                                      int32_t hopCount)
{
  NFD_LOG_DEBUG ("Seq: " << seqno << " - Delay: " << delay.ToDouble (Time::MS)
                         << " - HopCount: " << hopCount);
  if (lastDelayStr.get () == std::numeric_limits<double>::max ())
    {
      lastDelayStr.clear ();
    }
  lastDelayStr.update (delay.ToDouble (Time::MS));
}

void
CustomTracerBoot::PhyStateChg (std::string context, Time start, Time duration, WifiPhyState state)
{
  // double currentA       = 0.0;
  // std::string currState = "?";
  // double consEnergyWifi = 0.0;
  // double consEnergySrc  = 0.0;
  // auto node             = NodeList::GetNode(Simulator::GetContext());
  // auto energySrcCont    = node->GetObject<ns3::EnergySourceContainer>();
  // if (energySrcCont != NULL) {
  //     auto energySrc = energySrcCont->Get(0);
  //     if (energySrc != NULL) {
  //         consEnergySrc        = energySrc->GetInitialEnergy() - energySrc->GetRemainingEnergy();
  //         auto energyModelCont = energySrc->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel");
  //         auto wifiEnergy      = (energyModelCont.GetN() > 0 ? energyModelCont.Get(0)->GetObject<ns3::WifiRadioEnergyModel>() : NULL);
  //         if (wifiEnergy != NULL) {
  //             currentA       = wifiEnergy->GetCurrentA();
  //             currState      = decodeCurrentWifiState(wifiEnergy->GetCurrentState());
  //             consEnergyWifi = wifiEnergy->GetTotalEnergyConsumption();
  //         }
  //     }
  // }
  // NS_LOG_DEBUG("CURRENT STATE -"
  //              << "(" << currState << ", " << currentA << ")"
  //              << " - Consumed Energy: (" << consEnergySrc << ", " << consEnergyWifi << ")"
  //              << " - Elapsed Time: ["
  //              << start.GetSeconds()
  //              << " + " << duration.GetSeconds() << "]");
  // NS_LOG_DEBUG("NEW STATE - " << decodeCurrentWifiState(state));
}

void
CustomTracerBoot::PhyRxError (std::string context, Ptr<const Packet> pkt, double snr)
{
  // no need to count this dropped packet as
  // it has already been counted in PhyRxDrop
  // NS_LOG_DEBUG("Pkt RX error - SNR: " << snr);
}

void
CustomTracerBoot::PhyTxDrop (std::string context, Ptr<const Packet> pkt)
{
  NS_LOG_DEBUG ("Pkt TX drop");
  phyTxDropped++;
}

void
CustomTracerBoot::PhyRxDrop (std::string context, Ptr<const Packet> pk,
                             WifiPhyRxfailureReason reason)
{
  NS_LOG_DEBUG ("Pkt RX drop - Reason: " << decodeWifiRxFailureReason (reason));
  phyRxDropped++;
}

// static void CourseChange(std::string context, Ptr<const MobilityModel> mobility) {
//     Vector pos = mobility->GetPosition();
//     Vector vel = mobility->GetVelocity();
//     std::cout << Simulator::Now()
//               << " RandomDirection2dMobilityModel:CourseChange(): MODEL=" << mobility
//               << " POS: x=" << pos.x << " y=" << pos.y << " z=" << pos.z << " VEL:" << vel.x
//               << " y=" << vel.y << " z=" << vel.z << std::endl;
// }

// power is in dBi
// void powerChange(Ptr<OutputStreamWrapper> stream, std::string context, double oldPower, double newPower, Mac48Address remoteAddress){
//     NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newPower);
//     *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldPower << "\t" << newPower << std::endl;
// }

// /// Trace function for remaining energy at node.
// void remainingEnergy(double oldValue, double remainingEnergy) {
//     // auto node = ::ns3::NodeList::GetNode(::ns3::Simulator::GetContext());
//     // NS_LOG_UNCOND("+" << Simulator::Now().GetSeconds() << "s " << node->GetId() << " remainingEnergy(): Current remaining energy = " << remainingEnergy << "J");
//     NS_LOG_UNCOND("+" << Simulator::Now().GetSeconds() << "s  remainingEnergy(): Current remaining energy = " << remainingEnergy << "J");
// }

// /// Trace function for total energy consumption at node.
// void TotalEnergy(double oldValue, double totalEnergy) {
//     NS_LOG_UNCOND("+" << Simulator::Now().GetSeconds()
//                       << "s Total energy consumed by radio = " << totalEnergy << "J");
// }

} // namespace ns3
