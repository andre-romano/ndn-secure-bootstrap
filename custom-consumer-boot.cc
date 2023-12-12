// custom-app.cpp

#include "custom-consumer-boot.hpp"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

#include "ns3/boolean.h"
#include "ns3/callback.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"

#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("CustomConsumerBoot");

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(CustomConsumerBoot);

    // register NS-3 type
    TypeId
    CustomConsumerBoot::GetTypeId() {
        static TypeId tid = TypeId("CustomConsumerBoot")
                                .SetParent<ndn::App>()
                                .AddConstructor<CustomConsumerBoot>()
                                .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                                              MakeStringAccessor(&CustomConsumerBoot::m_interestName), MakeStringChecker());
        return tid;

        //   .AddAttribute("Randomize",
        //                 "Type of send time randomization: none (default), uniform, exponential",
        //                 StringValue("none"),
        //                 MakeStringAccessor(&ConsumerCbr::SetRandomize, &ConsumerCbr::GetRandomize),
        //                 MakeStringChecker())
    }

    // Processing upon start of the application
    void
    CustomConsumerBoot::StartApplication() {
        // initialize ndn::App
        ndn::App::StartApplication();

        // Add entry to FIB for `/prefix/sub` (we are the Producer for this content)
        // ndn::FibHelper::AddRoute(GetNode(), "/prefix/sub", m_face, 0);

        // Schedule send of first interest
        Simulator::Schedule(Seconds(1.0), &CustomConsumerBoot::SendInterest, this);
    }

    // Processing when application is stopped
    void
    CustomConsumerBoot::StopApplication() {
        // cleanup ndn::App
        ndn::App::StopApplication();
    }

    void
    CustomConsumerBoot::SendInterest() {
        /////////////////////////////////////
        // Sending one Interest packet out //
        /////////////////////////////////////

        // Create and configure ndn::Interest
        auto interest                   = std::make_shared<ndn::Interest>(m_interestName);
        Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
        interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
        interest->setInterestLifetime(ndn::time::seconds(1));
        interest->setCanBePrefix(false);

        NS_LOG_DEBUG("Sending Interest packet for " << *interest);

        // Call trace (for logging purposes)
        m_transmittedInterests(interest, this, m_face);

        m_appLink->onReceiveInterest(*interest);

        // Schedule send of next interests
        Simulator::Schedule(Seconds(1.0), &CustomConsumerBoot::SendInterest, this);
    }

    // Callback that will be called when Data arrives
    void
    CustomConsumerBoot::OnData(std::shared_ptr<const ndn::Data> data) {
        NS_LOG_DEBUG("Receiving Data packet for " << data->getName());
        // std::cout << "DATA received for name " << data->getName() << std::endl;
    }

}  // namespace ns3