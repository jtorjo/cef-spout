#include "settings.h"


CefSettings settings::cef() {
  CefSettings sett;
  //sett.windowless_rendering_enabled = true;
  sett.multi_threaded_message_loop = false;
  sett.no_sandbox = true;
  return sett;
}

CefBrowserSettings settings::browser_cef() {
  CefBrowserSettings browser_settings ;
  browser_settings.windowless_frame_rate = 60;
  return browser_settings;
}

CefWindowInfo* settings::window_info() {
    // Information used when creating the native window.
    CefWindowInfo *wi = new CefWindowInfo;
	//wi->windowless_rendering_enabled = true;
	
	if (wi->windowless_rendering_enabled) {
		wi->shared_texture_enabled = false;
		wi->external_begin_frame_enabled = true;
		wi->SetAsWindowless(nullptr);
	}
	else {
		wi->SetAsPopup(nullptr, "cefsimple");
	}
	return wi;
}
