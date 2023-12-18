// custom-app.cpp

#include "custom-consumer-boot.hpp"
#include "certificate-fetcher-from-ns3.hpp"

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

NS_LOG_COMPONENT_DEFINE("CustomConsumerBoot");

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(CustomConsumerBoot);

    // register NS-3 type
    TypeId CustomConsumerBoot::GetTypeId() {
        static TypeId tid = TypeId("CustomConsumerBoot")
                                .SetParent<ndn::App>()
                                .AddConstructor<CustomConsumerBoot>()
                                .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                                              MakeStringAccessor(&CustomConsumerBoot::m_interestName), MakeStringChecker())
                                .AddAttribute("Frequency", "Frequency of interest packets", StringValue("1.0"),
                                              MakeDoubleAccessor(&CustomConsumerBoot::m_frequency), MakeDoubleChecker<double>())
                                .AddAttribute("LifeTime", "Lifetime of interest packets (in seconds)", StringValue("2s"),
                                              MakeTimeAccessor(&CustomConsumerBoot::m_interestLifeTime), MakeTimeChecker())
                                .AddAttribute("Randomize",
                                              "Type of send time randomization: none (default), uniform, exponential",
                                              StringValue("none"),
                                              MakeStringAccessor(&CustomConsumerBoot::SetRandomize, &CustomConsumerBoot::GetRandomize),
                                              MakeStringChecker())
                                .AddAttribute("ValidatorConf", "Validator config filename", StringValue("./scratch/sim_bootsec/config/validator.conf"),
                                              MakeStringAccessor(&CustomConsumerBoot::m_validatorFilename), MakeStringChecker());
        return tid;
    }

    CustomConsumerBoot::CustomConsumerBoot() {}
    CustomConsumerBoot::~CustomConsumerBoot() {}

    // Processing upon start of the application
    void
    CustomConsumerBoot::StartApplication() {
        NS_LOG_FUNCTION_NOARGS();
        ndn::App::StartApplication();

        // setup validator (trust schema)
        SetupValidator();

        // Add entry to FIB for `/prefix/sub` (we are the Producer for this content)
        // ndn::FibHelper::AddRoute(GetNode(), "/prefix/sub", m_face, 0);

        // Schedule send of first interest
        m_firstTime = true;
        ScheduleNextPacket();
    }

    // Processing when application is stopped
    void CustomConsumerBoot::StopApplication() {
        // cancel periodic packet generation
        Simulator::Cancel(m_sendEvent);

        // cleanup ndn::App
        ndn::App::StopApplication();
    }

    /// @brief must be run after ndn::App.StartApplication()
    void CustomConsumerBoot::SetupValidator() {
        // m_validator = std::make_shared<::ndn::security::ValidatorConfig>(std::make_unique<::ndn::security::v2::CertificateFetcherFromNS3>(m_appLink));
        m_validator = std::make_shared<::ndn::security::ValidatorConfig>(std::make_unique<::ndn::security::v2::CertificateFetcherOffline>());
        try {
            m_validator->load(m_validatorFilename);
        } catch (const std::exception &e) {
            throw std::runtime_error("Failed to load validation rules file=" + m_validatorFilename +
                                     " Error=" + e.what());
        }
    }

    void CustomConsumerBoot::ScheduleNextPacket() {
        if (m_firstTime) {
            m_sendEvent = Simulator::Schedule(Seconds(0.0), &CustomConsumerBoot::SendInterest, this);
            m_firstTime = false;
        } else if (!m_sendEvent.IsRunning()) {
            m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                              : Seconds(m_random->GetValue()),
                                              &CustomConsumerBoot::SendInterest, this);
        }
    }

    void CustomConsumerBoot::SendInterest() {
        /////////////////////////////////////
        // Sending one Interest packet out //
        /////////////////////////////////////

        // Create and configure ndn::Interest
        auto interest                   = std::make_shared<ndn::Interest>(m_interestName);
        Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
        interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
        interest->setInterestLifetime(ndn::time::milliseconds(m_interestLifeTime.GetMilliSeconds()));
        interest->setCanBePrefix(false);

        NS_LOG_DEBUG("Sending Interest packet: " << *interest);

        // Call trace (for logging purposes) and send interest
        m_transmittedInterests(interest, this, m_face);
        m_appLink->onReceiveInterest(*interest);

        // Schedule send of next interests
        ScheduleNextPacket();
    }

    // Callback that will be called when Data arrives
    void CustomConsumerBoot::OnData(std::shared_ptr<const ndn::Data> data) {
        NS_LOG_DEBUG("Receiving Data packet: " << data->getName());
        m_validator->validate(*data, MakeCallback(&CustomConsumerBoot::OnDataValidated, this), MakeCallback(&CustomConsumerBoot::OnDataValidationFailed, this));
        // std::cout << "DATA received for name " << data->getName() << std::endl;
    }

    void CustomConsumerBoot::OnDataValidated(const ndn::Data &data) {
        NS_LOG_INFO("Valid Data packet: " << data.getName());
    }

    void CustomConsumerBoot::OnDataValidationFailed(const ndn::Data &data, const ::ndn::security::v2::ValidationError &error) {
        NS_LOG_ERROR("Validation failed for Data packet: " << data.getName() << " - Error: " << error);
    }

    void CustomConsumerBoot::SetRandomize(const std::string &value) {
        if (value == "uniform") {
            m_random = CreateObject<UniformRandomVariable>();
            m_random->SetAttribute("Min", DoubleValue(0.0));
            m_random->SetAttribute("Max", DoubleValue(2 * 1.0 / m_frequency));
        } else if (value == "exponential") {
            m_random = CreateObject<ExponentialRandomVariable>();
            m_random->SetAttribute("Mean", DoubleValue(1.0 / m_frequency));
            m_random->SetAttribute("Bound", DoubleValue(50 * 1.0 / m_frequency));
        } else
            m_random = 0;

        m_randomType = value;
    }

    std::string CustomConsumerBoot::GetRandomize() const {
        return m_randomType;
    }

}  // namespace ns3