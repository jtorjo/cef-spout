
#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>
#include <include/cef_version.h>

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <sstream>
#include <iomanip>
#include <functional>
#include <map>
#include <vector>
#include <fstream>
#include <algorithm>

#include "util.h"
#include <chrono>

#include "SpoutDX.h"
#include "composition.h"

using namespace std;

//
// helper function to convert a 
// CefDictionaryValue to a CefV8Object
//
CefRefPtr<CefV8Value> to_v8object(CefRefPtr<CefDictionaryValue> const& dictionary)
{
	auto const obj = CefV8Value::CreateObject(nullptr, nullptr);
	if (dictionary)
	{
		auto const attrib = V8_PROPERTY_ATTRIBUTE_READONLY;
		CefDictionaryValue::KeyList keys;
		dictionary->GetKeys(keys);
		for (auto const& k : keys)
		{
			auto const type = dictionary->GetType(k);
			switch (type)
			{
			case VTYPE_BOOL: obj->SetValue(k,
				CefV8Value::CreateBool(dictionary->GetBool(k)), attrib);
				break;
			case VTYPE_INT: obj->SetValue(k,
				CefV8Value::CreateInt(dictionary->GetInt(k)), attrib);
				break;
			case VTYPE_DOUBLE: obj->SetValue(k,
				CefV8Value::CreateDouble(dictionary->GetDouble(k)), attrib);
				break;
			case VTYPE_STRING: obj->SetValue(k,
				CefV8Value::CreateString(dictionary->GetString(k)), attrib);
				break;

			default: break;
			}
		}
	}
	return obj;
}

class WebView;
class FrameBuffer;

extern bool show_devtools_;


const char* globName = "CEF";
int fbCount = 0;


vector<ID3D11Texture2D *> activeTextures;
vector<HANDLE> activeHandles;
vector<string> activeNames;
int numActiveSenders;

class DevToolsClient : public CefClient
{
private:
	IMPLEMENT_REFCOUNTING(DevToolsClient);
};


//
// internal factory method so popups (window.open()) 
// can create layers on the fly
//
shared_ptr<Layer> create_web_layer(
	std::shared_ptr<Composition> const& device,
	bool want_input,
	CefRefPtr<WebView> const& view);

//
// internal factory method so popups (dropdowns, PET_POPUP) 
// can create simple layers on the fly
//
shared_ptr<Layer> create_popup_layer(
	shared_ptr<d3d11::Device> const& device,
	shared_ptr<FrameBuffer> const& buffer);

//
// V8 handler for our 'mixer' object available to javascript
// running in a page within this application
//
class MixerHandler :
	public CefV8Accessor
{
public:
	MixerHandler(
		CefRefPtr<CefBrowser> const& browser,
		CefRefPtr<CefV8Context> const& context)
		: browser_(browser)
		, context_(context)
	{
		auto window = context->GetGlobal();
		auto const obj = CefV8Value::CreateObject(this, nullptr);
		obj->SetValue("requestStats", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
		window->SetValue("mixer", obj, V8_PROPERTY_ATTRIBUTE_NONE);
	}

	void update(CefRefPtr<CefDictionaryValue> const& dictionary)
	{
		if (!request_stats_) {
			return;
		}

		context_->Enter();
		CefV8ValueList values;
		values.push_back(to_v8object(dictionary));
		request_stats_->ExecuteFunction(request_stats_, values);
		context_->Exit();
	}

	bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& /*exception*/) override {

		if (name == "requestStats" && request_stats_ != nullptr) {
			retval = request_stats_;
			return true;
		}

		// Value does not exist.
		return false;
	}

	bool Set(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		const CefRefPtr<CefV8Value> value,
		CefString& /*exception*/) override
	{
		if (name == "requestStats") {
			request_stats_ = value;

			// notify the browser process that we want stats
			auto message = CefProcessMessage::Create("mixer-request-stats");
			if (message != nullptr && browser_ != nullptr) {
				//FIXME browser_->SendProcessMessage(PID_BROWSER, message);
			}
			return true;
		}
		return false;
	}

	IMPLEMENT_REFCOUNTING(MixerHandler);

private:

	CefRefPtr<CefBrowser> const browser_;
	CefRefPtr<CefV8Context> const context_;
	CefRefPtr<CefV8Value> request_stats_;
};


class WebApp : public CefApp,
	public CefBrowserProcessHandler,
	public CefRenderProcessHandler
{
public:
	WebApp() {
	}

	CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
		return this;
	}

	CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
		return this;
	}

	void OnBeforeCommandLineProcessing(
		CefString const& /*process_type*/,
		CefRefPtr<CefCommandLine> command_line) override
	{
		// disable creation of a GPUCache/ folder on disk
		command_line->AppendSwitch("disable-gpu-shader-disk-cache");

		//command_line->AppendSwitch("disable-accelerated-video-decode");

		// un-comment to show the built-in Chromium fps meter
		command_line->AppendSwitch("show-fps-counter");		

		//command_line->AppendSwitch("disable-gpu-vsync");

		// Most systems would not need to use this switch - but on older hardware, 
		// Chromium may still choose to disable D3D11 for gpu workarounds.
		// Accelerated OSR will not at all with D3D11 disabled, so we force it on.
		//
		// See the discussion on this issue:
		// https://github.com/daktronics/cef-mixer/issues/10
		//
		command_line->AppendSwitchWithValue("use-angle", "d3d11");

		// tell Chromium to autoplay <video> elements without 
		// requiring the muted attribute or user interaction
		command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");

#if !defined(NDEBUG)
		// ~RenderProcessHostImpl() complains about DCHECK(is_self_deleted_)
		// when we run single process mode ... I haven't figured out how to resolve yet
		//command_line->AppendSwitch("single-process");
#endif
	}

	virtual void OnContextInitialized() override {
	}

	//
	// CefRenderProcessHandler::OnContextCreated
	//
	// Adds our custom 'mixer' object to the javascript context running
	// in the render process
	//
	void OnContextCreated(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefV8Context> context) override
	{
		mixer_handler_ = new MixerHandler(browser, context);
	}

	//
	// CefRenderProcessHandler::OnBrowserDestroyed
	//
	void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override
	{
		mixer_handler_ = nullptr;
	}

	//
	// CefRenderProcessHandler::OnProcessMessageReceived
	//
	bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
		CefProcessId /*source_process*/,
		CefRefPtr<CefProcessMessage> message)
	{
		auto const name = message->GetName().ToString();
		if (name == "mixer-update-stats")
		{
			if (mixer_handler_ != nullptr)
			{
				// we expect just 1 param that is a dictionary of stat values
				auto const args = message->GetArgumentList();
				auto const size = args->GetSize();
				if (size > 0) {
					auto const dict = args->GetDictionary(0);
					if (dict && dict->GetSize() > 0) {
						mixer_handler_->update(dict);
					}
				}
			}
			return true;
		}
		return false;
	}

private:

	IMPLEMENT_REFCOUNTING(WebApp);

	CefRefPtr<MixerHandler> mixer_handler_;
};


class FrameBuffer
{
public:
	FrameBuffer(shared_ptr<Composition> const& composition) : composition_(composition), dirty_(false)
	{
		spoutID.append(to_string(fbCount));
		fbCount++;
	}

	string spoutID = "CEF_";

	int32_t width() {
		if (shared_buffer_) {
			return shared_buffer_->width();
		}
		return 0;
	}

	int32_t height() {
		if (shared_buffer_) {
			return shared_buffer_->height();
		}
		return 0;
	}

	void on_paint(const void* buffer, uint32_t width, uint32_t height)
	{
		uint32_t stride = width * 4;
		size_t cb = stride * height;

		if (!shared_buffer_ || (shared_buffer_->width() != width) || (shared_buffer_->height() != height))
		{
			shared_buffer_ = composition_->device()->create_texture(width, height, DXGI_FORMAT_B8G8R8A8_UNORM, nullptr, 0);
			sw_buffer_ = shared_ptr<uint8_t>((uint8_t*)malloc(cb), free);
		}

		if (sw_buffer_ && buffer)
		{
			// todo: support dirty rect(s)
			memcpy(sw_buffer_.get(), buffer, cb);
		}

		dirty_ = true;

		// quick test
		// 
		// i need to create shared texture
		//on_gpu_paint( shared_buffer_->share_handle());
	}

	//
	// called in response to Cef's OnAcceleratedPaint notification
	//
	void on_gpu_paint(void* shared_handle)
	{
		// Note: we're not handling keyed mutexes yet

		lock_guard<mutex> guard(lock_);

		// did the shared texture change?
		if (shared_buffer_)
		{
			if (shared_handle != shared_buffer_->share_handle()) {
				shared_buffer_.reset();
			}
		}

		// open the shared texture
		if (!shared_buffer_)
		{
			shared_buffer_ = composition_->device()->open_shared_texture((void*)shared_handle);
			if (!shared_buffer_) {
				log_message("could not open shared texture!");
			}
		}

		dirty_ = true;
		composition_->draw_spout(shared_handle);
	}

	//
	// this method returns what should be considered the front buffer
	// we're simply using the shared texture directly 
	//
	// ... this method could be expanded on to handle 
	// synchronization through a keyed mutex
	// 
	shared_ptr<d3d11::Texture2D> swap(shared_ptr<d3d11::Context> const& ctx)
	{
		lock_guard<mutex> guard(lock_);

		// using software buffer? just copy to texture
		if (sw_buffer_ && shared_buffer_ && dirty_)
		{
			d3d11::ScopedBinder<d3d11::Texture2D> binder(ctx, shared_buffer_);
			shared_buffer_->copy_from(
				sw_buffer_.get(),
				shared_buffer_->width() * 4,
				shared_buffer_->height());
		}

		dirty_ = false;
		return shared_buffer_;
	}

private:

	mutex lock_;
	atomic_bool abort_;
	shared_ptr<d3d11::Texture2D> shared_buffer_;
	std::shared_ptr<Composition> const composition_;
	shared_ptr<uint8_t> sw_buffer_;
	bool dirty_;
};

//
// Simple string visitor that will dump the contents
// to a file
//
class HtmlSourceWriter : public CefStringVisitor
{
public:
	HtmlSourceWriter(string const& filename) {
		fout_ = make_shared<ofstream>(filename);
	}

	void Visit(const CefString& string) override
	{
		if (fout_ && fout_->is_open())
		{
			auto const utf8 = string.ToString();
			fout_->write(utf8.c_str(), utf8.size());
		}
	}

private:
	IMPLEMENT_REFCOUNTING(HtmlSourceWriter);
	shared_ptr<ofstream> fout_;
};


class WebView : public CefClient,
	public CefRenderHandler,
	public CefLifeSpanHandler,
	public CefLoadHandler
{
public:
	WebView(
		string const& name,
		shared_ptr<Composition> const& composition,
		int width,
		int height,
		bool use_shared_textures,
		bool send_begin_Frame)
		: name_(name)
		, width_(width)
		, height_(height)
		, view_buffer_(make_shared<FrameBuffer>(composition))
		, popup_buffer_(make_shared<FrameBuffer>(composition))
		, needs_stats_update_(false)
		, use_shared_textures_(use_shared_textures)
		, send_begin_frame_(send_begin_Frame)
		, composition_(composition)
	{
		frame_ = 0;
		fps_start_ = 0ull;
		//globName = name.c_str();
	}

	~WebView() {
		close();
	}

	bool use_shared_textures() const {
		return use_shared_textures_;
	}

	void close()
	{
		// get thread-safe reference
		decltype(browser_) browser;
		{
			lock_guard<mutex> guard(lock_);
			browser = browser_;
			browser_ = nullptr;
		}

		if (browser.get()) {
			browser->GetHost()->CloseBrowser(true);
		}

		// FIXME close spout
		log_message("html view is closed\n");
	}

	CefRefPtr<CefRenderHandler> GetRenderHandler() override {
		return this;
	}

	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
		return this;
	}

	CefRefPtr<CefLoadHandler> GetLoadHandler() override {
		return this;
	}

	bool OnProcessMessageReceived(
		CefRefPtr<CefBrowser> ,
		CefRefPtr<CefFrame> frame,
		CefProcessId ,
		CefRefPtr<CefProcessMessage> message) override
	{
		auto name = message->GetName().ToString();
		if (name == "mixer-request-stats")
		{
			// just flag that we need to deliver stats updates
			// to the render process via a message
			needs_stats_update_ = true;
			// FIXME may keep pointer to the frame?
			return true;
		}
		return false;
	}

	void OnPaint(
		CefRefPtr<CefBrowser> /*browser*/,
		PaintElementType type,
		const RectList& dirtyRects,
		const void* buffer,
		int width,
		int height) override
	{
		if (type == PET_VIEW)
		{
			frame_++;
			auto const now = time_now();
			if (!fps_start_) {
				fps_start_ = now;
			}

			if (view_buffer_) {
				view_buffer_->on_paint(buffer, width, height);
			}

			if ((now - fps_start_) > 1000000)
			{
				auto const fps = frame_ / double((now - fps_start_) / 1000000.0);

				auto const w = view_buffer_ ? view_buffer_->width() : 0;
				auto const h = view_buffer_ ? view_buffer_->height() : 0;

				log_message("html: OnAcceleratedPaint (%dx%d), fps: %3.2f\n", w, h, fps);

				frame_ = 0;
				fps_start_ = time_now();
			}
		}
		else
		{
			// just update the popup frame ... we are only tracking 
			// metrics for the view

			if (popup_buffer_) {
				popup_buffer_->on_paint(buffer, width, height);
			}
		}

	}

	void OnAcceleratedPaint(
		CefRefPtr<CefBrowser> /*browser*/,
		PaintElementType type,
		const RectList& dirtyRects,
		void* share_handle) override
	{
		if (type == PET_VIEW)
		{
			frame_++;
			auto const now = time_now();
			if (!fps_start_) {
				fps_start_ = now;
			}

			if (view_buffer_) {
				view_buffer_->on_gpu_paint((void*)share_handle);
			}

			if ((now - fps_start_) > 1000000)
			{
				auto const fps = frame_ / double((now - fps_start_) / 1000000.0);

				auto const w = view_buffer_ ? view_buffer_->width() : 0;
				auto const h = view_buffer_ ? view_buffer_->height() : 0;

				log_message("html: OnAcceleratedPaint (%dx%d), fps: %3.2f\n", w, h, fps);

				frame_ = 0;
				fps_start_ = time_now();
			}
		}
		else
		{
			// just update the popup frame ... we are only tracking 
			// metrics for the view

			if (popup_buffer_) {
				popup_buffer_->on_gpu_paint((void*)share_handle);
			}
		}
	}

	void GetViewRect(CefRefPtr<CefBrowser> /*browser*/, CefRect& rect) override
	{
		rect.Set(0, 0, width_, height_);
	}

	void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
	{
		if (!CefCurrentlyOn(TID_UI))
		{
			assert(0);
			return;
		}

		{
			lock_guard<mutex> guard(lock_);
			if (!browser_.get()) {
				browser_ = browser;
			}
		}
	}

	void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override
	{
		log_message("FATAL : we should never end up in OnPopupShow");
	}

	void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) override
	{
		log_message("size popup - %d,%d  %dx%d\n", rect.x, rect.y, rect.width, rect.height);

		decltype(popup_layer_) layer;

		{
			lock_guard<mutex> guard(lock_);
			layer = popup_layer_;
		}

		if (layer)
		{
			auto const composition = layer->composition();
			if (composition)
			{
				auto const outer_width = composition->width();
				auto const outer_height = composition->height();
				if (outer_width > 0 && outer_height > 0)
				{
					auto const x = rect.x / float(outer_width);
					auto const y = rect.y / float(outer_height);
					auto const w = rect.width / float(outer_width);
					auto const h = rect.height / float(outer_height);
					layer->move(x, y, w, h);
				}
			}
		}
	}

	bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		const CefString& target_url,
		const CefString& target_frame_name,
		WindowOpenDisposition target_disposition,
		bool user_gesture,
		const CefPopupFeatures& popup_features,
		CefWindowInfo& window_info,
		CefRefPtr<CefClient>& client,
		CefBrowserSettings& settings,
		CefRefPtr<CefDictionaryValue>& extra_info,
		bool* no_javascript_access) override
	{
		log_message("fatal - this should not be needed");
		return false;
	}

	void OnLoadEnd(CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		int /*httpStatusCode*/)
	{
		dump_source(frame);
	}

	shared_ptr<d3d11::Texture2D> texture(shared_ptr<d3d11::Context> const& ctx)
	{
		if (view_buffer_) {
			return view_buffer_->swap(ctx);
		}
		return nullptr;
	}

	void tick(double t)
	{
		auto const browser = safe_browser();
		// optionally issue a BeginFrame request
		if (send_begin_frame_ && browser) {
			browser->GetHost()->SendExternalBeginFrame();
		}
	}

	void update_stats(CefRefPtr<CefBrowser> const& browser,
		shared_ptr<Composition> const& composition)
	{
	}

	void resize(int width, int height)
	{
		// only signal change if necessary
		if (width != width_ || height != height_)
		{
			width_ = width;
			height_ = height;

			auto const browser = safe_browser();
			if (browser)
			{
				browser->GetHost()->WasResized();
				log_message("html resize - %dx%d\n", width, height);
			}
		}
	}

	void dump_source(CefRefPtr<CefFrame> frame)
	{
	}

	void show_devtools()
	{
	}

	void mouse_click(MouseButton button, bool up, int32_t x, int32_t y)
	{
	}

	void mouse_move(bool leave, int32_t x, int32_t y)
	{
	}

	void refresh() {
		auto const browser = safe_browser();
		if (browser)
		{
			browser->Reload();
		}
	}

private:
	IMPLEMENT_REFCOUNTING(WebView);

	CefRefPtr<CefBrowser> safe_browser()
	{
		lock_guard<mutex> guard(lock_);
		return browser_;
	}

	string name_;
	int width_;
	int height_;
	uint32_t frame_;
	uint64_t fps_start_;
	shared_ptr<FrameBuffer> view_buffer_;
	shared_ptr<FrameBuffer> popup_buffer_;
	mutex lock_;
	CefRefPtr<CefBrowser> browser_;
	bool needs_stats_update_;
	bool use_shared_textures_;
	bool send_begin_frame_;

	shared_ptr<Layer> popup_layer_;
	shared_ptr<Composition> const composition_;
};


class WebLayer : public Layer
{
public:
	WebLayer(
		std::shared_ptr<d3d11::Device> const& device,
		bool want_input,
		CefRefPtr<WebView> const& view)
		: Layer(device, want_input, view->use_shared_textures())
		, view_(view) {
	}

	~WebLayer() {
		if (view_) {
			view_->close();
		}
	}

	//
	// forward composition reference to our view
	// it may use it for popup layers
	//
	void attach(std::shared_ptr<Composition> const& comp) override
	{
		Layer::attach(comp);
	}

	void tick(double t) override
	{
		auto const comp = composition();
		if (comp)
		{
			// The bounding box for this layer is in normalized coordinates,
			// the html view needs to know pixel size...so we convert from normalized
			// to pixels based on the composition dimensions (which are in pixels).
			//
			// Note: it is safe to call resize() on the view repeatedly since
			// it will ignore the call if the requested size is the same

			auto const rect = bounds();
			auto const width = static_cast<int>(rect.width * comp->width());
			auto const height = static_cast<int>(rect.height * comp->height());

			if (view_)
			{
				// from Masako Toda
				if (show_devtools_)
				{
					view_->show_devtools();
					show_devtools_ = false;
				}

				view_->resize(width, height);
				view_->tick(t);
			}
		}

		Layer::tick(t);
	}

	void render(shared_ptr<d3d11::Context> const& ctx) override
	{
		// simply use the base class method to draw our texture
		if (view_) {
			render_texture(ctx, view_->texture(ctx));
		}
	}

	void mouse_click(MouseButton button, bool up, int32_t x, int32_t y) override
	{
		if (view_) {
			view_->mouse_click(button, up, x, y);
		}
	}

	void mouse_move(bool leave, int32_t x, int32_t y) override
	{
		if (view_) {
			view_->mouse_move(leave, x, y);
		}
	}

	void refresh() override
	{
		if (view_) {
			view_->refresh();
		}
	}



private:

	CefRefPtr<WebView> const view_;
};

//
// a simple layer that will render out PET_POPUP for a
// corresponding view
//
class PopupLayer : public Layer
{
public:
	PopupLayer(
		shared_ptr<d3d11::Device> const& device,
		shared_ptr<FrameBuffer> const& buffer)
		: Layer(device, false, true)
		, frame_buffer_(buffer) {
	}

	void render(shared_ptr<d3d11::Context> const& ctx) override
	{
		if (frame_buffer_) {
			render_texture(ctx, frame_buffer_->swap(ctx));
		}
	}

private:
	shared_ptr<FrameBuffer> const frame_buffer_;
};



//
// Lifetime management for CEF components.  
//
// Manages the CEF message loop and CefInitialize/CefShutdown
//
class CefModule
{
public:
	CefModule(HINSTANCE mod) : module_(mod) {
		ready_ = false;
	}

	static void startup(HINSTANCE);
	static void shutdown();

private:

	//
	// simple CefTask we'll post to our message-pump 
	// thread to stop it (required to break out of CefRunMessageLoop)
	//
	class QuitTask : public CefTask
	{
	public:
		QuitTask() { }
		void Execute() override {
			CefQuitMessageLoop();
		}
		IMPLEMENT_REFCOUNTING(QuitTask);
	};

	void message_loop();

	condition_variable signal_;
	atomic_bool ready_;
	mutex lock_;
	HINSTANCE const module_;
	shared_ptr<thread> thread_;
	static shared_ptr<CefModule> instance_;
};

std::shared_ptr<CefModule> CefModule::instance_;

void CefModule::startup(HINSTANCE mod)
{
	assert(!instance_.get());
	instance_ = make_shared<CefModule>(mod);
	instance_->thread_ = make_shared<thread>(
		bind(&CefModule::message_loop, instance_.get()));

	{ // wait for message loop to initialize

		unique_lock<mutex> lock(instance_->lock_);
		weak_ptr<CefModule> weak_self(instance_);
		instance_->signal_.wait(lock, [weak_self]() {
			auto const mod = weak_self.lock();
			if (mod) {
				return mod->ready_.load();
			}
			return true;
		});
	}

	log_message("cef module is ready\n");
}

void CefModule::shutdown()
{
	assert(instance_.get());
	if (instance_)
	{
		if (instance_->thread_)
		{
			CefRefPtr<CefTask> task(new QuitTask());
			CefPostTask(TID_UI, task.get());
			instance_->thread_->join();
			instance_->thread_.reset();
		}
		instance_.reset();
	}
}

void CefModule::message_loop()
{
	log_message("cef initializing ... \n");

	CefSettings settings;
	settings.no_sandbox = true;
	settings.multi_threaded_message_loop = false;
	settings.windowless_rendering_enabled = true;

	CefRefPtr<WebApp> app(new WebApp());

	CefMainArgs main_args(module_);
	CefInitialize(main_args, settings, app, nullptr);

	log_message("cef is initialized.\n");

	// signal cef is initialized and ready
	ready_ = true;
	signal_.notify_one();

	CefRunMessageLoop();

	log_message("cef shutting down ... \n");

	CefShutdown();
	log_message("cef is shutdown\n");
}

//
// internal factory method so popups can create layers on the fly
//
shared_ptr<Layer> create_web_layer(
	shared_ptr<Composition> const& composition,
	bool want_input,
	CefRefPtr<WebView> const& view)
{
	return make_shared<WebLayer>(composition->device(), want_input, view);
}

//
// use CEF to load and render a web page within a layer
//
shared_ptr<Layer> create_web_layer(
	shared_ptr<Composition> const& composition,
	string const& url,
	int width,
	int height,
	bool want_input,
	bool view_source)
{
	CefWindowInfo window_info;
	window_info.SetAsWindowless(nullptr);

	// we want to use OnAcceleratedPaint
	window_info.shared_texture_enabled = true;

	// we are going to issue calls to SendExternalBeginFrame
	// and CEF will not use its internal BeginFrameTimer in this case
	window_info.external_begin_frame_enabled = true;

	CefBrowserSettings settings;

	// Set the maximum rate that the HTML content will render at
	//
	// NOTE: this value is NOT capped to 60 by CEF when using shared textures and
	// it is completely ignored when using SendExternalBeginFrame
	//
	// For testing, this application uses 120Hz to show that the 60Hz limit is ignored
	// (set window_info.external_begin_frame_enabled above to false to test)
	//
	settings.windowless_frame_rate = 120;

	string name;
	CefRefPtr<WebView> view(new WebView(
		name, composition, width, height,
		window_info.shared_texture_enabled,
		window_info.external_begin_frame_enabled));

	CefBrowserHost::CreateBrowser(
		window_info,
		view,
		url,
		settings, nullptr,
		nullptr);

	return create_web_layer(composition, want_input, view);
}

shared_ptr<Layer> create_popup_layer(
	shared_ptr<d3d11::Device> const& device,
	shared_ptr<FrameBuffer> const& buffer)
{
	if (device && buffer) {
		return make_shared<PopupLayer>(device, buffer);
	}
	return nullptr;
}

//
// public method to setup CEF for this application
//
int cef_initialize(HINSTANCE instance)
{
	CefEnableHighDPISupport();

	{ // check first if we need to run as a worker process

		CefRefPtr<WebApp> app(new WebApp());
		CefMainArgs main_args(instance);
		int exit_code = CefExecuteProcess(main_args, app, nullptr);
		if (exit_code >= 0) {
			return exit_code;
		}
	}

	//MessageBox(0, L"Attach Debugger", L"CEF OSR", MB_OK);

	CefModule::startup(instance);
	return -1;
}

//
// public method to tear-down CEF ... call before your main() function exits
//
void cef_uninitialize()
{
	CefModule::shutdown();
}

//
// return the CEF + Chromium version
//
string cef_version()
{
	ostringstream ver;
	ver << "CEF: " <<
		CEF_VERSION << " (Chromium: "
		<< CHROME_VERSION_MAJOR << "."
		<< CHROME_VERSION_MINOR << "."
		<< CHROME_VERSION_BUILD << "."
		<< CHROME_VERSION_PATCH << ")";
	return ver.str();
}

