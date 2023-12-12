// custom-app.hpp

#ifndef CUSTOM_CONSUMER_BOOT_H_
#define CUSTOM_CONSUMER_BOOT_H_

#include <string>

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

namespace ns3 {

    class CustomConsumerBoot : public ndn::App {
      public:
        // register NS-3 type "CustomConsumerBoot"
        static TypeId GetTypeId();

        // (overridden from ndn::App) Processing upon start of the application
        virtual void StartApplication();

        // (overridden from ndn::App) Processing when application is stopped
        virtual void StopApplication();

        // (overridden from ndn::App) Callback that will be called when Data arrives
        virtual void OnData(std::shared_ptr<const ndn::Data> contentObject);

        void setPrefix(std::string name);

      private:
        std::string m_interestName;
        void SendInterest();
    };

}  // namespace ns3

#endif  // CUSTOM_CONSUMER_BOOT_H_