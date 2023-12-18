// custom-app.hpp

#ifndef CUSTOM_CONSUMER_BOOT_H_
#define CUSTOM_CONSUMER_BOOT_H_

#include <string>

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/random-variable-stream.h"

#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/validator-config.hpp"

namespace ns3 {

    class CustomConsumerBoot : public ndn::App {
      public:
        CustomConsumerBoot();
        ~CustomConsumerBoot();

        // register NS-3 type "CustomConsumerBoot"
        static TypeId GetTypeId();

        // (overridden from ndn::App) Processing upon start of the application
        virtual void StartApplication();

        // (overridden from ndn::App) Processing when application is stopped
        virtual void StopApplication();

        // (overridden from ndn::App) Callback that will be called when Data arrives
        virtual void OnData(std::shared_ptr<const ndn::Data> data);

      public:
        void OnDataValidated(const ndn::Data &data);
        void OnDataValidationFailed(const ndn::Data &data, const ::ndn::security::v2::ValidationError &error);

        /**
         * @brief Set type of frequency randomization
         * @param value Either 'none', 'uniform', or 'exponential'
         */
        void SetRandomize(const std::string &value);

        /**
         * @brief Get type of frequency randomization
         * @returns either 'none', 'uniform', or 'exponential'
         */
        std::string GetRandomize() const;

      private:
        void SetupValidator();

        void SendInterest();
        void ScheduleNextPacket();

      private:
        std::string m_interestName;
        double m_frequency;
        Time m_interestLifeTime;

        std::string m_validatorFilename;
        std::shared_ptr<::ndn::ValidatorConfig> m_validator;

        bool m_firstTime;     ///< @brief First time sending an Interest packet
        EventId m_sendEvent;  ///< @brief EventId of pending "send packet" event

        Ptr<RandomVariableStream> m_random;  ///< @brief Random generator for packet send
        std::string m_randomType;
    };

}  // namespace ns3

#endif  // CUSTOM_CONSUMER_BOOT_H_