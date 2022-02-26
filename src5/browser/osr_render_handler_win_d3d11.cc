// Copyright 2018 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "osr_render_handler_win_d3d11.h"

#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "tests/shared/browser/util_win.h"

#include "../spoutDX.h"
#include <thread>

namespace client {

BrowserLayer::BrowserLayer(const std::shared_ptr<d3d11::Device>& device)
    : d3d11::Layer(device, true /* flip */) {
  frame_buffer_ = std::make_shared<d3d11::FrameBuffer>(device_);
}

void BrowserLayer::render(const std::shared_ptr<d3d11::Context>& ctx) {
  // Use the base class method to draw our texture.
  render_texture(ctx, frame_buffer_->texture());
}

void BrowserLayer::on_paint(void* share_handle, spoutDX * sender) {
  frame_buffer_->on_paint(share_handle, sender);
}

std::pair<uint32_t, uint32_t> BrowserLayer::texture_size() const {
  const auto texture = frame_buffer_->texture();
  return std::make_pair(texture->width(), texture->height());
}

PopupLayer::PopupLayer(const std::shared_ptr<d3d11::Device>& device)
    : BrowserLayer(device) {}

void PopupLayer::set_bounds(const CefRect& bounds) {
  const auto comp = composition();
  if (!comp)
    return;

  const auto outer_width = comp->width();
  const auto outer_height = comp->height();
  if (outer_width == 0 || outer_height == 0)
    return;

  original_bounds_ = bounds;
  bounds_ = bounds;

  // If x or y are negative, move them to 0.
  if (bounds_.x < 0)
    bounds_.x = 0;
  if (bounds_.y < 0)
    bounds_.y = 0;
  // If popup goes outside the view, try to reposition origin
  if (bounds_.x + bounds_.width > outer_width)
    bounds_.x = outer_width - bounds_.width;
  if (bounds_.y + bounds_.height > outer_height)
    bounds_.y = outer_height - bounds_.height;
  // If x or y became negative, move them to 0 again.
  if (bounds_.x < 0)
    bounds_.x = 0;
  if (bounds_.y < 0)
    bounds_.y = 0;

  const auto x = bounds_.x / float(outer_width);
  const auto y = bounds_.y / float(outer_height);
  const auto w = bounds_.width / float(outer_width);
  const auto h = bounds_.height / float(outer_height);
  move(x, y, w, h);
}

OsrRenderHandlerWinD3D11::OsrRenderHandlerWinD3D11(
    const OsrRendererSettings& settings,
    HWND hwnd)
    : OsrRenderHandlerWin(settings, hwnd), start_time_(0) {
    width_ = height_ = 0;
}

bool OsrRenderHandlerWinD3D11::Initialize(CefRefPtr<CefBrowser> browser,
                                          int width,
                                          int height) {
  CEF_REQUIRE_UI_THREAD();


  sender_ = std::make_shared<spoutDX>();
  auto ok = sender_->OpenDirectX11();
  if (!ok) {
      LOG(FATAL) << "can't open directx11";
      return false;
  }

  // Create a D3D11 device instance.
  //device_ = d3d11::Device::create();
  device_ = std::make_shared<d3d11::Device>(sender_->GetDX11Device(), sender_->GetDX11Context());
  DCHECK(device_);
  if (!device_)
    return false;

  sender_->SetSenderName("Simple Windows sender");
  sender_->SetSenderFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
  sender_->SetMaxSenders(10);

  // Create a D3D11 swapchain for the window.
  swap_chain_ = device_->create_swapchain(hwnd());
  DCHECK(swap_chain_);
  if (!swap_chain_)
    return false;

  // Create the browser layer.
  browser_layer_ = std::make_shared<BrowserLayer>(device_);

  // Set up the composition.
  composition_ = std::make_shared<d3d11::Composition>(device_, width, height);
  composition_->add_layer(browser_layer_);

  // Size to the whole composition.
  browser_layer_->move(0.0f, 0.0f, 1.0f, 1.0f);

  start_time_ = GetTimeNow();

  SetBrowser(browser);

  std::thread t(&OsrRenderHandlerWinD3D11::render_thread, this);
  t.detach();
  return true;
}

void OsrRenderHandlerWinD3D11::SetSpin(float spinX, float spinY) {
  CEF_REQUIRE_UI_THREAD();
  // Spin support is not implemented.
}

void OsrRenderHandlerWinD3D11::IncrementSpin(float spinDX, float spinDY) {
  CEF_REQUIRE_UI_THREAD();
  // Spin support is not implemented.
}

bool OsrRenderHandlerWinD3D11::IsOverPopupWidget(int x, int y) const {
  CEF_REQUIRE_UI_THREAD();
  return popup_layer_ && popup_layer_->contains(x, y);
}

int OsrRenderHandlerWinD3D11::GetPopupXOffset() const {
  CEF_REQUIRE_UI_THREAD();
  if (popup_layer_)
    return popup_layer_->xoffset();
  return 0;
}

int OsrRenderHandlerWinD3D11::GetPopupYOffset() const {
  CEF_REQUIRE_UI_THREAD();
  if (popup_layer_)
    return popup_layer_->yoffset();
  return 0;
}

void OsrRenderHandlerWinD3D11::OnPopupShow(CefRefPtr<CefBrowser> browser,
                                           bool show) {
  CEF_REQUIRE_UI_THREAD();

  if (show) {
    DCHECK(!popup_layer_);

    // Create a new layer.
    popup_layer_ = std::make_shared<PopupLayer>(device_);
    composition_->add_layer(popup_layer_);
  } else {
    DCHECK(popup_layer_);

    composition_->remove_layer(popup_layer_);
    popup_layer_ = nullptr;

    Render();
  }
}

void OsrRenderHandlerWinD3D11::OnPopupSize(CefRefPtr<CefBrowser> browser,
                                           const CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();
  popup_layer_->set_bounds(rect);
}

void OsrRenderHandlerWinD3D11::OnPaint(
    CefRefPtr<CefBrowser> browser,
    CefRenderHandler::PaintElementType type,
    const CefRenderHandler::RectList& dirtyRects,
    const void* buffer,
    int width,
    int height) {
  // Not used with this implementation.
  NOTREACHED();
}

void OsrRenderHandlerWinD3D11::OnAcceleratedPaint(
    CefRefPtr<CefBrowser> browser,
    CefRenderHandler::PaintElementType type,
    const CefRenderHandler::RectList& dirtyRects,
    void* share_handle) {
  CEF_REQUIRE_UI_THREAD();

  if (type == PET_POPUP) {
    popup_layer_->on_paint(share_handle, sender_.get());
  } else {
    browser_layer_->on_paint(share_handle, sender_.get());
  }

  Render();
}

void OsrRenderHandlerWinD3D11::render_thread() {
    ::Sleep(5000);
 /*
    width_ = 500;
    height_ = 500;
    uint32_t stride = width_ * 4;
    size_t cb = stride * height_;
    std::shared_ptr<uint8_t> sw_buffer_;
    sw_buffer_ = std::shared_ptr<uint8_t>((uint8_t*)malloc(cb), free);
    */
    while (true) {
        ::Sleep(100);

        /*
        for (uint32_t i = 0; i < width_ * height_ * 4; ++i)
            sw_buffer_.get()[i] = 0x7f;

        sender_->SendImage(sw_buffer_.get(), width_, height_);
        */

        auto ctx = device_->immedidate_context();
        d3d11::ScopedBinder<d3d11::Texture2D> binder(ctx, texture_);
        sender_->SendTexture( texture_->texture_handle());
        sender_->HoldFps(60);        
    }
}


void OsrRenderHandlerWinD3D11::Render() {
  // Update composition + layers based on time.
  const auto t = (GetTimeNow() - start_time_) / 1000000.0;
  composition_->tick(t);

  auto ctx = device_->immedidate_context();
  swap_chain_->bind(ctx);

  const auto texture_size = browser_layer_->texture_size();
  if (width_ != texture_size.first || height_ != texture_size.second) {
    width_ = texture_size.first;
    height_ = texture_size.second;
    texture_ = device_->create_texture(width_, height_, DXGI_FORMAT_B8G8R8A8_UNORM, nullptr, 0);
    
  }

  // Resize the composition and swap chain to match the texture if necessary.
  composition_->resize(!send_begin_frame(), texture_size.first, texture_size.second);
  swap_chain_->resize(texture_size.first, texture_size.second);

  // Clear the render target.
  swap_chain_->clear(0.0f, 0.0f, 1.0f, 1.0f);

  // Render the scene.
  composition_->render(ctx);

  // Present to window.
  swap_chain_->present(send_begin_frame() ? 0 : 1);

  //sender_->SendTexture(browser_layer_->texture()->texture_handle());

  d3d11::ScopedBinder<d3d11::Texture2D> binder(ctx, texture_);
  texture_->copy_from( browser_layer_->texture());


  /* crash, even with browser_layer_->frame_buffer()->resize(width_, height_, sender_.get());
  * 
  ID3D11DeviceContext* d3ctx = *ctx.get();  
  D3D11_MAPPED_SUBRESOURCE res;
  auto hr = d3ctx->Map(browser_layer_->texture()->texture_handle(), 0, D3D11_MAP_READ, 0, &res);
  if (hr == S_OK)
    memcpy(sw_buffer_.get(), res.pData, 4 * width_ * height_);
  d3ctx->Unmap(browser_layer_->texture()->texture_handle(), 0);
  */
}

}  // namespace client
