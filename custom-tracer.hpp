// custom-app.hpp

#ifndef CUSTOM_TRACER_BOOT_H_
#define CUSTOM_TRACER_BOOT_H_

#include <limits>
#include <string>

#include "custom-utils.hpp"

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/trace-helper.h"
#include "ns3/wifi-phy-state-helper.h"
#include "ns3/wifi-phy.h"

namespace ns3 {
  std::ostream &operator<<(std::ostream &os, const NodeContainer &nodeContainer);
  std::istream &operator>>(std::istream &is, NodeContainer &nodeContainer);

  ATTRIBUTE_HELPER_HEADER(NodeContainer);
} // namespace ns3

namespace ns3 {

  class CustomTracer : public ns3::Object {
  public:
    // register NS-3 type "CustomConsumer"
    static TypeId GetTypeId();

    CustomTracer();

    void doInstall();
    void collectData();

  private:
    void saveDataToFile();
    void resetTraceCounters();
    void updateCounters();

    // utilitary functions
  private:
    std::string decodeCurrentWifiState(const WifiPhyState &state);
    std::string decodeWifiRxFailureReason(const WifiPhyRxfailureReason &reason);

    // traces
  private:
    void PhyStateChg(std::string context, Time start, Time duration, WifiPhyState state);
    void PhyRxError(std::string context, Ptr<const Packet> pkt, double snr);
    void PhyTxDrop(std::string context, Ptr<const Packet> pkt);
    void PhyRxDrop(std::string context, Ptr<const Packet> pkt, WifiPhyRxfailureReason reason);

    void FirstInterestDataDelay(Ptr<ndn::App> app, uint32_t seqno, Time delay, uint32_t retx_count,
                                int32_t hop_count);
    void LastRetransmittedInterestDataDelay(Ptr<ndn::App> app, uint32_t seqno, Time delay, int32_t hopCount);

  private:
    template <typename T> class PktCounter {
    public:
      PktCounter() : last(0), current(0) {}
      void update() {
        last = current;
        current = 0;
      }
      T getDiff() { return current - last; }
      T last, current;
    };

  private:
    NodeContainer mNodesToMonitor;
    std::string mTraceFilename;
    Ptr<OutputStreamWrapper> ostream;

  private:
    double remainingEnergy, energyConsumption, initialEnergy;
    std::size_t pitSize, csSize;
    PktCounter<uint64_t> csHits, csMisses;
    PktCounter<uint64_t> inData, inInt, inNack;
    PktCounter<uint64_t> outData, outInt, outNacks;
    PktCounter<uint64_t> satInt, unsatInt, loopedInt;
    PktCounter<uint64_t> droppedInt;
    uint64_t retxInt, uniqData;
    uint64_t phyRxDropped, phyTxDropped;
    ::utils::AvgStruct firstDelayStr, lastDelayStr, hopCountStr;
  };

} // namespace ns3

#endif // CUSTOM_TRACER_BOOT_H_