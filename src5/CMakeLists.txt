# Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

#
# Source files.
#

#AUX_SOURCE_DIRECTORY(spout_src_files "../lib_spout/SpoutGL/*.cpp")
#AUX_SOURCE_DIRECTORY(spout_inc_files "../lib_spout/SpoutGL/*.h")

# cefclient browser sources.
set(CEFCLIENT_BROWSER_BROWSER_SRCS
  browser/binding_test.cc
  browser/binding_test.h
  browser/browser_window.cc
  browser/browser_window.h
  browser/bytes_write_handler.cc
  browser/bytes_write_handler.h
  browser/client_app_delegates_browser.cc
  browser/client_browser.cc
  browser/client_browser.h
  browser/client_handler.cc
  browser/client_handler.h
  browser/client_handler_osr.cc
  browser/client_handler_osr.h
  browser/client_handler_std.cc
  browser/client_handler_std.h
  browser/client_types.h
  browser/dialog_test.cc
  browser/dialog_test.h
  browser/drm_test.cc
  browser/drm_test.h
  browser/image_cache.cc
  browser/image_cache.h
  browser/main_context.cc
  browser/main_context.h
  browser/main_context_impl.cc
  browser/main_context_impl.h
  browser/media_router_test.cc
  browser/media_router_test.h
  browser/osr_dragdrop_events.h
  browser/osr_renderer.cc
  browser/osr_renderer.h
  browser/osr_renderer_settings.h
  browser/preferences_test.cc
  browser/preferences_test.h
  browser/resource.h
  browser/response_filter_test.cc
  browser/response_filter_test.h
  browser/root_window.cc
  browser/root_window.h
  browser/root_window_create.cc
  browser/root_window_manager.cc
  browser/root_window_manager.h
  browser/scheme_test.cc
  browser/scheme_test.h
  browser/server_test.cc
  browser/server_test.h
  browser/temp_window.h
  browser/test_runner.cc
  browser/test_runner.h
  browser/urlrequest_test.cc
  browser/urlrequest_test.h
  browser/window_test.cc
  browser/window_test.h
  browser/window_test_runner.cc
  browser/window_test_runner.h
  util.h
  util.cpp
  SpoutDX.h
  SpoutDX.cpp
  platform.h
  ${spout_src_files}
  ${spout_inc_files}
  )
source_group(cefclient\\\\browser FILES ${CEFCLIENT_BROWSER_BROWSER_SRCS})

set(CEFCLIENT_BROWSER_SHARED_BROWSER_SRCS
  ../shared/browser/client_app_browser.cc
  ../shared/browser/client_app_browser.h
  ../shared/browser/extension_util.cc
  ../shared/browser/extension_util.h
  ../shared/browser/file_util.cc
  ../shared/browser/file_util.h
  ../shared/browser/geometry_util.cc
  ../shared/browser/geometry_util.h
  ../shared/browser/main_message_loop.cc
  ../shared/browser/main_message_loop.h
  ../shared/browser/main_message_loop_external_pump.cc
  ../shared/browser/main_message_loop_external_pump.h
  ../shared/browser/main_message_loop_std.cc
  ../shared/browser/main_message_loop_std.h
  ../shared/browser/resource_util.h
  )
source_group(shared\\\\browser FILES ${CEFCLIENT_BROWSER_SHARED_BROWSER_SRCS})

set(CEFCLIENT_BROWSER_SRCS
  ${CEFCLIENT_BROWSER_BROWSER_SRCS}
  ${CEFCLIENT_BROWSER_SHARED_BROWSER_SRCS}
  )

# cefclient common sources.
set(CEFCLIENT_COMMON_COMMON_SRCS
  common/client_app_delegates_common.cc
  common/scheme_test_common.cc
  common/scheme_test_common.h
  )
source_group(cefclient\\\\common FILES ${CEFCLIENT_COMMON_COMMON_SRCS})

set(CEFCLIENT_COMMON_SHARED_COMMON_SRCS
  ../shared/common/client_app.cc
  ../shared/common/client_app.h
  ../shared/common/client_app_other.cc
  ../shared/common/client_app_other.h
  ../shared/common/client_switches.cc
  ../shared/common/client_switches.h
  )
source_group(shared\\\\common FILES ${CEFCLIENT_COMMON_SHARED_COMMON_SRCS})

set(CEFCLIENT_COMMON_SRCS
  ${CEFCLIENT_COMMON_COMMON_SRCS}
  ${CEFCLIENT_COMMON_SHARED_COMMON_SRCS}
  )

# cefclient renderer sources.
set(CEFCLIENT_RENDERER_RENDERER_SRCS
  renderer/client_app_delegates_renderer.cc
  renderer/client_renderer.cc
  renderer/client_renderer.h
  renderer/performance_test.cc
  renderer/performance_test.h
  renderer/performance_test_setup.h
  renderer/performance_test_tests.cc
  )
source_group(cefclient\\\\renderer FILES ${CEFCLIENT_RENDERER_RENDERER_SRCS})

set(CEFCLIENT_RENDERER_SHARED_RENDERER_SRCS
  ../shared/renderer/client_app_renderer.cc
  ../shared/renderer/client_app_renderer.h
  )
source_group(shared\\\\renderer FILES ${CEFCLIENT_RENDERER_SHARED_RENDERER_SRCS})

set(CEFCLIENT_RENDERER_SRCS
  ${CEFCLIENT_RENDERER_RENDERER_SRCS}
  ${CEFCLIENT_RENDERER_SHARED_RENDERER_SRCS}
  )

#cefclient Linux sources
set(CEFCLIENT_LINUX_SRCS
  cefclient_gtk.cc
  )
source_group(cefclient FILES ${CEFCLIENT_LINUX_SRCS})

set(CEFCLIENT_LINUX_BROWSER_SRCS
  browser/browser_window_osr_gtk.cc
  browser/browser_window_osr_gtk.h
  browser/browser_window_std_gtk.cc
  browser/browser_window_std_gtk.h
  browser/dialog_handler_gtk.cc
  browser/dialog_handler_gtk.h
  browser/main_context_impl_posix.cc
  browser/main_message_loop_multithreaded_gtk.cc
  browser/main_message_loop_multithreaded_gtk.h
  browser/print_handler_gtk.cc
  browser/print_handler_gtk.h
  browser/resource_util_linux.cc
  browser/root_window_gtk.cc
  browser/root_window_gtk.h
  browser/root_window_views.cc
  browser/root_window_views.h
  browser/temp_window_x11.cc
  browser/temp_window_x11.h
  browser/util_gtk.cc
  browser/util_gtk.h
  browser/views_menu_bar.cc
  browser/views_menu_bar.h
  browser/views_style.cc
  browser/views_style.h
  browser/views_window.cc
  browser/views_window.h
  browser/window_test_runner_gtk.cc
  browser/window_test_runner_gtk.h
  browser/window_test_runner_views.cc
  browser/window_test_runner_views.h
  )
source_group(cefclient\\\\browser FILES ${CEFCLIENT_LINUX_BROWSER_SRCS})

set(CEFCLIENT_LINUX_SHARED_BROWSER_SRCS
  ../shared/browser/main_message_loop_external_pump_linux.cc
  ../shared/browser/resource_util_posix.cc
  )
source_group(shared\\\\browser FILES ${CEFCLIENT_LINUX_SHARED_BROWSER_SRCS})

set(CEFCLIENT_LINUX_SRCS
  ${CEFCLIENT_LINUX_SRCS}
  ${CEFCLIENT_LINUX_BROWSER_SRCS}
  ${CEFCLIENT_LINUX_SHARED_BROWSER_SRCS}
  )

#cefclient Mac OS X sources
set(CEFCLIENT_MACOSX_SRCS
  cefclient_mac.mm
  )
source_group(cefclient FILES ${CEFCLIENT_MACOSX_SRCS})

set(CEFCLIENT_MACOSX_BROWSER_SRCS
  browser/browser_window_osr_mac.h
  browser/browser_window_osr_mac.mm
  browser/browser_window_std_mac.h
  browser/browser_window_std_mac.mm
  browser/main_context_impl_posix.cc
  browser/osr_accessibility_helper.cc
  browser/osr_accessibility_helper.h
  browser/osr_accessibility_node.cc
  browser/osr_accessibility_node.h
  browser/osr_accessibility_node_mac.mm
  browser/root_window_mac.h
  browser/root_window_mac.mm
  browser/temp_window_mac.h
  browser/temp_window_mac.mm
  browser/text_input_client_osr_mac.h
  browser/text_input_client_osr_mac.mm
  browser/window_test_runner_mac.h
  browser/window_test_runner_mac.mm
  )
source_group(cefclient\\\\browser FILES ${CEFCLIENT_MACOSX_BROWSER_SRCS})

set(CEFCLIENT_MACOSX_SHARED_BROWSER_SRCS
  ../shared/browser/main_message_loop_external_pump_mac.mm
  ../shared/browser/resource_util_mac.mm
  ../shared/browser/resource_util_posix.cc
  )
source_group(shared\\\\browser FILES ${CEFCLIENT_MACOSX_SHARED_BROWSER_SRCS})

set(CEFCLIENT_MACOSX_SRCS
  ${CEFCLIENT_MACOSX_SRCS}
  ${CEFCLIENT_MACOSX_BROWSER_SRCS}
  ${CEFCLIENT_MACOSX_SHARED_BROWSER_SRCS}
  )

# cefclient Mac OS X helper sources.
set(CEFCLIENT_HELPER_SHARED_SRCS
  ../shared/process_helper_mac.cc
  )
source_group(shared FILES ${CEFCLIENT_HELPER_SHARED_SRCS})

set(CEFCLIENT_MACOSX_HELPER_SRCS
  ${CEFCLIENT_HELPER_SHARED_SRCS}
  )

#cefclient Windows sources
set(CEFCLIENT_WINDOWS_SRCS
  cefclient_win.cc
  )
source_group(cefclient FILES ${CEFCLIENT_WINDOWS_SRCS})

set(CEFCLIENT_WINDOWS_BROWSER_SRCS
  browser/browser_window_osr_win.cc
  browser/browser_window_osr_win.h
  browser/browser_window_std_win.cc
  browser/browser_window_std_win.h
  browser/main_context_impl_win.cc
  browser/main_message_loop_multithreaded_win.cc
  browser/main_message_loop_multithreaded_win.h
  browser/osr_accessibility_helper.cc
  browser/osr_accessibility_helper.h
  browser/osr_accessibility_node.cc
  browser/osr_accessibility_node.h
  browser/osr_accessibility_node_win.cc
  browser/osr_d3d11_win.cc
  browser/osr_d3d11_win.h
  browser/osr_dragdrop_win.cc
  browser/osr_dragdrop_win.h
  browser/osr_ime_handler_win.cc
  browser/osr_ime_handler_win.h
  browser/osr_render_handler_win.cc
  browser/osr_render_handler_win.h
  browser/osr_render_handler_win_d3d11.cc
  browser/osr_render_handler_win_d3d11.h
  browser/osr_render_handler_win_gl.cc
  browser/osr_render_handler_win_gl.h
  browser/osr_window_win.cc
  browser/osr_window_win.h
  browser/resource_util_win_idmap.cc
  browser/root_window_views.cc
  browser/root_window_views.h
  browser/root_window_win.cc
  browser/root_window_win.h
  browser/temp_window_win.cc
  browser/temp_window_win.h
  browser/views_menu_bar.cc
  browser/views_menu_bar.h
  browser/views_style.cc
  browser/views_style.h
  browser/views_window.cc
  browser/views_window.h
  browser/window_test_runner_views.cc
  browser/window_test_runner_views.h
  browser/window_test_runner_win.cc
  browser/window_test_runner_win.h
  )
source_group(cefclient\\\\browser FILES ${CEFCLIENT_WINDOWS_BROWSER_SRCS})

set(CEFCLIENT_WINDOWS_RESOURCES_WIN_SRCS
  resources/win/cefclient.rc
  )
source_group(cefclient\\\\resources\\\\win FILES ${CEFCLIENT_WINDOWS_RESOURCES_WIN_SRCS})

set(CEFCLIENT_WINDOWS_SHARED_BROWSER_SRCS
  ../shared/browser/main_message_loop_external_pump_win.cc
  ../shared/browser/resource_util_win.cc
  ../shared/browser/util_win.cc
  ../shared/browser/util_win.h
  )
source_group(shared\\\\browser FILES ${CEFCLIENT_WINDOWS_SHARED_BROWSER_SRCS})

set(CEFCLIENT_WINDOWS_SRCS
  ${CEFCLIENT_WINDOWS_SRCS}
  ${CEFCLIENT_WINDOWS_BROWSER_SRCS}
  ${CEFCLIENT_WINDOWS_RESOURCES_WIN_SRCS}
  ${CEFCLIENT_WINDOWS_SHARED_BROWSER_SRCS}
  )

# cefclient resources.
set(CEFCLIENT_RESOURCES_RESOURCES_SRCS
  resources/binding.html
  resources/dialogs.html
  resources/draggable.html
  resources/drm.html
  resources/localstorage.html
  resources/logo.png
  resources/media_router.html
  resources/menu_icon.1x.png
  resources/menu_icon.2x.png
  resources/other_tests.html
  resources/performance.html
  resources/performance2.html
  resources/preferences.html
  resources/response_filter.html
  resources/server.html
  resources/transparency.html
  resources/urlrequest.html
  resources/websocket.html
  resources/window.html
  resources/xmlhttprequest.html
  )
source_group(cefclient\\\\resources FILES ${CEFCLIENT_RESOURCES_RESOURCES_SRCS})

set(CEFCLIENT_RESOURCES_RESOURCES_EXTENSIONS_SET_PAGE_COLOR_SRCS
  resources/extensions/set_page_color/README.md
  resources/extensions/set_page_color/icon.png
  resources/extensions/set_page_color/manifest.json
  resources/extensions/set_page_color/popup.html
  resources/extensions/set_page_color/popup.js
  )
source_group(cefclient\\\\resources\\\\extensions\\\\set_page_color FILES ${CEFCLIENT_RESOURCES_RESOURCES_EXTENSIONS_SET_PAGE_COLOR_SRCS})

set(CEFCLIENT_RESOURCES_RESOURCES_MAC_SRCS_MACOSX
  resources/mac/Info.plist
  resources/mac/cefclient.icns
  )
APPEND_PLATFORM_SOURCES(CEFCLIENT_RESOURCES_RESOURCES_MAC_SRCS)
source_group(cefclient\\\\resources\\\\mac FILES ${CEFCLIENT_RESOURCES_RESOURCES_MAC_SRCS})

set(CEFCLIENT_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS_MACOSX
  resources/mac/English.lproj/InfoPlist.strings
  resources/mac/English.lproj/MainMenu.xib
  )
APPEND_PLATFORM_SOURCES(CEFCLIENT_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS)
source_group(cefclient\\\\resources\\\\mac\\\\English.lproj FILES ${CEFCLIENT_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS})

set(CEFCLIENT_RESOURCES_SHARED_RESOURCES_SRCS
  ../shared/resources/osr_test.html
  ../shared/resources/pdf.html
  ../shared/resources/pdf.pdf
  ../shared/resources/window_icon.1x.png
  ../shared/resources/window_icon.2x.png
  )
source_group(shared\\\\resources FILES ${CEFCLIENT_RESOURCES_SHARED_RESOURCES_SRCS})

set(CEFCLIENT_RESOURCES_SRCS
  ${CEFCLIENT_RESOURCES_RESOURCES_SRCS}
  ${CEFCLIENT_RESOURCES_RESOURCES_EXTENSIONS_SET_PAGE_COLOR_SRCS}
  ${CEFCLIENT_RESOURCES_RESOURCES_MAC_SRCS}
  ${CEFCLIENT_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS}
  ${CEFCLIENT_RESOURCES_SHARED_RESOURCES_SRCS}
  )


#
# Shared configuration.
#

# Target executable names.
set(CEF_TARGET "cef_spout")
if(OS_MACOSX)
  set(CEF_HELPER_TARGET "cefclient_Helper")
  set(CEF_HELPER_OUTPUT_NAME "cefclient Helper")
else()
  # Logical target used to link the libcef library.
  ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
endif()

# Determine the target output directory.
SET_CEF_TARGET_OUT_DIR()





#
# Windows configuration.
#

# All sources required by the "cefclient" target. Generates an executable that
# is used for all processes.
set(CEFCLIENT_SRCS
    ${CEFCLIENT_BROWSER_SRCS}
    ${CEFCLIENT_COMMON_SRCS}
    ${CEFCLIENT_RENDERER_SRCS}
    ${CEFCLIENT_RESOURCES_SRCS}
    ${CEFCLIENT_WINDOWS_SRCS}
)

# Executable target.
add_executable(${CEF_TARGET} WIN32 ${CEFCLIENT_SRCS})

target_include_directories(${CEF_TARGET} PRIVATE "..")


SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
add_dependencies(${CEF_TARGET} libcef_dll_wrapper)
target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS} d3d11.lib glu32.lib imm32.lib opengl32.lib)

if(USE_ATL)
    # Required by VS2013 to link accessibility API functions.
    target_link_libraries(${CEF_TARGET} oleacc.lib)
endif()

if(USE_SANDBOX)
    # Logical target used to link the cef_sandbox library.
    ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    target_link_libraries(${CEF_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
endif()

# Add the custom manifest files to the executable.
ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}/resources/win" "${CEF_TARGET}" "exe")

# Copy CEF binary and resource files to the target output directory.
COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
