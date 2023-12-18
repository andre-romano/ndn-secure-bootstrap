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

#ifndef NDN_SECURITY_V2_CERTIFICATE_FETCHER_FROM_NS3_HPP
#define NDN_SECURITY_V2_CERTIFICATE_FETCHER_FROM_NS3_HPP

#include "ns3/simulator.h"

#include "ns3/ndnSIM/ndn-cxx/security/v2/certificate-fetcher.hpp"
#include "ns3/ndnSIM/ndn-cxx/util/scheduler.hpp"

#include "ns3/ndnSIM/NFD/daemon/face/face.hpp"
#include "ns3/ndnSIM/model/ndn-app-link-service.hpp"

#include <map>

namespace ndn {

    namespace lp {
        class Nack;
    }  // namespace lp

    namespace security {
        namespace v2 {

            /**
             * @brief Fetch missing keys from the network
             */
            class CertificateFetcherFromNS3 : public CertificateFetcher {
              public:
                CertificateFetcherFromNS3(::ns3::ndn::AppLinkService *appLink);

              protected:
                void doFetch(const shared_ptr<CertificateRequest> &certRequest, const shared_ptr<ValidationState> &state,
                             const ValidationContinuation &continueValidation) override;

                /**
                 * @brief Callback invoked when certificate is retrieved.
                 */
                void
                dataCallback(const Data &data,
                             const shared_ptr<CertificateRequest> &certRequest, const shared_ptr<ValidationState> &state,
                             const ValidationContinuation &continueValidation);

                /**
                 * @brief Callback invoked when interest for fetching certificate gets NACKed.
                 *
                 * Retries with exponential backoff while `certRequest->nRetriesLeft > 0`
                 */
                void
                nackCallback(const lp::Nack &nack,
                             const shared_ptr<CertificateRequest> &certRequest, const shared_ptr<ValidationState> &state,
                             const ValidationContinuation &continueValidation);

                /**
                 * @brief Callback invoked when interest for fetching certificate times out.
                 *
                 * It will retry if `certRequest->nRetriesLeft > 0`
                 */
                void
                timeoutCallback(const shared_ptr<CertificateRequest> &certRequest, const shared_ptr<ValidationState> &state,
                                const ValidationContinuation &continueValidation);

              private:
                void cancelEvent(std::string name);

              protected:
                std::shared_ptr<::ns3::ndn::AppLinkService> m_appLink;
                std::map<std::string, ::ns3::EventId> m_sendEvents;
            };

        }  // namespace v2
    }      // namespace security
}  // namespace ndn

#endif  // NDN_SECURITY_V2_CERTIFICATE_FETCHER_FROM_NS3_HPP
