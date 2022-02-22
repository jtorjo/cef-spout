#include "composition.h"
#include "util.h"

#include <include/cef_parser.h>

using namespace std;

DXGI_FORMAT texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;


Layer::Layer(
	shared_ptr<d3d11::Device> const& device, bool want_input, bool flip)
	: device_(device)
	, flip_(flip)
	, want_input_(want_input)
{
	bounds_.x = bounds_.y = bounds_.width = bounds_.height = 0.0f;
}

Layer::~Layer() {
}

void Layer::attach(std::shared_ptr<Composition> const& parent) {
	composition_ = parent;
}

shared_ptr<Composition> Layer::composition() const {
	return composition_.lock();
}

Rect Layer::bounds() const {
	return bounds_;
}

bool Layer::want_input() const {
	return want_input_;
}

void Layer::mouse_click(MouseButton, bool, int32_t, int32_t)
{
	// default is to do nothing with input
}

void Layer::mouse_move(bool, int32_t, int32_t)
{
	// default is to do nothing with input
}

void Layer::refresh()
{
	//nothing
}

void Layer::move(float x, float y, float width, float height)
{
	bounds_.x = x;
	bounds_.y = y;
	bounds_.width = width;
	bounds_.height = height;

	// obviously, it is not efficient to create the quad everytime we
	// move ... but for now we're just trying to get something on-screen
	geometry_.reset();
}

void Layer::tick(double)
{
	// nothing to update in the base class
}

//
// helper method for derived classes to draw a textured-quad.
//
void Layer::render_texture(
	shared_ptr<d3d11::Context> const& ctx,
	shared_ptr<d3d11::Texture2D> const& texture)
{
	if (!geometry_) {
		geometry_ = device_->create_quad(bounds_.x,
			bounds_.y, bounds_.width, bounds_.height, flip_);
	}

	if (geometry_ && texture)
	{
		// we need a shader
		if (!effect_) {
			effect_ = device_->create_default_effect();
		}

		// bind our states/resource to the pipeline
		d3d11::ScopedBinder<d3d11::Geometry> quad_binder(ctx, geometry_);
		d3d11::ScopedBinder<d3d11::Effect> fx_binder(ctx, effect_);
		d3d11::ScopedBinder<d3d11::Texture2D> tex_binder(ctx, texture);

		// actually draw the quad
		geometry_->draw();
	}
}


Composition::Composition(int width, int height)
	: width_(width)
	, height_(height)
	, vsync_(true)
{
	fps_ = 0.0;
	time_ = 0.0;
	frame_ = 0;
	fps_start_ = time_now();

	auto ok = sender_.OpenDirectX11();
	if (!ok)
		log_message("FATAL: can't open directx11");
	device_ = make_shared<d3d11::Device>(sender_.GetDX11Device(), sender_.GetDX11Context());
	sender_.SetSenderName("Simple Windows sender");
	sender_.SetSenderFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	sender_.SetMaxSenders(10);
}

bool Composition::is_vsync() const
{
	return vsync_;
}

double Composition::time() const
{
	return time_;
}

double Composition::fps() const
{
	return fps_;
}

void Composition::add_layer(shared_ptr<Layer> const& layer)
{
	if (layer)
	{
		lock_guard<mutex> guard(lock_);

		layers_.push_back(layer);

		// attach ourself as the parent
		layer->attach(shared_from_this());
	}
}

bool Composition::remove_layer(std::shared_ptr<Layer> const& layer)
{
	size_t match = 0;
	if (layer)
	{
		lock_guard<mutex> guard(lock_);
		for (auto i = layers_.begin(); i != layers_.end(); )
		{
			if ((*i).get() == layer.get()) {
				i = layers_.erase(i);
				++match;
			}
			else {
				i++;
			}
		}
	}
	return (match > 0);
}

void Composition::resize(bool vsync, int width, int height)
{
	vsync_ = vsync;
	width_ = width;
	height_ = height;
}

void Composition::tick(double t)
{
	time_ = t;

	// don't hold a lock during tick()
	decltype(layers_) layers;
	{
		lock_guard<mutex> guard(lock_);
		layers.assign(layers_.begin(), layers_.end());
	}

	for (auto const& layer : layers) {
		layer->tick(t);
	}
}

void Composition::render(shared_ptr<d3d11::Context> const& ctx)
{
	// don't hold a lock during render()
	decltype(layers_) layers;
	{
		lock_guard<mutex> guard(lock_);
		layers.assign(layers_.begin(), layers_.end());
	}

	// pretty simple ... just use painter's algorithm and render 
	// our layers in order (not doing any depth or 3D here)
	for (auto const& layer : layers) {
		layer->render(ctx);
	}

	frame_++;
	auto const now = time_now();
	if ((now - fps_start_) > 1000000)
	{
		fps_ = frame_ / double((now - fps_start_) / 1000000.0);
		//log_message("composition: fps: %3.2f\n", fps_);
		frame_ = 0;
		fps_start_ = time_now();
	}
}

void Composition::mouse_click(MouseButton button, bool up, int32_t x, int32_t y)
{
	// forward to layer - making x, y relative to layer
	auto const layer = layer_from_point(x, y);
	if (layer) {
		layer->mouse_click(button, up, x, y);
	}
}

void Composition::mouse_move(bool leave, int32_t x, int32_t y)
{
	// forward to layer - making x, y relative to layer
	auto const layer = layer_from_point(x, y);
	if (layer) {
		layer->mouse_move(leave, x, y);
	}
}

void Composition::refresh()
{
	// forward to layer - making x, y relative to layer
	for (size_t i = 0; i < layers_.size(); i++)
	{
		auto const layer = layers_[i];
		if (layer) {
			layer->refresh();
		}
	}
}

shared_ptr<Layer> Composition::layer_from_point(int32_t& x, int32_t& y)
{
	auto const w = width();
	auto const h = height();

	// get thread-safe copy
	decltype(layers_) layers;
	{
		lock_guard<mutex> guard(lock_);
		layers.assign(layers_.begin(), layers_.end());
	}

	//
	// walk layers from front to back and find one 
	// that contains the mouse point (and wants input)
	//
	for (auto i = layers.rbegin(); i != layers.rend(); i++)
	{
		auto const l = (*i);
		if (l->want_input())
		{
			auto bounds = l->bounds();

			// convert to screen space
			auto const sx = static_cast<int32_t>(bounds.x * w);
			auto const sw = static_cast<int32_t>(bounds.width * w);
			auto const sy = static_cast<int32_t>(bounds.y * h);
			auto const sh = static_cast<int32_t>(bounds.height * h);
			if (x >= sx && x < (sx + sw))
			{
				if (y >= sy && y < (sy + sh))
				{
					// convert points to relative
					x = x - sx;
					y = y - sy;
					return l;
				}
			}
		}
	}

	return nullptr;
}

void Composition::draw_spout(ID3D11Texture2D* texture) {
	sender_.SendTexture(texture);
	sender_.HoldFps(60);
}
ID3D11Texture2D* Composition::create_texture() {
	ID3D11Texture2D* texture = nullptr;
	sender_.CreateDX11texture(*device_, width_, height_, texFormat, &texture);
	return texture;
}










int to_int(CefRefPtr<CefDictionaryValue> const& dict, string const& key, int default_value)
{
	if (dict)
	{
		auto const type = dict->GetType(key);
		if (type == VTYPE_INT) {
			return dict->GetInt(key);
		}
		if (type == VTYPE_DOUBLE) {
			return static_cast<int>(dict->GetDouble(key));
		}
	}
	return default_value;
}

float to_float(CefRefPtr<CefDictionaryValue> const& dict, string const& key, float default_value)
{
	if (dict)
	{
		auto const type = dict->GetType(key);
		if (type == VTYPE_INT) {
			return static_cast<float>(dict->GetInt(key));
		}
		if (type == VTYPE_DOUBLE) {
			return static_cast<float>(dict->GetDouble(key));
		}
	}
	return default_value;
}

//
// create a composition layer from the given dictionary
//
shared_ptr<Layer> to_layer(
	shared_ptr<Composition> const& composition,
	int width,
	int height,
	CefRefPtr<CefDictionaryValue> const& dict)
{
	if (!dict || dict->GetType("type") != VTYPE_STRING) {
		return nullptr;
	}

	auto const type = dict->GetString("type");

	// get the url or filename for the layer
	if (dict->GetType("src") != VTYPE_STRING) {
		return nullptr;
	}
	auto const src = dict->GetString("src");

if (type == "web")
	{
		auto const want_input = dict->GetBool("want_input");
		auto const view_source = dict->GetBool("view_source");

		return create_web_layer(composition, src, width, height, want_input, view_source);
	}

	return nullptr;
}

shared_ptr<Composition> create_composition(
	string const& json)
{
	auto const val = CefParseJSON(
		CefString(json), JSON_PARSER_ALLOW_TRAILING_COMMAS);
	if (!val.get() || !val->IsValid() || (val->GetType() != VTYPE_DICTIONARY)) {
		return nullptr;
	}

	auto const dict = val->GetDictionary();

	auto const composition = make_shared<Composition>(window_width(), window_height());

	// create and add layers as defined in the layers array
	if (dict->GetType("layers") == VTYPE_LIST)
	{
		auto const layers = dict->GetList("layers");
		if (layers && layers->IsValid())
		{
			for (size_t n = 0; n < layers->GetSize(); ++n)
			{
				if (layers->GetType(n) == VTYPE_DICTIONARY)
				{
					auto const obj = layers->GetDictionary(n);
					if (obj && obj->IsValid())
					{
						// create a valid layer from the JSON-object
						auto const layer = to_layer(composition,
							composition->width(),
							composition->height(),
							obj);
						if (!layer) {
							continue;
						}

						// add the layer to the composition
						composition->add_layer(layer);

						// move to default position
						auto const x = to_float(obj, "left", 0.0f);
						auto const y = to_float(obj, "top", 0.0f);
						auto const w = to_float(obj, "width", 1.0f);
						auto const h = to_float(obj, "height", 1.0f);

						layer->move(x, y, w, h);
					}
				}
			}
		}
	}

	return composition;
}

