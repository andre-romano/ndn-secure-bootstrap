// custom-producer.hpp

#ifndef CUSTOM_PRODUCER_BOOT_H_
#define CUSTOM_PRODUCER_BOOT_H_

#include "ns3/ndnSIM/apps/ndn-producer.hpp"

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include <set>
#include <string>

// namespace ns3 {
//     class IntMetricSet : public std::set<IntMetric> {};

//     std::ostream &operator<<(std::ostream &os, const IntMetricSet &intMetrics);
//     std::istream &operator>>(std::istream &is, IntMetricSet &intMetrics);

//     ATTRIBUTE_HELPER_HEADER(IntMetricSet);

//     class IntFwdMap : public ::nfd::fw::IntMetaInfo::IntFwdMap {};

//     std::ostream &operator<<(std::ostream &os, const IntFwdMap &obj);
//     std::istream &operator>>(std::istream &is, IntFwdMap &obj);

//     ATTRIBUTE_HELPER_HEADER(IntFwdMap);
// }  // namespace ns3

namespace ns3 {
    namespace ndn {

        class CustomProducerBoot : public App {
          public:
            static TypeId GetTypeId();
            CustomProducerBoot();
            // virtual ~CustomProducerBoot();

            // inherited from Application base class.
            virtual void StartApplication();

            virtual void StopApplication();

            // (overridden from ndn::App) Callback that will be called when Interest arrives
            virtual void OnInterest(std::shared_ptr<const Interest> interest);

          private:
            Name m_prefix;
            Name m_postfix;
            uint32_t m_virtualPayloadSize;
            Time m_freshness;
        };

    }  // namespace ndn
}  // namespace ns3

#endif  // CUSTOM_PRODUCER_BOOT_H_