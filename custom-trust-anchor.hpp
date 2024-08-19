// custom-producer.hpp

#ifndef CUSTOM_TRUST_ANCHOR_H_
#define CUSTOM_TRUST_ANCHOR_H_

// system libs
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// custom includes
#include "custom-app.hpp"

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

    class CustomTrustAnchor : public CustomApp {

    public:
      static TypeId GetTypeId();
      CustomTrustAnchor();
      ~CustomTrustAnchor();

      void StartApplication() override;
      void StopApplication() override;

      void OnInterestKey(std::shared_ptr<const ndn::Interest> interest) override;
      void OnInterestContent(std::shared_ptr<const ndn::Interest> interest) override;

      void OnDataCertificate(std::shared_ptr<const ndn::Data> data) override;
      void OnDataContent(std::shared_ptr<const ndn::Data> data) override;

    private:
      void createTrustAnchor();
      void readValidationRules();

      void addProducerSchema(::ndn::Name identityName);
      void sendDataSubscribe();

    private:
      ::ndn::Name m_zonePrefix;
      ns3::Time m_schemaFreshness;

      std::string m_trustAnchorCert;
    };

  } // namespace ndn
} // namespace ns3

#endif // CUSTOM_TRUST_ANCHOR_H_