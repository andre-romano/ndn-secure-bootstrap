
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "certificate-fetcher-from-ns3.hpp"

#include "ns3/ndnSIM/ndn-cxx/util/logger.hpp"

namespace ndn {
    namespace security {
        namespace v2 {

            NDN_LOG_INIT(ndn.security.v2.CertificateFetcher.FromNS3);

#define NDN_LOG_DEBUG_DEPTH(x) NDN_LOG_DEBUG(std::string(state->getDepth() + 1, '>') << " " << x)
#define NDN_LOG_TRACE_DEPTH(x) NDN_LOG_TRACE(std::string(state->getDepth() + 1, '>') << " " << x)

            CertificateFetcherFromNS3::CertificateFetcherFromNS3(::ns3::ndn::AppLinkService *appLink)
                : m_appLink(appLink) {}

            void CertificateFetcherFromNS3::doFetch(const shared_ptr<CertificateRequest> &certRequest,
                                                    const shared_ptr<ValidationState> &state,
                                                    const ValidationContinuation &continueValidation) {
                NDN_LOG_DEBUG("Send Interest for " << certRequest->interest.getName());

                m_appLink->onReceiveInterest(certRequest->interest);
                // m_appLink->sendInterest(certRequest->interest, endpoint);
                // m_appLink->getLinkService()->afterReceiveInterest(certRequest->interest, endpoint);

                NDN_LOG_DEBUG("ard ");
                m_appLink->afterReceiveData.connect([=](const Data &data, const ::nfd::face::EndpointId &endpoint) {
                    dataCallback(data, certRequest, state, continueValidation);
                });

                NDN_LOG_DEBUG("arn ");
                m_appLink->afterReceiveNack.connect([=](const ::ndn::lp::Nack &nack, const ::nfd::face::EndpointId &endpoint) {
                    nackCallback(nack, certRequest, state, continueValidation);
                });

                auto interestName = certRequest->interest.getName().toUri();
                cancelEvent(interestName);
                NDN_LOG_DEBUG("ca ");
                m_sendEvents[interestName] = ns3::Simulator::Schedule(ns3::MilliSeconds(certRequest->interest.getInterestLifetime().count()),
                                                                      &CertificateFetcherFromNS3::timeoutCallback, this, certRequest, state, continueValidation);
                NDN_LOG_DEBUG("sched ");
            }

            void CertificateFetcherFromNS3::dataCallback(const Data &data,
                                                         const shared_ptr<CertificateRequest> &certRequest,
                                                         const shared_ptr<ValidationState> &state,
                                                         const ValidationContinuation &continueValidation) {
                NDN_LOG_DEBUG("Fetched certificate from NS3 network " << data.getName());
                cancelEvent(data.getName().toUri());

                Certificate cert;
                try {
                    cert = Certificate(data);
                } catch (const tlv::Error &e) {
                    return state->fail({ValidationError::Code::MALFORMED_CERT, "Fetched a malformed certificate "
                                                                               "`" +
                                                                                   data.getName().toUri() + "` (" + e.what() + ")"});
                }
                continueValidation(cert, state);
            }

            void CertificateFetcherFromNS3::nackCallback(const lp::Nack &nack,
                                                         const shared_ptr<CertificateRequest> &certRequest,
                                                         const shared_ptr<ValidationState> &state,
                                                         const ValidationContinuation &continueValidation) {
                NDN_LOG_DEBUG("NACK (" << nack.getReason() << ") while fetching certificate "
                                       << certRequest->interest.getName());
                auto interestName = nack.getInterest().getName().toUri();
                cancelEvent(interestName);

                --certRequest->nRetriesLeft;
                if (certRequest->nRetriesLeft >= 0) {
                    m_sendEvents[interestName] = ns3::Simulator::Schedule(ns3::MilliSeconds(certRequest->waitAfterNack.count()),
                                                                          &CertificateFetcherFromNS3::fetch, this, certRequest, state, continueValidation);
                    certRequest->waitAfterNack *= 2;
                } else {
                    state->fail({ValidationError::Code::CANNOT_RETRIEVE_CERT, "Cannot fetch certificate after all "
                                                                              "retries `" +
                                                                                  certRequest->interest.getName().toUri() + "`"});
                }
            }

            void CertificateFetcherFromNS3::timeoutCallback(const shared_ptr<CertificateRequest> &certRequest,
                                                            const shared_ptr<ValidationState> &state,
                                                            const ValidationContinuation &continueValidation) {
                NDN_LOG_DEBUG("Timeout while fetching certificate " << certRequest->interest.getName()
                                                                    << ", retrying");
                // cancelEvent(certRequest->interest.getName().toUri());

                --certRequest->nRetriesLeft;
                if (certRequest->nRetriesLeft >= 0) {
                    fetch(certRequest, state, continueValidation);
                } else {
                    state->fail({ValidationError::Code::CANNOT_RETRIEVE_CERT, "Cannot fetch certificate after all "
                                                                              "retries `" +
                                                                                  certRequest->interest.getName().toUri() + "`"});
                }
            }

            void CertificateFetcherFromNS3::cancelEvent(std::string name) {
                NDN_LOG_DEBUG("Cancel event for " << name);
                auto it = m_sendEvents.find(name);
                if (it != m_sendEvents.end()) {
                    it->second.Cancel();
                    m_sendEvents.erase(it);
                }
            }

        }  // namespace v2
    }      // namespace security
}  // namespace ndn
