/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsError.h"

#include "mozilla/UniquePtr.h"

#include "signaling/src/sdp/Sdp.h"
#include "signaling/src/sdp/SdpEnum.h"
#include "signaling/src/sdp/RsdparsaSdp.h"
#include "signaling/src/sdp/RsdparsaSdpParser.h"
#include "signaling/src/sdp/RsdparsaSdpInc.h"
#include "signaling/src/sdp/RsdparsaSdpGlue.h"

namespace mozilla {

static const std::string& WEBRTC_SDP_NAME = "WEBRTCSDP";

const std::string& RsdparsaSdpParser::Name() const { return WEBRTC_SDP_NAME; }

UniquePtr<SdpParser::Results> RsdparsaSdpParser::Parse(
    const std::string& aText) {
  UniquePtr<SdpParser::InternalResults> results(
      new SdpParser::InternalResults(Name()));
  RustSdpSession* result = nullptr;
  RustSdpError* err = nullptr;
  StringView sdpTextView{aText.c_str(), aText.length()};
  nsresult rv = parse_sdp(sdpTextView, false, &result, &err);
  if (rv != NS_OK) {
    size_t line = sdp_get_error_line_num(err);
    std::string errMsg = convertStringView(sdp_get_error_message(err));
    sdp_free_error(err);
    results->AddParseError(line, errMsg);
    return results;
  }

  if (err) {
    size_t line = sdp_get_error_line_num(err);
    std::string warningMsg = convertStringView(sdp_get_error_message(err));
    sdp_free_error(err);
    results->AddParseWarning(line, warningMsg);
  }

  RsdparsaSessionHandle uniqueResult(result);
  RustSdpOrigin rustOrigin = sdp_get_origin(uniqueResult.get());
  auto address = convertExplicitlyTypedAddress(&rustOrigin.addr);
  SdpOrigin origin(convertStringView(rustOrigin.username), rustOrigin.sessionId,
                   rustOrigin.sessionVersion, address.first, address.second);

  results->SetSdp(MakeUnique<RsdparsaSdp>(std::move(uniqueResult), origin));
  return results;
}

bool RsdparsaSdpParser::IsNamed(const std::string& aName) {
  return aName == WEBRTC_SDP_NAME;
}

}  // namespace mozilla
