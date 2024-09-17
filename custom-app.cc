// custom-app.cpp

#include "custom-app.hpp"
#include "custom-utils.hpp"

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
#include <fstream>
#include <limits>
#include <stdexcept> // for standard exception classes
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
    //     PUBLIC       //
    //////////////////////

    bool CustomApp::isValidKeyName(const ::ndn::Name &keyName) {
      return ::ndn::security::v2::isValidKeyName(keyName);
    }

    bool CustomApp::isValidCertificateName(const ::ndn::Name &certName) {
      return ::ndn::security::v2::Certificate::isValidName(certName);
    }

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
                            MakeNameAccessor(&CustomApp::m_signPrefix), MakeNameChecker())
              .AddAttribute("SchemaPrefix", "Trust Schema prefix", StringValue("/SCHEMA"),
                            MakeNameAccessor(&CustomApp::m_schemaPrefix), MakeNameChecker())
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
      m_validatorRoot = std::make_shared<::ndn::security::v2::validator_config::ConfigSection>();
      try {
        readValidationRules();
      } catch(const std::exception &e) {
        throw std::runtime_error("Failed to load validation rules file='" + m_validatorConf +
                                 "' - Error=" + e.what());
      }

      // define schema prefixes /zoneA/SCHEMA/SUBSCRIBE
      m_schemaContentPrefix = m_schemaPrefix.deepCopy().append("CONTENT");
      m_schemaSubscribePrefix = m_schemaPrefix.deepCopy().append("SUBSCRIBE");
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
      (isValidKeyName(interest->getName()) ? OnInterestKey(interest) : OnInterestContent(interest));
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
        (isValidCertificateName(data->getName()) ? OnDataCertificate(data) : OnDataContent(data));
      }
    }

    void CustomApp::OnInterestKey(std::shared_ptr<const ndn::Interest> interest) {}
    void CustomApp::OnInterestContent(std::shared_ptr<const ndn::Interest> interest) {}

    void CustomApp::OnDataCertificate(std::shared_ptr<const ndn::Data> data) {}
    void CustomApp::OnDataContent(std::shared_ptr<const ndn::Data> data) {}

    void CustomApp::OnDataValidated(const ndn::Data &data) {
      NS_LOG_FUNCTION_NOARGS();
      m_dataIsValid = true;
      auto dataPtr = std::make_shared<const ndn::Data>(data);
      (isValidCertificateName(dataPtr->getName()) ? OnDataCertificate(dataPtr) : OnDataContent(dataPtr));
    }
    void CustomApp::OnDataValidationFailed(const ndn::Data &data,
                                           const ::ndn::security::v2::ValidationError &error) {
      NS_LOG_FUNCTION(data.getName());
      m_dataIsValid = false;
      printValidationRules();
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
    bool CustomApp::getShouldValidateData() { return m_shouldValidateData; }

    /// @brief must be run after ndn::App.StartApplication()
    void CustomApp::readValidationRules() {
      try {
        // Ler o arquivo INFO
        NS_LOG_INFO("Reading trust schema from file '" << m_validatorConf << "' ... ");
        boost::property_tree::read_info(m_validatorConf, *m_validatorRoot);
        reloadValidationRules();
      } catch(const std::exception &e) {
        throw std::runtime_error("Failed to load validation rules file='" + m_validatorConf +
                                 "' - Error=" + e.what());
      }
    }

    /// @brief must be run after ndn::App.StartApplication()
    void CustomApp::readValidationRules(std::shared_ptr<const ndn::Data> data) {
      try {
        NS_LOG_INFO("Reading trust schema from Data packet '" << data->getName() << "'... ");
        auto &contentBlock = data->getContent();
        std::string contentStr(contentBlock.value_begin(), contentBlock.value_end());
        std::stringstream inputStream(contentStr);
        NS_LOG_INFO("InputStream: \n" << inputStream.str());
        boost::property_tree::read_info(inputStream, *m_validatorRoot);
        reloadValidationRules();
      } catch(const std::exception &e) {
        throw std::runtime_error("Failed load validation rules from '" + data->getName().toUri() +
                                 "' - Error=" + e.what());
      }
    }

    /// @brief Write Validation Rules from memory into a file
    void CustomApp::writeValidationRules() {
      try {
        NS_LOG_INFO("Write trust schema into file '" << m_validatorConf << "'... ");
        ::boost::property_tree::write_info(m_validatorConf, *m_validatorRoot);
      } catch(const std::exception &e) {
        throw std::runtime_error("Failed to write validation rules into file='" + m_validatorConf +
                                 "' - Error=" + e.what());
      }
    }

    void CustomApp::clearValidationRules() {
      NS_LOG_FUNCTION_NOARGS();
      m_validatorRoot->clear();
      reloadValidationRules();
    }

    /// @brief get a copy of the validation rules stored in memory
    /// @return copy of validation rules (string format)
    std::string CustomApp::getValidationRules() {
      std::stringstream stream;
      ::boost::property_tree::write_info(stream, *m_validatorRoot);
      return stream.str();
    }

    void CustomApp::addValidationRule(std::string dataRegex, std::string keyLocatorRegex) {
      try {
        NS_LOG_FUNCTION("data=" << dataRegex << " , keylocator=" << keyLocatorRegex);
        ::ndn::security::v2::validator_config::ConfigSection rule;
        rule.put("id", dataRegex);
        rule.put("for", "data");
        rule.put("filter.type", "name");
        rule.put("filter.regex", dataRegex);
        rule.put("checker.type", "customized");
        rule.put("checker.sig-type", "rsa-sha256");
        rule.put("checker.key-locator.type", "name");
        rule.put("checker.key-locator.regex", keyLocatorRegex);

        m_validatorRoot->push_front(std::make_pair("rule", rule));
        reloadValidationRules();
      } catch(const std::exception &e) {
        throw std::runtime_error("Failed add validation rule for data=''" + dataRegex + "' , keyLocator='" +
                                 keyLocatorRegex + "' - Error=" + e.what());
      }
    }

    void CustomApp::addTrustAnchor(std::string filename) {
      try {
        NS_LOG_FUNCTION(filename);

        if(!::utils::fileExists(filename)) {
          throw std::runtime_error("file '" + filename + "' does not exist");
        }

        ::ndn::security::v2::validator_config::ConfigSection trustAnchor;
        trustAnchor.put("type", "file");
        trustAnchor.put("file-name", filename);

        m_validatorRoot->push_front(std::make_pair("trust-anchor", trustAnchor));

        reloadValidationRules();
      } catch(const std::exception &e) {
        throw std::runtime_error("Failed to add validation trust-anchor in filename ''" + filename +
                                 "' - Error=" + e.what());
      }
    }

    std::string CustomApp::getValidationRegex(const ::ndn::Name &prefix) {
      std::stringstream res;
      for(auto &item : prefix) {
        res << "<" << item << ">";
      }
      return res.str();
    }

    void CustomApp::sendInterest(::ndn::Name name, ns3::Time lifeTime, const InterestOptions &opts) {
      sendInterest(name.toUri(), lifeTime, opts);
    }

    void CustomApp::sendInterest(std::string name, ns3::Time lifeTime, const InterestOptions &opts) {
      NS_LOG_FUNCTION(name);
      // Create and configure ndn::Interest
      auto interest = std::make_shared<ndn::Interest>(name);
      ::ns3::Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
      interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
      interest->setInterestLifetime(ndn::time::milliseconds(lifeTime.GetMilliSeconds()));
      interest->setCanBePrefix(opts.canBePrefix);
      interest->setMustBeFresh(opts.mustBeFresh);

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

    void CustomApp::sendCertificate(std::shared_ptr<const ndn::Interest> interest, const DataOptions &opts) {
      sendCertificate(interest->getName(), opts);
    }

    void CustomApp::sendCertificate(const ndn::Name &keyName, const DataOptions &opts) {
      NS_LOG_FUNCTION(keyName);

      try {
        auto identity =
            m_keyChain.getPib().getIdentity(::ndn::security::v2::extractIdentityFromKeyName(keyName));
        auto key = identity.getKey(keyName);
        auto cert = std::make_shared<::ndn::security::v2::Certificate>(key.getDefaultCertificate());

        sendCertificate(cert);
      } catch(::ndn::security::pib::Pib::Error &e) {
        throw std::runtime_error("Could not find certificate for: " +
                                 ::ndn::security::v2::extractIdentityFromKeyName(keyName).toUri());
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
            m_keyChain.getPib().getIdentity(::ndn::security::v2::extractIdentityFromKeyName(keyName));
        auto key = identity.getKey(keyName);
        m_keyChain.deleteCertificate(key, cert.getName());
        m_keyChain.addCertificate(key, cert);
        m_keyChain.setDefaultCertificate(key, cert);
        NS_LOG_INFO("Added certificate '" << cert.getName() << "' in local keyChain");
      } catch(::ndn::security::pib::Pib::Error &e) {
        throw std::runtime_error("Could not find Identity for: " +
                                 ::ndn::security::v2::extractIdentityFromKeyName(keyName).toUri());
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

    void CustomApp::printValidationRules() {
      // print validatorConf INFO output
      NS_LOG_INFO("\n" << getValidationRules());
    }

    bool CustomApp::isDataValid() { return m_dataIsValid; }

    bool CustomApp::hasEvent(std::string &name) { return (m_sendEvents.find(name) != m_sendEvents.end()); }

    bool CustomApp::isEventRunning(std::string &name) {
      return (hasEvent(name) && m_sendEvents[name].IsRunning());
    }

    void CustomApp::scheduleSubscribeSchema() {
      auto schemaSubscribePrefixStr = m_schemaSubscribePrefix.toUri();
      if(!hasEvent(schemaSubscribePrefixStr)) {
        m_sendEvents[schemaSubscribePrefixStr] =
            Simulator::Schedule(Seconds(0.0), &CustomApp::sendSubscribeSchema, this);
      } else if(!isEventRunning(schemaSubscribePrefixStr)) {
        m_sendEvents[schemaSubscribePrefixStr] =
            Simulator::Schedule(m_schemaSubscribeLifetime, &CustomApp::sendSubscribeSchema, this);
      }
    }

    void CustomApp::sendSubscribeSchema() {
      InterestOptions opts;
      opts.canBePrefix = false;
      opts.mustBeFresh = true;
      sendInterest(m_schemaSubscribePrefix, m_schemaSubscribeLifetime, opts);
      scheduleSubscribeSchema();
    }

    //////////////////////
    //     PRIVATE
    //////////////////////

    /// @brief reload validation rules stored in memory
    void CustomApp::reloadValidationRules() {
      // print Validation Rules
      printValidationRules();
      // load validatorRoot into validatorConf
      m_validator->resetAnchors();
      m_validator->resetVerifiedCertificates();
      m_validator->load(*m_validatorRoot, m_validatorConf);
      NS_LOG_INFO("Trust schema RELOADED");
    }

  } // namespace ndn
} // namespace ns3