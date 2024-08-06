// custom-app.cpp

#include "custom-app.hpp"

// NDN-CXX
#include "ns3/ndnSIM/ndn-cxx/security/v2/certificate-fetcher-from-network.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/v2/certificate-fetcher-offline.hpp"
#include "ns3/ndnSIM/ndn-cxx/util/io.hpp"

// NS3 / NDNSIM
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
#include "ns3/random-variable-stream.h"

#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"

// system libs
#include <limits>
#include <memory>
#include <string>

// namespace ns3 {
//     ATTRIBUTE_HELPER_CPP(IntMetricSet);

//     std::ostream &operator<<(std::ostream &os, const IntMetricSet
//     &intMetrics) {
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

NS_LOG_COMPONENT_DEFINE("CustomApp");

namespace ns3 {
  namespace ndn {

    // Necessary if you are planning to use ndn::AppHelper
    NS_OBJECT_ENSURE_REGISTERED(CustomApp);

    //////////////////////
    //     PUBLIC
    //////////////////////

    TypeId CustomApp::GetTypeId() {
      static TypeId tid =
          TypeId("CustomApp")
              .SetParent<ndn::App>()
              .AddConstructor<CustomApp>()
              .AddAttribute("SubscribeLifetime", "Lifeitme of Subscribe Interest packets",
                            TimeValue(Seconds(2.0)), MakeTimeAccessor(&CustomApp::m_schemaSubscribeLifetime),
                            MakeTimeChecker())
              .AddAttribute("SignLifetime", "Lifeitme of Sign Interest packets", TimeValue(Seconds(2.0)),
                            MakeTimeAccessor(&CustomApp::m_signLifetime), MakeTimeChecker())
              .AddAttribute("SignPrefix", "Sign (request) prefix", StringValue("/SIGN"),
                            MakeStringAccessor(&CustomApp::m_signPrefix), MakeStringChecker())
              .AddAttribute("SchemaPrefix", "Trust Schema prefix", StringValue("/SCHEMA"),
                            MakeStringAccessor(&CustomApp::m_schemaPrefix), MakeStringChecker())
              .AddAttribute("ValidatorConf", "Validator config filename",
                            StringValue("./scratch/sim_bootsec/config/validator.conf"),
                            MakeStringAccessor(&CustomApp::m_validatorConf), MakeStringChecker());
      // .AddAttribute("IntMetrics",
      //               "Set of INT metrics to collect",
      //               IntMetricSetValue(),
      //               MakeIntMetricSetAccessor(&CustomApp::m_collectMetrics),
      //               MakeIntMetricSetChecker())
      // .AddAttribute("IntFwd",
      //               "INT Fwd commands",
      //               IntFwdMapValue(),
      //               MakeIntFwdMapAccessor(&CustomApp::m_fwdMap),
      //               MakeIntFwdMapChecker());
      return tid;
    }

    CustomApp::CustomApp()
        : m_face_NDN_CXX(0), m_keyChain("pib-memory:", "tpm-memory:"),
          m_signingInfo(::ndn::security::SigningInfo::SIGNER_TYPE_NULL), m_dataIsValid(false) {
      setSignValidityPeriod(365);
      setShouldValidateData(true);
    }

    CustomApp::~CustomApp() {}

    void CustomApp::StartApplication() {
      NS_LOG_FUNCTION_NOARGS();
      ndn::App::StartApplication();

      // create ndn::Face to allow real-world application to interact inside ns3
      m_face_NDN_CXX = std::make_shared<::ndn::Face>();

      // setup validatorConf
      m_validator = std::make_shared<::ndn::security::ValidatorConfig>(
          std::make_unique<::ndn::security::v2::CertificateFetcherFromNetwork>(*m_face_NDN_CXX));
      readValidationRules(m_validatorConf);

      // define schema prefixes
      m_schemaContentPrefix = m_schemaPrefix + "/content";
      m_schemaSubscribePrefix = m_schemaPrefix + "/subscribe";
    }

    // Processing when application is stopped
    void CustomApp::StopApplication() {
      NS_LOG_FUNCTION_NOARGS();
      // cancel periodic packet generation
      for(const auto &pairSendEvent : m_sendEvents) {
        Simulator::Cancel(pairSendEvent.second);
      }

      // cleanup ndn::App
      ndn::App::StopApplication();
    }

    void CustomApp::OnInterest(std::shared_ptr<const ndn::Interest> interest) {
      ndn::App::OnInterest(interest); // forward call to perform app-level tracing

      // Note that Interests send out by the app will not be sent back to the app !
      (::ndn::security::v2::isValidKeyName(interest->getName()) ? OnInterestKey(interest)
                                                                : OnInterestContent(interest));
    }

    void CustomApp::OnData(std::shared_ptr<const ndn::Data> data) {
      ndn::App::OnData(data); // forward call to perform app-level tracing

      // Note that datas send out by the app will not be sent back to the app !
      NS_LOG_DEBUG("Receiving Data packet: " << data->getName()
                                             << " - KeyLocator: " << data->getSignature().getKeyLocator());
      if(m_shouldValidateData) {
        NS_LOG_DEBUG("Validating Data ... ");
        m_validator->validate(*data, MakeCallback(&CustomApp::OnDataValidated, this),
                              MakeCallback(&CustomApp::OnDataValidationFailed, this));
      } else {
        NS_LOG_DEBUG("Validation SKIPPED");
        (::ndn::security::v2::isValidKeyName(data->getName()) ? OnDataKey(data) : OnDataContent(data));
      }
    }

    void CustomApp::OnInterestKey(std::shared_ptr<const ndn::Interest> interest) {}
    void CustomApp::OnInterestContent(std::shared_ptr<const ndn::Interest> interest) {}

    void CustomApp::OnDataKey(std::shared_ptr<const ndn::Data> data) {}
    void CustomApp::OnDataContent(std::shared_ptr<const ndn::Data> data) {}

    void CustomApp::OnDataValidated(const ndn::Data &data) {
      NS_LOG_FUNCTION_NOARGS();
      m_dataIsValid = true;
      auto dataPtr = std::make_shared<const ndn::Data>(data);
      (::ndn::security::v2::isValidKeyName(dataPtr->getName()) ? OnDataKey(dataPtr) : OnDataContent(dataPtr));
    }
    void CustomApp::OnDataValidationFailed(const ndn::Data &data,
                                           const ::ndn::security::v2::ValidationError &error) {
      NS_LOG_FUNCTION(data.getName());
      m_dataIsValid = false;
    }

    //////////////////////
    //     PROTECTED
    //////////////////////

    void CustomApp::setSignValidityPeriod(int daysValid) {
      ::ndn::SignatureInfo signatureInfo;
      signatureInfo.setValidityPeriod(::ndn::security::ValidityPeriod(
          ndn::time::system_clock::TimePoint(), ndn::time::system_clock::now() + ndn::time::days(daysValid)));
      m_signingInfo.setSignatureInfo(signatureInfo);
    }

    void CustomApp::setShouldValidateData(bool validate) { m_shouldValidateData = validate; }

    /// @brief must be run after ndn::App.StartApplication()
    void CustomApp::readValidationRules(std::string filename) {
      try {
        m_validator->load(filename);
      } catch(const std::exception &e) {
        throw std::runtime_error("Failed to load validation rules file='" + filename +
                                 "' - Error=" + e.what());
      }
    }

    /// @brief must be run after ndn::App.StartApplication()
    void CustomApp::readValidationRules(std::shared_ptr<const ndn::Data> data) {
      try {
        NS_LOG_INFO("Updating trust schema ... ");
        auto buffer = data->getContent().getBuffer();
        auto dataStr = std::string(buffer->begin(), buffer->end());
        m_validator->load(dataStr, m_validatorConf);
        NS_LOG_INFO("Trust schema UPDATED");
      } catch(const std::exception &e) {
        NS_LOG_ERROR("Failed load validation rules from '" << data->getName() << "' - Error=" << e.what());
      }
    }

    void CustomApp::sendInterest(std::string name, ns3::Time lifeTime) {
      // Create and configure ndn::Interest
      auto interest = std::make_shared<ndn::Interest>(name);
      ::ns3::Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
      interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
      interest->setInterestLifetime(ndn::time::milliseconds(lifeTime.GetMilliSeconds()));
      interest->setCanBePrefix(false);

      // send packet
      sendInterest(interest);
    }

    void CustomApp::sendInterest(std::shared_ptr<ndn::Interest> interest) {
      // to create real wire encoding
      interest->wireEncode();

      // Call trace (for logging purposes), send interest, schedule next interests
      NS_LOG_DEBUG("Sending Interest packet: " << *interest);
      m_transmittedInterests(interest, this, m_face);
      m_appLink->onReceiveInterest(*interest);
    }

    void CustomApp::sendData(std::shared_ptr<ndn::Data> data) {
      // to create real wire encoding
      data->wireEncode();

      // Call trace (for logging purposes), send data packet
      NS_LOG_INFO("Sending Data packet: " << data->getName());
      // NS_LOG_INFO("Signature: " << data->getSignature().getSignatureInfo());
      m_transmittedDatas(data, this, m_face);
      m_appLink->onReceiveData(*data);
    }

    void CustomApp::sendCertificate(std::shared_ptr<const ndn::Interest> interest) {
      sendCertificate(interest->getName());
    }

    void CustomApp::sendCertificate(const ndn::Name &keyName) {
      NS_LOG_FUNCTION(keyName);

      try {
        auto identity =
            m_keyChain.getPib().getIdentity(::ndn::security::v2::extractIdentityFromKeyName(keyName));
        auto key = identity.getKey(keyName);
        auto cert = std::make_shared<::ndn::security::v2::Certificate>(key.getDefaultCertificate());

        sendCertificate(cert);
      } catch(::ndn::security::pib::Pib::Error &e) {
        NS_LOG_ERROR(
            "Could not find certificate for: " << ::ndn::security::v2::extractIdentityFromKeyName(keyName));
      }
    }

    void CustomApp::sendCertificate(std::shared_ptr<::ndn::security::v2::Certificate> cert) {
      // to create real wire encoding
      cert->wireEncode();

      // LOGGING
      NS_LOG_INFO("Sending Certificate packet: " << cert->getName());
      NS_LOG_INFO("Signature: " << cert->getSignature().getSignatureInfo());

      // Call trace (for logging purposes) - no way to log certificate
      // issuing below m_transmittedDatas(cert, this, m_face);
      m_appLink->onReceiveData(*cert);
    }

    const ::ndn::security::v2::Certificate &CustomApp::createCertificate(const ndn::Name &prefix) {
      NS_LOG_INFO("Creating certificate/identity for '" << prefix << "' ...");
      try {
        // clear keychain from any identical identities
        m_keyChain.deleteIdentity(m_keyChain.getPib().getIdentity(prefix));
      } catch(::ndn::security::pib::Pib::Error &e) {
        // no identity found, proceed with the new identity creation
      }
      // create identity and certificates
      auto identity = m_keyChain.createIdentity(prefix);
      auto &key = identity.getDefaultKey();
      return key.getDefaultCertificate();
    }

    void CustomApp::addCertificate(::ndn::security::v2::Certificate &cert) {
      NS_LOG_FUNCTION(cert.getName());
      // add certificate to key chain
      auto keyName = cert.getKeyName();
      try {
        auto identity =
            m_keyChain.getPib().getIdentity(::ndn::security::v2::extractIdentityFromCertName(keyName));
        auto key = identity.getKey(keyName);
        m_keyChain.deleteCertificate(key, cert.getName());
        m_keyChain.addCertificate(key, cert);
        NS_LOG_INFO("Added certificate '" << keyName << "' to local keyChain");
      } catch(::ndn::security::pib::Pib::Error &e) {
        NS_LOG_ERROR(
            "Could not find Identity for: " << ::ndn::security::v2::extractIdentityFromKeyName(keyName));
      }
    }

    void CustomApp::printKeyChain() {
      for(auto identity : m_keyChain.getPib().getIdentities()) {
        NS_LOG_DEBUG("Identity: " << identity.getName());
        for(auto key : identity.getKeys()) {
          NS_LOG_DEBUG("Key: " << key.getName() << " - Type: " << key.getKeyType());
          for(auto cert : key.getCertificates()) {
            NS_LOG_DEBUG(cert);
          }
        }
      }
    }

    bool CustomApp::isDataValid() { return m_dataIsValid; }

    bool CustomApp::hasEvent(std::string &name) { return (m_sendEvents.find(name) != m_sendEvents.end()); }

    bool CustomApp::isEventRunning(std::string &name) {
      return (hasEvent(name) && m_sendEvents[name].IsRunning());
    }

    void CustomApp::scheduleSubscribeSchema() {
      if(!hasEvent(m_schemaSubscribePrefix)) {
        m_sendEvents[m_schemaSubscribePrefix] =
            Simulator::Schedule(Seconds(0.0), &CustomApp::sendSubscribeSchema, this);
      } else if(!isEventRunning(m_schemaSubscribePrefix)) {
        m_sendEvents[m_schemaSubscribePrefix] =
            Simulator::Schedule(m_schemaSubscribeLifetime, &CustomApp::sendSubscribeSchema, this);
      }
    }

    void CustomApp::sendSubscribeSchema() {
      sendInterest(m_schemaSubscribePrefix, m_schemaSubscribeLifetime);
      scheduleSubscribeSchema();
    }

    void CustomApp::checkOnDataSchemaProtocol(std::shared_ptr<const ndn::Data> data) {
      if(data->getName().isPrefixOf(m_schemaContentPrefix)) {
        readValidationRules(data);
      } else if(data->getName().isPrefixOf(m_schemaSubscribePrefix)) {
        NS_LOG_INFO("Sending SCHEMA content Interest for '" << m_schemaContentPrefix << "' ... ");
        sendInterest(m_schemaContentPrefix, m_schemaSubscribeLifetime);
      }
    }

  } // namespace ndn
} // namespace ns3