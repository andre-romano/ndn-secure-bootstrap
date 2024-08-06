// custom-app.hpp

#ifndef CUSTOM_CONSUMER_H_
#define CUSTOM_CONSUMER_H_

// system libs
#include <map>
#include <set>
#include <string>
#include <vector>

// NS3 / NDNSIM
#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/random-variable-stream.h"

// NDN-CXX
#include "ns3/ndnSIM/ndn-cxx/face.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/validator-config.hpp"

// custom includes
#include "custom-app.hpp"

namespace ns3 {
  namespace ndn {

    class CustomConsumer : public CustomApp {
    public:
      CustomConsumer();
      ~CustomConsumer();

      // register NS-3 type "CustomConsumer"
      static TypeId GetTypeId();

      void StartApplication() override;
      void StopApplication() override;

      void OnDataContent(std::shared_ptr<const ndn::Data> data) override;

      /**
       * @brief Set type of frequency randomization
       * @param value Either 'none', 'uniform', or 'exponential'
       */
      void SetRandomize(const std::string &value);
      std::string GetRandomize() const;

    private:
      void scheduleInterestContent();
      void sendInterestContent();

    private:
      std::string m_prefix;
      double m_frequency;
      ::ns3::Time m_lifeTime;
      std::string m_randomType;

      ::ns3::Ptr<::ns3::RandomVariableStream> m_random; ///< @brief Random generator for packet send
    };

  } // namespace ndn
} // namespace ns3

#endif // CUSTOM_CONSUMER_H_