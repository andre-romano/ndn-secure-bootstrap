// custom-producer.cpp

#include "custom-producer-boot.hpp"

#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

// #include "ns3/boolean.h"
// #include "ns3/callback.h"
// #include "ns3/double.h"
// #include "ns3/integer.h"
// #include "ns3/string.h"
// #include "ns3/uinteger.h"
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"

#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"

#include <limits>
#include <memory>
#include <string>

// namespace ns3 {
//     ATTRIBUTE_HELPER_CPP(IntMetricSet);

//     std::ostream &operator<<(std::ostream &os, const IntMetricSet &intMetrics) {
//         for (auto item : intMetrics) {
//             os << item;
//         }
//         return os;
//     }
//     std::istream &operator>>(std::istream &is, IntMetricSet &intMetrics) {
//         std::string temp;
//         char data;
//         while (is >> data) {
//             if (data != ':') {
//                 temp.push_back(data);
//             } else {
//                 auto intMetric = IntMetric::get(temp);
//                 if (intMetric.isValid()) {
//                     intMetrics.insert(intMetric);
//                 }
//                 temp.clear();
//             }
//         }
//         return is;
//     }

//     ATTRIBUTE_HELPER_CPP(IntFwdMap);

//     std::ostream &operator<<(std::ostream &os, const IntFwdMap &obj) {
//         os << "[";
//         for (auto item : obj) {
//             auto fwdProp = item.second;
//             os << "("
//                << fwdProp.getMetric() << ","
//                << fwdProp.getAction() << ","
//                << fwdProp.getCriteria()
//                << "),";
//         }
//         os << "]";
//         return os;
//     }
//     std::istream &operator>>(std::istream &is, IntFwdMap &obj) {
//         std::string temp;
//         IntMetric metric;
//         IntFwdAction action;
//         IntFwdCriteria criteria;
//         char data;
//         while (is >> data) {
//             if (data == '[' || data == ']')
//                 continue;
//             else if (data != '(' && data != ',' && data != ')') {
//                 temp.push_back(data);
//             } else if (!metric.isValid()) {
//                 metric = IntMetric::get(temp);
//                 temp.clear();
//             } else if (!action.isValid()) {
//                 action = IntFwdAction::getTypeStatic(temp);
//                 temp.clear();
//             } else {
//                 criteria = IntFwdCriteria::getTypeStatic(temp);
//                 temp.clear();
//                 obj[metric] = IntFwdProperty(metric, action, criteria);
//                 metric      = IntMetric();
//                 action      = IntFwdAction();
//                 criteria    = IntFwdCriteria();
//             }
//         }
//         return is;
//     }

// }  // namespace ns3

NS_LOG_COMPONENT_DEFINE("CustomProducerBoot");

namespace ns3 {
    namespace ndn {

        // Necessary if you are planning to use ndn::AppHelper
        NS_OBJECT_ENSURE_REGISTERED(CustomProducerBoot);

        TypeId
        CustomProducerBoot::GetTypeId() {
            static TypeId tid = TypeId("CustomProducerBoot")
                                    .SetParent<ndn::App>()
                                    .AddConstructor<CustomProducerBoot>()
                                    .AddAttribute("Prefix",
                                                  "Name of the Interest",
                                                  StringValue("/"),
                                                  MakeNameAccessor(&CustomProducerBoot::m_prefix),
                                                  MakeNameChecker())
                                    .AddAttribute(
                                        "Postfix",
                                        "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                                        StringValue("/"),
                                        MakeNameAccessor(&CustomProducerBoot::m_postfix),
                                        MakeNameChecker())
                                    .AddAttribute("PayloadSize",
                                                  "Virtual payload size for Content packets",
                                                  UintegerValue(1024),
                                                  MakeUintegerAccessor(&CustomProducerBoot::m_virtualPayloadSize),
                                                  MakeUintegerChecker<uint32_t>())
                                    .AddAttribute("Freshness",
                                                  "Freshness of data packets, if 0, then unlimited freshness",
                                                  TimeValue(Seconds(0)),
                                                  MakeTimeAccessor(&CustomProducerBoot::m_freshness),
                                                  MakeTimeChecker());
            // .AddAttribute("IntMetrics",
            //               "Set of INT metrics to collect",
            //               IntMetricSetValue(),
            //               MakeIntMetricSetAccessor(&CustomProducerBoot::m_collectMetrics),
            //               MakeIntMetricSetChecker())
            // .AddAttribute("IntFwd",
            //               "INT Fwd commands",
            //               IntFwdMapValue(),
            //               MakeIntFwdMapAccessor(&CustomProducerBoot::m_fwdMap),
            //               MakeIntFwdMapChecker());
            return tid;
        }

        CustomProducerBoot::CustomProducerBoot() {}

        void
        CustomProducerBoot::StartApplication() {
            NS_LOG_FUNCTION_NOARGS();
            App::StartApplication();

            // equivalent to setting interest filter for "/prefix" prefix
            ndn::FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
        }

        void
        CustomProducerBoot::StopApplication() {
            NS_LOG_FUNCTION_NOARGS();
            App::StopApplication();
        }

        void
        CustomProducerBoot::OnInterest(std::shared_ptr<const ndn::Interest> interest) {
            ndn::App::OnInterest(interest);  // forward call to perform app-level tracing

            // Note that Interests send out by the app will not be sent back to the app !

            auto node = ::ns3::NodeList::GetNode(::ns3::Simulator::GetContext());
            Name dataName(interest->getName());
            // dataName.append(m_postfix);
            // dataName.appendVersion();

            auto data = make_shared<Data>();
            data->setName(dataName);
            data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

            data->setContent(make_shared<::ndn::Buffer>(m_virtualPayloadSize));
            ndn::StackHelper::getKeyChain().sign(*data);

            NS_LOG_INFO(data->getName());

            // to create real wire encoding
            data->wireEncode();

            // Call trace (for logging purposes)
            m_transmittedDatas(data, this, m_face);
            m_appLink->onReceiveData(*data);
        }

    }  // namespace ndn
}  // namespace ns3