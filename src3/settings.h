#pragma once

#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"

class settings
{
public:
	static CefSettings cef();
	static CefBrowserSettings browser_cef();
	static CefWindowInfo* window_info();
};

