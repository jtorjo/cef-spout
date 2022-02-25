#
# Global setup.
#

cmake_minimum_required(VERSION 2.8.12.1)

# Only generate Debug and Release configuration types.
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name.
# TODO: Change this line to match your project name when you copy this file.
project(cef_spout)

# Use folders in the resulting project files.
set_property(GLOBAL PROPERTY OS_FOLDERS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


#
# Load the CEF configuration.
#

# Execute FindCEF.cmake which must exist in CMAKE_MODULE_PATH.
find_package(CEF REQUIRED)

# we only bother with UNICODE builds on Windows
if(MSVC)
	add_definitions(-DUNICODE -D_UNICODE)
endif()

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_HOME_DIRECTORY}/bin
    )
	
set(
    CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_HOME_DIRECTORY}/lib
    )
	
set(
    TARGET_OUTPUT_DIRECTORY
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>
    )


# Spout-related configuration
file(GLOB spout_src_files "lib_spout/SpoutGL/*.cpp")
file(GLOB spout_inc_files "lib_spout/SpoutGL/*.h")

#
# Define CEF-based targets.
#

# Include the libcef_dll_wrapper target.
# Comes from the libcef_dll/CMakeLists.txt file in the binary distribution
# directory.
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

add_subdirectory(src5)

# Display configuration settings.
PRINT_CEF_CONFIG()
