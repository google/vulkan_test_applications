# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

if(ANDROID)
set(ADDITIONAL_LIBS android android_native_app_glue atomic)
elseif(UNIX)
set(ADDITIONAL_LIBS xcb pthread)
endif()

if (NOT OUTPUT_FRAME)
    set(OUTPUT_FRAME -1)
endif()

if (NOT OUTPUT_FILE)
    set(OUTPUT_FILE output.ppm)
endif()

if (NOT DEFAULT_WINDOW_WIDTH)
  set(DEFAULT_WINDOW_WIDTH 100)
endif()

if (NOT DEFAULT_WINDOW_HEIGHT)
  set(DEFAULT_WINDOW_HEIGHT 100)
endif()

SET(DEFAULT_WINDOW_WIDTH ${DEFAULT_WINDOW_WIDTH} CACHE INT
    "Default window width for platforms that have resizable windows")
SET(DEFAULT_WINDOW_HEIGHT ${DEFAULT_WINDOW_HEIGHT} CACHE INT
    "Default window height for platforms that have resizable windows")

SET(OUTPUT_FRAME ${OUTPUT_FRAME} CACHE INT "Default output_frame value.")
SET(OUTPUT_FILE ${OUTPUT_FILE} CACHE STRING "Output file for output_frame.")

option(FIXED_TIMESTEP
    "Should the application run with a fixed timestep (0.1s)" ${FIXED_TIMESTEP})
option(PREFER_SEPARATE_PRESENT
    "Should the application prefer a separate present queue" ${PREFER_SEPARATE_PRESENT})

if (NOT HAVE_CALLBACK_SWAPCHAIN AND NOT OUTPUT_FRAME EQUAL -1)
    message(FATAL_ERROR "Callback swapchain was not build, OUTPUT_FRAME must be -1")
endif()

configure_file(entry_config.h.in entry_config.h)

add_vulkan_static_library(entry
    SOURCES
        entry.cpp
        entry.h
        entry_config.h.in
    LIBS
        ${ADDITIONAL_LIBS}
        logger
        containers)

if (NOT BUILD_APKS)
    target_include_directories(entry PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()
