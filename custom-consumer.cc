// custom-app.cpp

#include "custom-consumer.hpp"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"

#include "ns3/boolean.h"
#include "ns3/callback.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"

#include "ns3/ndnSIM/ndn-cxx/security/v2/certificate-fetcher-from-network.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/v2/certificate-fetcher-offline.hpp"
#include "ns3/ndnSIM/ndn-cxx/util/io.hpp"

NS_LOG_COMPONENT_DEFINE("CustomConsumer");

namespace ns3 {
  namespace ndn {

    NS_OBJECT_ENSURE_REGISTERED(CustomConsumer);

    //////////////////////
    //     PUBLIC
    //////////////////////

    // register NS-3 type
    TypeId CustomConsumer::GetTypeId() {
      static TypeId tid =
          TypeId("CustomConsumer")
              .SetParent<CustomApp>()
              .AddConstructor<CustomConsumer>()
              .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                            MakeNameAccessor(&CustomConsumer::m_prefix), MakeNameChecker())
              .AddAttribute("Frequency", "Frequency of interest packets", StringValue("1.0"),
                            MakeDoubleAccessor(&CustomConsumer::m_frequency), MakeDoubleChecker<double>())
              .AddAttribute("LifeTime", "Lifetime of interest packets (in seconds)", StringValue("2s"),
                            MakeTimeAccessor(&CustomConsumer::m_lifeTime), MakeTimeChecker())
              .AddAttribute("Randomize",
                            "Type of send time randomization: none (default), uniform, "
                            "exponential",
                            StringValue("none"),
                            MakeStringAccessor(&CustomConsumer::SetRandomize, &CustomConsumer::GetRandomize),
                            MakeStringChecker());
      return tid;
    }

    CustomConsumer::CustomConsumer() : CustomApp() {}
    CustomConsumer::~CustomConsumer() {}

    // Processing upon start of the application
    void CustomConsumer::StartApplication() {
      NS_LOG_FUNCTION_NOARGS();
      ndn::CustomApp::StartApplication();

      // Schedule send of first interest
      scheduleSubscribeSchema();
      scheduleInterestContent();
    }

    // Processing when application is stopped
    void CustomConsumer::StopApplication() {
      // CUSTOM CLEANUP HERE (if needed)

      // cleanup ndn::App
      ndn::CustomApp::StopApplication();
    }

    void CustomConsumer::OnDataContent(std::shared_ptr<const ndn::Data> data) {
      NS_LOG_FUNCTION(data->getName());
      checkOnDataSchemaProtocol(data);
    }

    void CustomConsumer::SetRandomize(const std::string &value) {
      if(value == "uniform") {
        m_random = CreateObject<UniformRandomVariable>();
        m_random->SetAttribute("Min", DoubleValue(0.0));
        m_random->SetAttribute("Max", DoubleValue(2 * 1.0 / m_frequency));
      } else if(value == "exponential") {
        m_random = CreateObject<ExponentialRandomVariable>();
        m_random->SetAttribute("Mean", DoubleValue(1.0 / m_frequency));
        m_random->SetAttribute("Bound", DoubleValue(50 * 1.0 / m_frequency));
      } else
        m_random = 0;

      m_randomType = value;
    }

    std::string CustomConsumer::GetRandomize() const { return m_randomType; }

    //////////////////////
    //     PRIVATE
    //////////////////////

    void CustomConsumer::scheduleInterestContent() {
      auto consumerPrefixStr = m_prefix.toUri();
      if(!hasEvent(consumerPrefixStr)) {
        m_sendEvents[consumerPrefixStr] =
            Simulator::Schedule(Seconds(0.0), &CustomConsumer::sendInterestContent, this);
      } else if(!isEventRunning(consumerPrefixStr)) {
        m_sendEvents[consumerPrefixStr] =
            Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency) : Seconds(m_random->GetValue()),
                                &CustomConsumer::sendInterestContent, this);
      }
    }

    void CustomConsumer::sendInterestContent() {
      sendInterest(m_prefix, m_lifeTime);
      scheduleInterestContent();
    }

  } // namespace ndn
} // namespace ns3