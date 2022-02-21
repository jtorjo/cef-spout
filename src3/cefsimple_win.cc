
// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "platform.h"
#include "resource.h"

#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "simple_app.h"
#include "simple_handler.h"
#include "settings.h"

using namespace std;

class ComInitializer
{
public:
	ComInitializer() {
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	}
	~ComInitializer() { CoUninitialize(); }
};

class WebApp : public CefApp, public CefBrowserProcessHandler, public CefRenderProcessHandler
{
public:
	WebApp() {}

	CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
	CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }

	void OnBeforeCommandLineProcessing(
		CefString const& /*process_type*/,
		CefRefPtr<CefCommandLine> command_line) override
	{
		// disable creation of a GPUCache/ folder on disk
		command_line->AppendSwitch("disable-gpu-shader-disk-cache");
		//command_line->AppendSwitch("disable-accelerated-video-decode");
		command_line->AppendSwitch("show-fps-counter");		
		//command_line->AppendSwitch("disable-gpu-vsync");
		command_line->AppendSwitchWithValue("use-angle", "d3d11");
		command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
	}

	virtual void OnContextInitialized() override {
	}
	void OnContextCreated(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefV8Context> context) override
	{}
	void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override
	{}
	bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
		CefProcessId /*source_process*/,
		CefRefPtr<CefProcessMessage> message)
	{
		return false;
	}

private:
	IMPLEMENT_REFCOUNTING(WebApp);
};

condition_variable signal_;
atomic_bool ready_;
mutex lock_;
HINSTANCE instance_;
shared_ptr<thread> thread_;

void message_loop();

void log_message(const char* msg, ...)
{
	// old-school, printf style logging
	if (msg) 
	{
		char buff[512];
		va_list args;
		va_start(args, msg);
		vsprintf(buff, msg, args);
		OutputDebugStringA(buff);
	}
}

void startup() {
	thread_ = make_shared<thread>(&message_loop);
	while (!ready_.load()) 
		::Sleep(50);

	log_message("cef module is ready\n");
}

void message_loop()
{
	log_message("cef initializing ... \n");

    CefSettings sett = settings::cef() ;

	CefRefPtr<WebApp> app(new WebApp());

	CefMainArgs main_args(instance_);
	CefInitialize(main_args, sett, app, nullptr);

	log_message("cef is initialized.\n");

	// signal cef is initialized and ready
	ready_ = true;
	signal_.notify_one();

	CefRunMessageLoop();

	log_message("cef shutting down ... \n");

	CefShutdown();
	log_message("cef is shutdown\n");
}

HWND create_hwnd() {
	LPCWSTR class_name = L"_main_window_";

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	if (!GetClassInfoEx(instance_, class_name, &wcex))
	{
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = DefWindowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = instance_;
		wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOWTEXT + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = class_name;
		wcex.hIconSm = nullptr;
		if (!RegisterClassExW(&wcex)) {
			return nullptr;
		}
	}
	auto const hwnd = CreateWindow(class_name,
		L"test",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		nullptr,
		nullptr,
		instance_,
		nullptr);
	ShowWindow(hwnd, SW_SHOWNORMAL);
	return hwnd;
}


// Entry point function for all processes.
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  instance_ = hInstance;

  // Enable High-DPI support on Windows 7 or newer.
  CefEnableHighDPISupport();

  // Provide CEF with command-line arguments.
  CefMainArgs main_args(hInstance);

  // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
  // that share the same executable. This function checks the command-line and,
  // if this is a sub-process, executes the appropriate logic.
  CefRefPtr<WebApp> exe_process_app(new WebApp());
  int exit_code = CefExecuteProcess(main_args, exe_process_app, nullptr);
  if (exit_code >= 0) {
    // The sub-process has completed so return here.
    return exit_code;
  }

  startup();

	ComInitializer com_init;

	// SimpleHandler implements browser-level callbacks.
	CefRefPtr<SimpleHandler> handler(new SimpleHandler(false));

	// Specify CEF browser settings here.
	CefBrowserSettings browser_settings = settings::browser_cef();

	std::string url;
	url = "http://www.google.com";
	url = "http://html5test.com";

	// Information used when creating the native window.
	CefWindowInfo *window_info = settings::window_info();
	CefBrowserHost::CreateBrowser(*window_info, handler, url, browser_settings, nullptr, nullptr);

	HACCEL accel_table = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_APPLICATION));

	HWND h = create_hwnd();
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(msg.hwnd, accel_table, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			// render
			auto browser = SimpleHandler::GetInstance()->first_browser();
			if(browser)
				browser->GetHost()->SendExternalBeginFrame();
		}
	}

  return 0;
}
