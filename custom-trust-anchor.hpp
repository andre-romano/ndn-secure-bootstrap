// custom-producer.hpp

#ifndef CUSTOM_TRUST_ANCHOR_H_
#define CUSTOM_TRUST_ANCHOR_H_

#include <functional>
#include <set>
#include <string>

// NS3 / ndnSIM / ndn-cxx includes
#include "ns3/ndnSIM/apps/ndn-producer.hpp"

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/signing-helpers.hpp"

// custom includes
#include "custom-producer.hpp"

// namespace ns3 {
//     class IntMetricSet : public std::set<IntMetric> {};

//     std::ostream &operator<<(std::ostream &os, const IntMetricSet
//     &intMetrics); std::istream &operator>>(std::istream &is, IntMetricSet
//     &intMetrics);

//     ATTRIBUTE_HELPER_HEADER(IntMetricSet);

//     class IntFwdMap : public ::nfd::fw::IntMetaInfo::IntFwdMap {};

//     std::ostream &operator<<(std::ostream &os, const IntFwdMap &obj);
//     std::istream &operator>>(std::istream &is, IntFwdMap &obj);

//     ATTRIBUTE_HELPER_HEADER(IntFwdMap);
// }  // namespace ns3

namespace ns3 {
  namespace ndn {

    class CustomTrustAnchor : public CustomProducer {

    public:
      static TypeId GetTypeId();
      CustomTrustAnchor();
      ~CustomTrustAnchor();

      void StartApplication() override;
      void StopApplication() override;

      void OnInterestKey(std::shared_ptr<const ndn::Interest> interest) override;
      void OnInterestContent(std::shared_ptr<const ndn::Interest> interest) override;

    private:
      void setupTrustAnchorCertFile();

    private:
      std::string m_schemaPrefix;

      std::string m_trustAnchorCert;
      std::string m_validatorFilename;
    };

  } // namespace ndn
} // namespace ns3

#endif // CUSTOM_TRUST_ANCHOR_H_