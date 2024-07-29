// custom-producer.hpp

#ifndef CUSTOM_PRODUCER_BOOT_H_
#define CUSTOM_PRODUCER_BOOT_H_

#include "ns3/ndnSIM/apps/ndn-producer.hpp"

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/signing-helpers.hpp"

#include <functional>
#include <set>
#include <string>

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

        class CustomProducer : public App {
          public:
            using signCallback = std::function<::ndn::security::v2::Certificate(
                ::ndn::security::v2::Certificate
            )>;

          public:
            static TypeId GetTypeId();
            CustomProducer();
            ~CustomProducer();

            // inherited from Application base class.
            virtual void StartApplication();

            virtual void StopApplication();

            // (overridden from ndn::App) Callback that will be called when
            // Interest arrives
            virtual void OnInterest(std::shared_ptr<const Interest> interest);

            void OnInterestKey(std::shared_ptr<const ndn::Interest> interest);
            void OnInterestContent(std::shared_ptr<const ndn::Interest> interest
            );

            void setSignCallback(signCallback cb);

          private:
            const ::ndn::security::v2::Certificate &createCertificate();
            void loadCertificateTrustAnchor();
            void printKeyChain();

          private:
            Name m_prefix;
            Name m_postfix;
            uint32_t m_virtualPayloadSize;
            Time m_freshness;

            Name m_identityPrefix;
            Name m_trustAnchorIdentityPrefix;
            std::string m_trustAnchorCertFilename;
            signCallback m_signCallback;

            ::ndn::security::v2::KeyChain m_keyChain;
            ::ndn::security::SigningInfo m_signingInfo;
        };

    } // namespace ndn
} // namespace ns3

#endif // CUSTOM_PRODUCER_BOOT_H_