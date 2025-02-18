// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "src/common/scheme_test_common.h"
#include "shared/common/client_app.h"

namespace client {

// static
void ClientApp::RegisterCustomSchemes(
    CefRawPtr<CefSchemeRegistrar> registrar,
    std::vector<CefString>& cookiable_schemes) {
  scheme_test::RegisterCustomSchemes(registrar, cookiable_schemes);
}

}  // namespace client
