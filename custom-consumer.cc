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

  NS_OBJECT_ENSURE_REGISTERED(CustomConsumer);

  //////////////////////
  //     PUBLIC
  //////////////////////

  // register NS-3 type
  TypeId CustomConsumer::GetTypeId() {
    static TypeId tid =
        TypeId("CustomConsumer")
            .SetParent<ndn::App>()
            .AddConstructor<CustomConsumer>()
            .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                          MakeStringAccessor(&CustomConsumer::m_prefix), MakeStringChecker())
            .AddAttribute("Frequency", "Frequency of interest packets", StringValue("1.0"),
                          MakeDoubleAccessor(&CustomConsumer::m_frequency), MakeDoubleChecker<double>())
            .AddAttribute("LifeTime", "Lifetime of interest packets (in seconds)", StringValue("2s"),
                          MakeTimeAccessor(&CustomConsumer::m_lifeTime), MakeTimeChecker())
            .AddAttribute("Randomize",
                          "Type of send time randomization: none (default), uniform, "
                          "exponential",
                          StringValue("none"),
                          MakeStringAccessor(&CustomConsumer::SetRandomize, &CustomConsumer::GetRandomize),
                          MakeStringChecker())
            .AddAttribute("SchemaPrefix", "Trust Schema prefix", StringValue("/SCHEMA"),
                          MakeStringAccessor(&CustomConsumer::m_schemaPrefix), MakeStringChecker())
            .AddAttribute("ValidatorConf", "Validator config filename",
                          StringValue("./scratch/sim_bootsec/config/validator.conf"),
                          MakeStringAccessor(&CustomConsumer::m_validatorConf), MakeStringChecker());
    return tid;
  }

  CustomConsumer::CustomConsumer() : m_face_NDN_CXX(0) {}
  CustomConsumer::~CustomConsumer() {}

  // Processing upon start of the application
  void CustomConsumer::StartApplication() {
    NS_LOG_FUNCTION_NOARGS();
    ndn::App::StartApplication();

    // create ndn::Face to allow real-world application to interact inside ns3
    m_face_NDN_CXX = std::make_shared<::ndn::Face>();

    // setup validatorConf
    m_validator = std::make_shared<::ndn::security::ValidatorConfig>(
        std::make_unique<::ndn::security::v2::CertificateFetcherFromNetwork>(*m_face_NDN_CXX));
    m_validator->load(m_validatorConf);

    // Schedule send of first interest
    ScheduleSubscribeSchema();
    ScheduleInterestContent();
  }

  // Processing when application is stopped
  void CustomConsumer::StopApplication() {
    // cancel periodic packet generation
    for(const auto &pairSendEvent : m_sendEvents) {
      Simulator::Cancel(pairSendEvent.second);
    }

    // cleanup ndn::App
    ndn::App::StopApplication();
  }

  // Callback that will be called when Data arrives
  void CustomConsumer::OnData(std::shared_ptr<const ndn::Data> data) {
    NS_LOG_DEBUG("Receiving Data packet: " << data->getName()
                                           << " - KeyLocator: " << data->getSignature().getKeyLocator());
    m_validator->validate(*data, MakeCallback(&CustomConsumer::OnDataValidated, this),
                          MakeCallback(&CustomConsumer::OnDataValidationFailed, this));
  }

  void CustomConsumer::OnDataValidated(const ndn::Data &data) {
    NS_LOG_INFO("Validation OK for Data packet: " << data.getName());
    auto schemaProtocol = data.getName().getSubName(-2);
    if(schemaProtocol.isPrefixOf("/SCHEMA/content")) {
      NS_LOG_INFO("Updating trust schema ... ");
      auto buffer = data.getContent().getBuffer();
      auto dataStr = std::string(buffer->begin(), buffer->end());
      m_validator->load(dataStr, m_validatorConf);
    } else if(schemaProtocol.isPrefixOf("/SCHEMA/subscribe")) {
      NS_LOG_INFO("Trust schema has changed - Sending schema request ... ");
      SendInterestSchema();
    }
  }

  void CustomConsumer::OnDataValidationFailed(const ndn::Data &data,
                                              const ::ndn::security::v2::ValidationError &error) {
    NS_LOG_ERROR("Validation FAILED for Data packet: " << data.getName() << " - Error: " << error);
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

  void CustomConsumer::ScheduleSubscribeSchema(ns3::Time delay) {
    Simulator::Schedule(delay, &CustomConsumer::SendSubscribeSchema, this);
  }

  void CustomConsumer::ScheduleInterestContent() {
    if(m_sendEvents.find("content") == m_sendEvents.end()) {
      m_sendEvents["content"] = Simulator::Schedule(Seconds(0.0), &CustomConsumer::SendInterestContent, this);
    } else if(!m_sendEvents["content"].IsRunning()) {
      m_sendEvents["content"] =
          Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency) : Seconds(m_random->GetValue()),
                              &CustomConsumer::SendInterestContent, this);
    }
  }

  void CustomConsumer::SendSubscribeSchema() {
    SendInterest(m_schemaPrefix + "/subscribe", m_lifeTime);
    ScheduleSubscribeSchema(m_lifeTime - ns3::Time("10ms"));
  }
  void CustomConsumer::SendInterestSchema() { SendInterest(m_schemaPrefix + "/content", m_lifeTime); }

  void CustomConsumer::SendInterestContent() {
    SendInterest(m_prefix, m_lifeTime);
    ScheduleInterestContent();
  }

  void CustomConsumer::SendInterest(std::string name, ns3::Time lifeTime) {
    // Create and configure ndn::Interest
    auto interest = std::make_shared<ndn::Interest>(name);
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    interest->setInterestLifetime(ndn::time::milliseconds(lifeTime.GetMilliSeconds()));
    interest->setCanBePrefix(false);

    // Call trace (for logging purposes), send interest, schedule next interests
    NS_LOG_DEBUG("Sending Interest packet: " << *interest);
    m_transmittedInterests(interest, this, m_face);
    m_appLink->onReceiveInterest(*interest);
  }

  /// @brief must be run after ndn::App.StartApplication()
  void CustomConsumer::ReadValidatorConfRules() {
    // try {
    //   m_validator->load(m_validatorFilename);
    // } catch(const std::exception &e) {
    //   throw std::runtime_error("Failed to load validation rules file=" + m_validatorFilename +
    //                            " Error=" + e.what());
    // }
  }

} // namespace ns3