// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "src/common/scheme_test_common.h"

#include "include/cef_scheme.h"

namespace client {
namespace scheme_test {

void RegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar,
                           std::vector<CefString>& cookiable_schemes) {
  registrar->AddCustomScheme(
      "client", CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_CORS_ENABLED);
}

}  // namespace scheme_test
}  // namespace client
