// custom-producer.hpp

#ifndef CUSTOM_PRODUCER_BOOT_H_
#define CUSTOM_PRODUCER_BOOT_H_

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include "ns3/ndnSIM/apps/ndn-producer.hpp"

#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/signing-helpers.hpp"

#include <functional>
#include <set>
#include <string>

#include "custom-app.hpp"

namespace ns3 {
  namespace ndn {

    class CustomProducer : public CustomApp {

    public:
      static TypeId GetTypeId();
      CustomProducer();
      ~CustomProducer();

      void StartApplication() override;
      void StopApplication() override;

      void OnInterestKey(std::shared_ptr<const ndn::Interest> interest) override;
      void OnInterestContent(std::shared_ptr<const ndn::Interest> interest) override;

      void OnDataKey(std::shared_ptr<const ndn::Data> data) override;
      void OnDataContent(std::shared_ptr<const ndn::Data> data) override;

      void OnDataValidationFailed(const ndn::Data &data,
                                  const ::ndn::security::v2::ValidationError &error) override;

    protected:
      std::string m_prefix;
      uint32_t m_virtualPayloadSize;
      Time m_freshness;
      std::string m_identityPrefix;

    private:
      void scheduleSignInterest();
      void sendSignInterest();
    };

  } // namespace ndn
} // namespace ns3

#endif // CUSTOM_PRODUCER_BOOT_H_