// custom-producer.cpp

#include "custom-producer-boot.hpp"

#include "ns3/ndnSIM/ndn-cxx/util/io.hpp"

#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

// #include "ns3/boolean.h"
// #include "ns3/callback.h"
// #include "ns3/double.h"
// #include "ns3/integer.h"
// #include "ns3/string.h"
// #include "ns3/uinteger.h"
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"

#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"

#include <limits>
#include <memory>
#include <string>

// namespace ns3 {
//     ATTRIBUTE_HELPER_CPP(IntMetricSet);

//     std::ostream &operator<<(std::ostream &os, const IntMetricSet &intMetrics) {
//         for (auto item : intMetrics) {
//             os << item;
//         }
//         return os;
//     }
//     std::istream &operator>>(std::istream &is, IntMetricSet &intMetrics) {
//         std::string temp;
//         char data;
//         while (is >> data) {
//             if (data != ':') {
//                 temp.push_back(data);
//             } else {
//                 auto intMetric = IntMetric::get(temp);
//                 if (intMetric.isValid()) {
//                     intMetrics.insert(intMetric);
//                 }
//                 temp.clear();
//             }
//         }
//         return is;
//     }

//     ATTRIBUTE_HELPER_CPP(IntFwdMap);

//     std::ostream &operator<<(std::ostream &os, const IntFwdMap &obj) {
//         os << "[";
//         for (auto item : obj) {
//             auto fwdProp = item.second;
//             os << "("
//                << fwdProp.getMetric() << ","
//                << fwdProp.getAction() << ","
//                << fwdProp.getCriteria()
//                << "),";
//         }
//         os << "]";
//         return os;
//     }
//     std::istream &operator>>(std::istream &is, IntFwdMap &obj) {
//         std::string temp;
//         IntMetric metric;
//         IntFwdAction action;
//         IntFwdCriteria criteria;
//         char data;
//         while (is >> data) {
//             if (data == '[' || data == ']')
//                 continue;
//             else if (data != '(' && data != ',' && data != ')') {
//                 temp.push_back(data);
//             } else if (!metric.isValid()) {
//                 metric = IntMetric::get(temp);
//                 temp.clear();
//             } else if (!action.isValid()) {
//                 action = IntFwdAction::getTypeStatic(temp);
//                 temp.clear();
//             } else {
//                 criteria = IntFwdCriteria::getTypeStatic(temp);
//                 temp.clear();
//                 obj[metric] = IntFwdProperty(metric, action, criteria);
//                 metric      = IntMetric();
//                 action      = IntFwdAction();
//                 criteria    = IntFwdCriteria();
//             }
//         }
//         return is;
//     }

// }  // namespace ns3

NS_LOG_COMPONENT_DEFINE("CustomProducerBoot");

namespace ns3 {
    namespace ndn {

        // Necessary if you are planning to use ndn::AppHelper
        NS_OBJECT_ENSURE_REGISTERED(CustomProducerBoot);

        TypeId CustomProducerBoot::GetTypeId() {
            static TypeId tid = TypeId("CustomProducerBoot")
                                    .SetParent<ndn::App>()
                                    .AddConstructor<CustomProducerBoot>()
                                    .AddAttribute("Prefix",
                                                  "Name of the Interest",
                                                  StringValue("/"),
                                                  MakeNameAccessor(&CustomProducerBoot::m_prefix),
                                                  MakeNameChecker())
                                    .AddAttribute(
                                        "Postfix",
                                        "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                                        StringValue("/"),
                                        MakeNameAccessor(&CustomProducerBoot::m_postfix),
                                        MakeNameChecker())
                                    .AddAttribute("PayloadSize",
                                                  "Virtual payload size for Content packets",
                                                  UintegerValue(1024),
                                                  MakeUintegerAccessor(&CustomProducerBoot::m_virtualPayloadSize),
                                                  MakeUintegerChecker<uint32_t>())
                                    .AddAttribute("Freshness",
                                                  "Freshness of data packets, if 0, then unlimited freshness",
                                                  TimeValue(Seconds(0)),
                                                  MakeTimeAccessor(&CustomProducerBoot::m_freshness),
                                                  MakeTimeChecker())
                                    .AddAttribute("IdentityPrefix",
                                                  "Name of the Identity of the App",
                                                  StringValue(""),
                                                  MakeNameAccessor(&CustomProducerBoot::m_identityPrefix),
                                                  MakeNameChecker());
            // .AddAttribute("IntMetrics",
            //               "Set of INT metrics to collect",
            //               IntMetricSetValue(),
            //               MakeIntMetricSetAccessor(&CustomProducerBoot::m_collectMetrics),
            //               MakeIntMetricSetChecker())
            // .AddAttribute("IntFwd",
            //               "INT Fwd commands",
            //               IntFwdMapValue(),
            //               MakeIntFwdMapAccessor(&CustomProducerBoot::m_fwdMap),
            //               MakeIntFwdMapChecker());
            return tid;
        }

        CustomProducerBoot::CustomProducerBoot() : m_keyChain("pib-memory:", "tpm-memory:"),
                                                   m_signingInfo(::ndn::security::SigningInfo::SIGNER_TYPE_NULL) {
            ::ndn::SignatureInfo signatureInfo;
            signatureInfo.setValidityPeriod(::ndn::security::ValidityPeriod(
                ndn::time::system_clock::TimePoint(), ndn::time::system_clock::now() + ndn::time::days(365)));
            m_signingInfo.setSignatureInfo(signatureInfo);
        }

        CustomProducerBoot::~CustomProducerBoot() {}

        void CustomProducerBoot::StartApplication() {
            NS_LOG_FUNCTION_NOARGS();
            App::StartApplication();

            // equivalent to setting interest filter for "/prefix" prefix
            ndn::FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

            // create self-signed certificate/identity and serve it
            auto &cert = createCertificate();
            // loadCertificateTrustAnchor();
            ndn::FibHelper::AddRoute(GetNode(), ::ndn::security::v2::extractKeyNameFromCertName(cert.getName()), m_face, 0);
            NS_LOG_DEBUG("Serving Data prefix: " << m_prefix << " - Certificate: " << cert.getName());

            // NS_LOG_DEBUG(cert);
            printKeyChain();
        }

        void CustomProducerBoot::StopApplication() {
            NS_LOG_FUNCTION_NOARGS();
            App::StopApplication();
        }

        void CustomProducerBoot::OnInterest(std::shared_ptr<const ndn::Interest> interest) {
            ndn::App::OnInterest(interest);  // forward call to perform app-level tracing

            // Note that Interests send out by the app will not be sent back to the app !
            if (::ndn::security::v2::isValidKeyName(interest->getName())) {
                OnInterestKey(interest);
            } else {
                OnInterestContent(interest);
            }
        }

        void CustomProducerBoot::OnInterestKey(std::shared_ptr<const ndn::Interest> interest) {
            NS_LOG_FUNCTION(interest->getName());

            auto identity = m_keyChain.getPib().getIdentity(::ndn::security::v2::extractIdentityFromKeyName(interest->getName()));
            auto key      = identity.getKey(interest->getName());
            auto cert     = std::make_shared<::ndn::security::v2::Certificate>(key.getDefaultCertificate());

            // to create real wire encoding
            cert->wireEncode();

            // LOGGING
            NS_LOG_INFO("Sending Certificate packet: " << cert->getName());
            // NS_LOG_INFO("Signature: " << cert.getSignature().getSignatureInfo());

            // Call trace (for logging purposes) - no way to log certificate issuing below
            // m_transmittedDatas(cert, this, m_face);
            m_appLink->onReceiveData(*cert);
        }

        void CustomProducerBoot::OnInterestContent(std::shared_ptr<const ndn::Interest> interest) {
            // NS_LOG_FUNCTION(interest->getName());

            Name dataName(interest->getName());
            auto data = make_shared<Data>();
            data->setName(dataName);
            data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
            data->setContent(make_shared<::ndn::Buffer>(m_virtualPayloadSize));

            // Sign Data packet with default identity
            m_keyChain.sign(*data, m_signingInfo);

            // to create real wire encoding
            data->wireEncode();

            // LOGGING
            NS_LOG_INFO("Sending Data packet: " << data->getName());
            // NS_LOG_INFO("Signature: " << data->getSignature().getSignatureInfo());

            // Call trace (for logging purposes)
            m_transmittedDatas(data, this, m_face);
            m_appLink->onReceiveData(*data);
        }

        const ::ndn::security::v2::Certificate &CustomProducerBoot::createCertificate() {
            if (m_identityPrefix.size() == 0) {
                m_identityPrefix = m_prefix;
            }
            NS_LOG_INFO("Identity: " << m_identityPrefix);
            try {
                // clear keychain from any identical identities
                m_keyChain.deleteIdentity(m_keyChain.getPib().getIdentity(m_identityPrefix));
            } catch (::ndn::security::pib::Pib::Error &e) {
                // no identity found, proceed with the new identity creation
            }
            // create identity and certificates
            auto identity = m_keyChain.createIdentity(m_identityPrefix);
            auto &key     = identity.getDefaultKey();
            auto cert     = m_signCallback(key.getDefaultCertificate());
            m_keyChain.setDefaultIdentity(identity);  ///< define the default Data packet signer

            // sign certificate with trust anchor
            m_keyChain.deleteCertificate(key, cert.getName());
            m_keyChain.addCertificate(key, cert);
            return key.getDefaultCertificate();
        }

        void CustomProducerBoot::loadCertificateTrustAnchor() {
            NS_LOG_DEBUG("TrustAnchor: " << m_trustAnchorIdentityPrefix << " - CertFilename: " << m_trustAnchorCertFilename);
            auto rootIdentity = m_keyChain.createIdentity(m_trustAnchorIdentityPrefix);
            auto rootKey      = rootIdentity.getDefaultKey();
            m_keyChain.deleteKey(rootIdentity, rootKey);

            auto newCert   = ::ndn::io::load<::ndn::security::v2::Certificate>(m_trustAnchorCertFilename);
            auto keyParams = ::ndn::security::v2::KeyChain::getDefaultKeyParams();
            keyParams.setKeyId(newCert->getKeyId());
            rootKey = m_keyChain.createKey(rootIdentity, keyParams);
            m_keyChain.addCertificate(rootKey, *newCert);
        }

        void CustomProducerBoot::setSignCallback(std::function<::ndn::security::v2::Certificate(::ndn::security::v2::Certificate)> cb) {
            m_signCallback = cb;
        }

        // void CustomProducerBoot::signCertificateWithTrustAnchor() {
        //     // trust anchor identity
        //     NS_LOG_INFO("Identity: " << m_identityPrefix << " - TrustAnchor: " << m_trustAnchorIdentityPrefix);
        //     auto rootIdentity = m_keyChain.getPib().getIdentity(m_trustAnchorIdentityPrefix);

        //     // local identity
        //     auto identity = m_keyChain.getPib().getIdentity(m_identityPrefix);
        //     auto &key     = identity.getDefaultKey();
        //     auto cert     = key.getDefaultCertificate();
        //     m_keyChain.sign(cert, ::ndn::security::SigningInfo(rootIdentity));
        //     m_keyChain.addCertificate(key, cert);
        // }

        void CustomProducerBoot::printKeyChain() {
            for (auto identity : m_keyChain.getPib().getIdentities()) {
                NS_LOG_DEBUG("Identity: " << identity.getName());
                for (auto key : identity.getKeys()) {
                    NS_LOG_DEBUG("Key: " << key.getName() << " - Type: " << key.getKeyType());
                    for (auto cert : key.getCertificates()) {
                        NS_LOG_DEBUG(cert);
                    }
                }
            }
        }

    }  // namespace ndn
}  // namespace ns3