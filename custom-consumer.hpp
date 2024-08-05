// custom-app.hpp

#ifndef CUSTOM_CONSUMER_BOOT_H_
#define CUSTOM_CONSUMER_BOOT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/random-variable-stream.h"

#include "ns3/ndnSIM/ndn-cxx/face.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/key-chain.hpp"
#include "ns3/ndnSIM/ndn-cxx/security/validator-config.hpp"

namespace ns3 {

  class CustomConsumer : public ndn::App {
  public:
    CustomConsumer();
    ~CustomConsumer();

    // register NS-3 type "CustomConsumer"
    static TypeId GetTypeId();

    virtual void StartApplication() override;
    virtual void StopApplication() override;

    virtual void OnData(std::shared_ptr<const ndn::Data> data) override;

    // data validation callbacks
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
    void ScheduleSubscribeSchema(ns3::Time delay = ns3::Time("0s"));
    void ScheduleInterestContent();

    void SendSubscribeSchema();
    void SendInterestSchema();
    void SendInterestContent();

    void SendInterest(std::string name, ns3::Time lifeTime);

    void ReadValidatorConfRules();

  private:
    std::string m_prefix;
    double m_frequency;
    ::ns3::Time m_lifeTime;
    std::string m_randomType;

    std::string m_schemaPrefix;
    std::string m_validatorConf;

    std::shared_ptr<::ndn::Face> m_face_NDN_CXX; ///< @brief ndn::Face to allow real-world
                                                 ///< applications to work inside ns3

    std::map<std::string, ::ns3::EventId> m_sendEvents; ///< @brief pending "send packet" event

    std::shared_ptr<::ndn::ValidatorConfig> m_validator; ///< @brief validates data packets
    ::ns3::Ptr<::ns3::RandomVariableStream> m_random;    ///< @brief Random generator for packet send
  };

} // namespace ns3

#endif // CUSTOM_CONSUMER_BOOT_H_