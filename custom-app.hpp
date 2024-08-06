// custom-producer.hpp

#ifndef CUSTOM_APP_H
#define CUSTOM_APP_H

// NDN-CXX
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
#include <map>
#include <set>
#include <string>
#include <vector>

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

      virtual void OnDataKey(std::shared_ptr<const ndn::Data> data);
      virtual void OnDataContent(std::shared_ptr<const ndn::Data> data);

      // data validation callbacks
      virtual void OnDataValidated(const ndn::Data &data);
      virtual void OnDataValidationFailed(const ndn::Data &data,
                                          const ::ndn::security::v2::ValidationError &error);

    protected:
      void setSignValidityPeriod(int daysValid);
      void setShouldValidateData(bool validate);

      void readValidationRules(std::string filename);
      void readValidationRules(std::shared_ptr<const ndn::Data> data);

      void sendInterest(std::string name, ns3::Time lifeTime);
      void sendInterest(std::shared_ptr<ndn::Interest> data);

      void sendData(std::shared_ptr<ndn::Data> data);

      void sendCertificate(std::shared_ptr<const ndn::Interest> interest);
      void sendCertificate(const ndn::Name &keyName);
      void sendCertificate(std::shared_ptr<::ndn::security::v2::Certificate> cert);

      void addCertificate(::ndn::security::v2::Certificate &cert);
      const ::ndn::security::v2::Certificate &createCertificate(const ndn::Name &prefix);

      void printKeyChain();

      bool isDataValid();

      void eraseSendEvent(std::string &name);
      bool hasEvent(std::string &name);
      bool isEventRunning(std::string &name);

      void scheduleSubscribeSchema();
      void sendSubscribeSchema();

      void checkOnDataSchemaProtocol(std::shared_ptr<const ndn::Data> data);

    protected:
      std::shared_ptr<::ndn::Face> m_face_NDN_CXX; ///< @brief ndn::Face to allow real-world
                                                   ///< applications to work inside ns3

      std::shared_ptr<::ndn::ValidatorConfig> m_validator; ///< @brief validates data packets
      std::string m_validatorConf;

      ::ndn::security::v2::KeyChain m_keyChain;
      ::ndn::security::SigningInfo m_signingInfo;

      std::string m_schemaPrefix;        ///< @brief common SCHEMA prefix
      std::string m_schemaContentPrefix; ///< @brief SCHEMA content prefix

      std::string m_schemaSubscribePrefix; ///< @brief SCHEMA subscribe prefix
      ns3::Time m_schemaSubscribeLifetime; ///< @brief SCHEMA subscribe lifetime

      std::string m_signPrefix; ///< @brief common SIGN prefix (to request trust anchor signing)
      ns3::Time m_signLifetime; ///< @brief lifetime of SIGN interests

      std::map<std::string, ::ns3::EventId> m_sendEvents; ///< @brief pending "send packet" event
    private:
      bool m_shouldValidateData;
      bool m_dataIsValid;
    };

  } // namespace ndn
} // namespace ns3

#endif // CUSTOM_APP_H