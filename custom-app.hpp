// custom-producer.hpp

#ifndef CUSTOM_APP_H
#define CUSTOM_APP_H

// NDN-CXX
#include "ns3/ndnSIM/ndn-cxx/lp/fields.hpp"
#include "ns3/ndnSIM/ndn-cxx/lp/tags.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/signing-helpers.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/validator-config.hpp"

// NS3 / NDNSIM
#include "ns3/ndnSIM/apps/ndn-producer.hpp"

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

// system libs
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// boost libs
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>

// custom includes
#include "custom-utils.hpp"

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

    class CustomApp : public App {

    public:
      struct InterestOptions {
        // Member variables
        bool canBePrefix;
        bool mustBeFresh;

        InterestOptions() : canBePrefix(false), mustBeFresh(false) {}
      };

      struct DataOptions {
        DataOptions() {}
      };

    public:
      static bool isValidKeyName(const ::ndn::Name &keyName);
      static bool isValidCertificateName(const ::ndn::Name &certName);

      static TypeId GetTypeId();

      CustomApp();
      ~CustomApp();

      virtual void StartApplication() override;
      virtual void StopApplication() override;

      // when interest/data are received
      void OnInterest(std::shared_ptr<const ndn::Interest> interest) override;
      void OnData(std::shared_ptr<const ndn::Data> data) override;

      virtual void OnInterestKey(std::shared_ptr<const ndn::Interest> interest);
      virtual void OnInterestContent(std::shared_ptr<const ndn::Interest> interest);

      virtual void OnDataCertificate(std::shared_ptr<const ndn::Data> data);
      virtual void OnDataContent(std::shared_ptr<const ndn::Data> data);

      // data validation callbacks
      virtual void OnDataValidated(const ndn::Data &data);
      virtual void OnDataValidationFailed(const ndn::Data &data,
                                          const ::ndn::security::v2::ValidationError &error);

    protected:
      void setSignValidityPeriod(int daysValid);
      void setShouldValidateData(bool validate);
      bool getShouldValidateData();

      void readValidationRules();
      void readValidationRules(std::shared_ptr<const ndn::Data> data);

      void writeValidationRules();

      void clearValidationRules();
      std::string getValidationRules();

      void addValidationRule(std::string dataRegex, std::string keyLocatorRegex);
      void addTrustAnchor(std::string filename);

      std::string getValidationRegex(const ::ndn::Name &prefix);

      void sendInterest(::ndn::Name name, ns3::Time lifeTime,
                        const InterestOptions &opts = InterestOptions());
      void sendInterest(std::string name, ns3::Time lifeTime,
                        const InterestOptions &opts = InterestOptions());
      void sendInterest(std::shared_ptr<ndn::Interest> interest);

      void sendData(std::shared_ptr<ndn::Data> data);

      void sendCertificate(std::shared_ptr<const ndn::Interest> interest,
                           const DataOptions &opts = DataOptions());
      void sendCertificate(const ndn::Name &keyName, const DataOptions &opts = DataOptions());
      void sendCertificate(std::shared_ptr<::ndn::security::v2::Certificate> cert);

      void addCertificate(::ndn::security::v2::Certificate &cert);
      const ::ndn::security::v2::Certificate &createCertificate(const ndn::Name &prefix);

      void printKeyChain();
      void printValidationRules();

      bool isDataValid();

      void eraseSendEvent(std::string &name);
      bool hasEvent(std::string &name);
      bool isEventRunning(std::string &name);

      void scheduleSubscribeSchema();
      void sendSubscribeSchema();

    private:
      void reloadValidationRules();

    protected:
      std::shared_ptr<::ndn::Face> m_face_NDN_CXX; ///< @brief ndn::Face to allow real-world
                                                   ///< applications to work inside ns3

      std::string m_validatorConf;

      ::ndn::Name m_schemaPrefix;        ///< @brief common SCHEMA prefix
      ::ndn::Name m_schemaContentPrefix; ///< @brief SCHEMA content prefix

      ::ndn::Name m_schemaSubscribePrefix; ///< @brief SCHEMA subscribe prefix
      ns3::Time m_schemaSubscribeLifetime; ///< @brief SCHEMA subscribe lifetime

      ::ndn::Name m_signPrefix; ///< @brief common SIGN prefix (to request trust anchor signing)
      ns3::Time m_signLifetime; ///< @brief lifetime of SIGN interests

      ::ndn::security::v2::KeyChain m_keyChain;
      ::ndn::security::SigningInfo m_signingInfo;

      std::map<std::string, ::ns3::EventId> m_sendEvents; ///< @brief pending "send packet" event

    private:
      std::shared_ptr<::ndn::ValidatorConfig> m_validator; ///< @brief validates data packets
      std::shared_ptr<::ndn::security::v2::validator_config::ConfigSection> m_validatorRoot;

      bool m_shouldValidateData;
      bool m_dataIsValid;
    };

  } // namespace ndn
} // namespace ns3

#endif // CUSTOM_APP_H