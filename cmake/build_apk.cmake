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

include(CMakeParseArguments)
find_package(PythonInterp)

list(LENGTH CMAKE_CONFIGURATION_TYPES num_elements)
if (num_elements GREATER 1)
  set(IS_MULTICONFIG ON)
  SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

macro(setup_folders target)
  if(IS_MULTICONFIG)
    file(RELATIVE_PATH relative_offset ${VulkanTestApplications_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR})
    set_property(TARGET ${target} PROPERTY FOLDER ${relative_offset})
  endif()
endmacro()

set(CONFIGURABLE_ANDROID_SOURCES
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/build.gradle
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/gradle.properties
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/local.properties
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/settings.gradle
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/build.gradle
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/CMakeLists.txt
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/proguard-rules.pro
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/AndroidManifest.xml
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/res/values/strings.xml)

set(NON_CONFIGURABLE_ANDROID_SOURCES
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/gradlew
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/gradlew.bat
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/gradle/wrapper/gradle-wrapper.jar
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/gradle/wrapper/gradle-wrapper.properties
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/res/mipmap-hdpi/ic_launcher.png
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/res/mipmap-mdpi/ic_launcher.png
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/res/mipmap-xhdpi/ic_launcher.png
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/res/mipmap-xxhdpi/ic_launcher.png
  ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/res/mipmap-xxxhdpi/ic_launcher.png)

set(CMAKE_GLSL_COMPILER "glslc" CACHE STRING "Which glsl compiler to use")

SET(DEFAULT_WINDOW_WIDTH ${DEFAULT_WINDOW_WIDTH} CACHE INT
    "Default window width for platforms that have resizable windows")

if(BUILD_APKS)
  # Create a dummy-target that we can use to gather all of our dependencies
  set(TARGET_SOURCES)
  set(ANDROID_TARGET_NAME ${target})
  set(${target}_SOURCES ${EXE_SOURCES})
  set(${target}_LIBS ${EXE_LIBS})
  set(ANDROID_ADDITIONAL_PARAMS)
  set(APK_BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}/dummyapk/")
  string(REPLACE "\",\"" " " ANDROID_ABIS_SPACES "${ANDROID_ABIS}")

  foreach(source ${CONFIGURABLE_ANDROID_SOURCES})
    file(RELATIVE_PATH rooted_source ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template ${source})
    configure_file(${source}.in ${CMAKE_CURRENT_BINARY_DIR}/dummyapk/${rooted_source} @ONLY)
    list(APPEND TARGET_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummyapk/${rooted_source})
  endforeach()
  foreach(source ${NON_CONFIGURABLE_ANDROID_SOURCES})
    file(RELATIVE_PATH rooted_source ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template ${source})
    configure_file(${source} ${CMAKE_CURRENT_BINARY_DIR}/dummyapk/${rooted_source} COPYONLY)
    list(APPEND TARGET_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummyapk/${rooted_source})
  endforeach()

  set(target_config ${VulkanTestApplications_BINARY_DIR}/.gradle_update)

  # Try and run a dummy build of our dummy program. This does not actually
  # build any artifacts, but does go and grab all of the necessary
  # dependencies to build with gradle. This is the only place that
  # should grab anything from the internet.
  add_custom_command(
      OUTPUT ${target_config}
      COMMENT "Gathering gradle dependencies"
      COMMAND ./gradlew --no-rebuild --gradle-user-home
        ${VulkanTestApplications_BINARY_DIR}/.gradle
      COMMAND ${CMAKE_COMMAND} -E touch ${target_config}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dummyapk
      DEPENDS ${TARGET_SOURCES})
  add_custom_target(gradle_config ALL DEPENDS ${target_config})
endif()

# This recursively gathers all of the dependencies for a target.
macro(gather_deps target)
  if(TARGET ${target})
    get_target_property(SRC ${target} LIB_SRCS)
    get_target_property(LIBS ${target} LIB_DEPS)
    get_target_property(SHADERS ${target} LIB_SHADERS)
    get_target_property(MODELS ${target} LIB_MODELS)
    if (SRC)
      list(APPEND SOURCE_DEPS ${SRC})
    endif()
    if (SHADERS)
      foreach(LIB ${SHADERS})
        gather_deps(${LIB})
      endforeach()
    endif()
    if (MODELS)
      foreach(LIB ${MODELS})
        gather_deps(${LIB})
      endforech()
    endif()
    if (LIBS)
      foreach(LIB ${LIBS})
        gather_deps(${LIB})
      endforeach()
    endif()
  endif()
endmacro()

# Compiles the given GLSL shader through glslc.
function(compile_glsl_using_glslc shader output_file)
  get_filename_component(input_file ${shader} ABSOLUTE)
  add_custom_command (
    OUTPUT ${output_file}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling SPIR-V binary ${shader}"
    DEPENDS ${shader} ${FILE_DEPS}
      ${VulkanTestApplications_SOURCE_DIR}/cmake/generate_cmake_dep.py
    COMMAND ${CMAKE_GLSL_COMPILER} -mfmt=c -o ${output_file} -c ${input_file} -MD
      ${ADDITIONAL_ARGS}
    COMMAND ${PYTHON_EXECUTABLE}
      ${VulkanTestApplications_SOURCE_DIR}/cmake/generate_cmake_dep.py
      ${output_file}.d
    COMMAND ${CMAKE_COMMAND} -E
      copy_if_different
        ${output_file}.d.cmake.tmp
        ${output_file}.d.cmake
  )
endfunction(compile_glsl_using_glslc)

# Compiles the given HLSL shader through glslc.
# The name of the resulting file is also written to the 'result' variable.
function(compile_hlsl_using_glslc shader output_file result)
  get_filename_component(input_file ${shader} ABSOLUTE)
  string(REPLACE ".hlsl." ".glslc.hlsl." glslc_hlsl_filename ${output_file})
  add_custom_command (
    OUTPUT ${glslc_hlsl_filename}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling HLSL to SPIR-V binary using glslc: ${shader}"
    DEPENDS ${shader} ${FILE_DEPS}
      ${VulkanTestApplications_SOURCE_DIR}/cmake/generate_cmake_dep.py
    COMMAND ${CMAKE_GLSL_COMPILER} -mfmt=c -x hlsl -o ${glslc_hlsl_filename} -c ${input_file} -MD
    COMMAND ${PYTHON_EXECUTABLE}
      ${VulkanTestApplications_SOURCE_DIR}/cmake/generate_cmake_dep.py
      ${glslc_hlsl_filename}.d
    COMMAND ${CMAKE_COMMAND} -E
      copy_if_different
        ${glslc_hlsl_filename}.d.cmake.tmp
        ${glslc_hlsl_filename}.d.cmake
  )
  set(${result} ${glslc_hlsl_filename} PARENT_SCOPE)
endfunction(compile_hlsl_using_glslc)

# Compiles the given HLSL shader through DXC.
# The name of the resulting file is also written to the 'result' variable.
function(compile_hlsl_using_dxc shader output_file result)
  get_filename_component(input_file ${shader} ABSOLUTE)
  string(REPLACE ".hlsl." ".dxc.hlsl." dxc_hlsl_filename ${output_file})
  if (NOT CMAKE_DXC_COMPILER)
    # Create an empty .spv file as placeholder so the C++ code compiles.
    file(WRITE ${dxc_hlsl_filename} "{}")
  else()
    # We currently only support fragment shaders and vertex shaders.
    # TODO: Set the DXC target environment for other shader kinds.
    if (${shader} MATCHES ".*\\.frag")
      set(DXC_TARGET_ENV "ps_6_0")
    elseif(${shader} MATCHES ".*\\.vert")
      set(DXC_TARGET_ENV "vs_6_0")
    endif()
  add_custom_command (
    OUTPUT ${dxc_hlsl_filename}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling HLSL to SPIR-V binary using DXC: ${shader}"
    DEPENDS ${shader} ${FILE_DEPS} ${VulkanTestApplications_SOURCE_DIR}/tools/spirv_c_mfmt.py
    COMMAND ${CMAKE_DXC_COMPILER} -spirv -Fo ${dxc_hlsl_filename} -E main -T ${DXC_TARGET_ENV} ${input_file}
    COMMAND ${PYTHON_EXECUTABLE} ${VulkanTestApplications_SOURCE_DIR}/tools/spirv_c_mfmt.py ${dxc_hlsl_filename} --output-file=${dxc_hlsl_filename}
  )
  endif()
  set(${result} ${dxc_hlsl_filename} PARENT_SCOPE)
endfunction(compile_hlsl_using_dxc)

# Android studio generates VERY deep paths. This causes builds on windows to fail,
# because CMake is not set up to handle deep paths.
# If we are building APKS, then shorten all paths
# This is a marco so we can set the variable correctly in the parent scope
macro(add_vulkan_subdirectory dir)

  if (NOT ANDROID OR NOT CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    add_subdirectory(${dir})
  else()
    if (NOT next_sub_index)
      set(next_sub_index 0)
    endif()
    add_subdirectory(${dir} ${next_sub_index})
    math(EXPR next_sub_index "${next_sub_index} + 1")
  endif()
endmacro()

# This adds a vulkan executable (program). By default this just plugs into
# add_executable (Linux/Windows) or add_library (Android). If BUILD_APKS
# is true, then an Android Studio project is created and a target to build it
# is created. All of the dependencies are correctly tracked and the
# project will be rebuilt if any dependency changes.
function(add_vulkan_executable target)
  cmake_parse_arguments(EXE "NON_DEFAULT" "" "SOURCES;LIBS;SHADERS;MODELS;ADDITIONAL;TEXTURES;ADDITIONAL_INCLUDES" ${ARGN})

  if (ANDROID)
    add_library(${target} SHARED ${EXE_SOURCES})
    if (EXE_LIBS)
      target_link_libraries(${target} PRIVATE ${EXE_LIBS})
    endif()
    target_link_libraries(${target} PRIVATE entry)
    mathfu_configure_flags(${target})
     if (EXE_SHADERS)
      foreach (shader ${EXE_SHADERS})
        get_target_property(libdir ${shader} SHADER_LIB_DIR)
        target_include_directories(${target} PRIVATE ${libdir})
        add_dependencies(${target} ${shader})
      endforeach()
      if (EXE_MODELS)
        foreach (model ${EXE_MODELS})
          get_target_property(libdir ${model} MODEL_LIB_DIR)
          target_include_directories(${target} PRIVATE ${libdir})
          add_dependencies(${target} ${model})
        endforeach()
      endif()
      if (EXE_TEXTURES)
        foreach (texture ${EXE_TEXTURES})
          get_target_property(libdir ${texture} TEXTURE_LIB_DIR)
          target_include_directories(${target} PRIVATE ${libdir})
          add_dependencies(${target} ${texture})
        endforeach()
      endif()
    endif()
  elseif (NOT BUILD_APKS)
    set(ADDITIONAL_ARGS)
    if (EXE_NON_DEFAULT)
      list(APPEND ADDITIONAL_ARGS EXCLUDE_FROM_ALL)
    endif()
    add_executable(${target} ${ADDITIONAL_ARGS}
      ${EXE_SOURCES} ${EXE_UNPARSED_ARGS})
    setup_folders(${target})
    target_include_directories(${target} PRIVATE ${VulkanTestApplications_SOURCE_DIR})
    mathfu_configure_flags(${target})
    if (EXE_ADDITIONAL_INCLUDES)
      target_include_directories(${target} PRIVATE ${EXE_ADDITIONAL_INCLUDES})
    endif()
    if (EXE_LIBS)
      target_link_libraries(${target} PRIVATE ${EXE_LIBS})
    endif()
    target_link_libraries(${target} PRIVATE entry)
    if (EXE_MODELS)
      foreach (model ${EXE_MODELS})
        get_target_property(libdir ${model} MODEL_LIB_DIR)
        target_include_directories(${target} PRIVATE ${libdir})
        add_dependencies(${target} ${model})
      endforeach()
    endif()
    if (EXE_TEXTURES)
        foreach (texture ${EXE_TEXTURES})
          get_target_property(libdir ${texture} TEXTURE_LIB_DIR)
          target_include_directories(${target} PRIVATE ${libdir})
          add_dependencies(${target} ${texture})
        endforeach()
    endif()
    if (EXE_SHADERS)
      foreach (shader ${EXE_SHADERS})
        get_target_property(libdir ${shader} SHADER_LIB_DIR)
        target_include_directories(${target} PRIVATE ${libdir})
        add_dependencies(${target} ${shader})
      endforeach()
    endif()
  else()
    set(TARGET_SOURCES)
    set(ANDROID_TARGET_NAME ${target})
    set(${target}_SOURCES ${EXE_SOURCES})
    set(${target}_LIBS ${EXE_LIBS})
    set(${target}_SHADERS ${EXE_SHADERS})
    set(ANDROID_ADDITIONAL_PARAMS)
    set(APK_BUILD_ROOT "${VulkanTestApplications_BINARY_DIR}/${target}-apk/")
    string(REPLACE "\",\"" " " ANDROID_ABIS_SPACES "${ANDROID_ABIS}")

    if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
      set(ANDROID_ADDITIONAL_PARAMS "android:debuggable=\"true\"")
      list(APPEND CONFIGURABLE_ANDROID_SOURCES
        ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/jni/Android.mk
        ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template/app/src/main/gdb-path.txt)
    endif()

    foreach(source ${CONFIGURABLE_ANDROID_SOURCES})
      file(RELATIVE_PATH rooted_source ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template ${source})
      configure_file(${source}.in ${APK_BUILD_ROOT}/${rooted_source} @ONLY)
      list(APPEND TARGET_SOURCES ${APK_BUILD_ROOT}/${rooted_source})
    endforeach()
    foreach(source ${NON_CONFIGURABLE_ANDROID_SOURCES})
      file(RELATIVE_PATH rooted_source ${VulkanTestApplications_SOURCE_DIR}/cmake/android_project_template ${source})
      configure_file(${source} ${APK_BUILD_ROOT}/${rooted_source} COPYONLY)
      list(APPEND TARGET_SOURCES ${APK_BUILD_ROOT}/${rooted_source})
    endforeach()

    if (CMAKE_BUILD_TYPE STREQUAL Debug)
      set(apk_build_location "${APK_BUILD_ROOT}/app/build/outputs/apk/app-debug.apk")
      set(ASSEMBLE_COMMAND assembleDebug)
    else()
      set(apk_build_location "${APK_BUILD_ROOT}/app/build/outputs/apk/app-release-unsigned.apk")
      set(ASSEMBLE_COMMAND assembleRelease)
    endif()

    set(target_apk ${VulkanTestApplications_BINARY_DIR}/apk/${target}.apk)

    add_custom_target(${target}_sources)
    set(ABSOLUTE_SOURCES)
    foreach(SOURCE ${EXE_SOURCES})
      get_filename_component(TEMP ${SOURCE} ABSOLUTE)
      list(APPEND ABSOLUTE_SOURCES ${TEMP})
    endforeach()
    set_target_properties(${target}_sources PROPERTIES LIB_SRCS "${ABSOLUTE_SOURCES}")
    set_target_properties(${target}_sources PROPERTIES LIB_DEPS "${EXE_LIBS}")
    set_target_properties(${target}_sources PROPERTIES LIB_SHADERS "${EXE_SHADERS}")
    set_target_properties(${target}_sources PROPERTIES LIB_MODELS "${EXE_MODELS}")


    set(SOURCE_DEPS)
    gather_deps(${target}_sources)
    gather_deps(entry)

    # Try not to pollute the user's root gradle directory.
    # Force our version of gradle to use a gradle cache directory
    #   that is local to this build.

    add_custom_command(
      OUTPUT ${target_apk}
      COMMAND ./gradlew --offline ${ASSEMBLE_COMMAND} --gradle-user-home
        ${VulkanTestApplications_BINARY_DIR}/.gradle
      COMMAND ${CMAKE_COMMAND} -E copy ${apk_build_location} ${target_apk}
      WORKING_DIRECTORY ${APK_BUILD_ROOT}
      DEPENDS ${SOURCE_DEPS}
              ${TARGET_SOURCES}
              ${MATHFU_HEADERS}
              gradle_config)
    set(TARGET_DEP ALL)
    if (EXE_NON_DEFAULT)
      set(TARGET_DEP)
    endif()
    add_custom_target(${target} ${TARGET_DEP} DEPENDS ${target_apk})
  endif()
endfunction(add_vulkan_executable)


# This adds a library. By default this just plugs into
# add_library. If BUILD_APKS is true then a custom target is created
# and all of the sources are added to it. When an executable
# (from above) uses the library, the sources treated AS
# dependencies.
function(_add_vulkan_library target)
  cmake_parse_arguments(LIB "" "TYPE" "SOURCES;LIBS;SHADERS" ${ARGN})

  if (BUILD_APKS)
    add_custom_target(${target})
    set(ABSOLUTE_SOURCES)
    foreach(SOURCE ${LIB_SOURCES})
      get_filename_component(TEMP ${SOURCE} ABSOLUTE)
      list(APPEND ABSOLUTE_SOURCES ${TEMP})
    endforeach()

    set_target_properties(${target} PROPERTIES LIB_SRCS "${ABSOLUTE_SOURCES}")
    set_target_properties(${target} PROPERTIES LIB_DEPS "${LIB_LIBS}")
  else()
    add_library(${target} ${LIB_TYPE} ${LIB_SOURCES})
    setup_folders(${target})
    mathfu_configure_flags(${target})
    target_link_libraries(${target} PRIVATE ${LIB_LIBS})
    if (EXE_SHADERS)
      foreach (shader ${EXE_SHADERS})
        get_target_property(libdir ${shader} SHADER_LIB_DIR)
        target_include_directories(${target} PRIVATE ${libdir})
        add_dependencies(${target} ${shader})
      endforeach()
    endif()
  endif()
endfunction()

# Helper function to add a static libray
function(add_vulkan_static_library target)
  _add_vulkan_library(${target} TYPE STATIC ${ARGN})
endfunction(add_vulkan_static_library)

# Helper function to add a shared libray
function(add_vulkan_shared_library target)
  _add_vulkan_library(${target} TYPE SHARED ${ARGN})
endfunction(add_vulkan_shared_library)

function(add_model_library target)
  cmake_parse_arguments(LIB "" "TYPE" "SOURCES" ${ARGN})
  if (BUILD_APKS)
    add_custom_target(${target})
    set(ABSOLUTE_SOURCES)
    foreach(SOURCE ${LIB_SOURCES})
      get_filename_component(TEMP ${SOURCE} ABSOLUTE)
      list(APPEND ABSOLUTE_SOURCES ${TEMP})
    endforeach()
    set_target_properties(${target} PROPERTIES LIB_SRCS "${ABSOLUTE_SOURCES}")
    set_target_properties(${target} PROPERTIES LIB_DEPS "")
  else()
    set(output_files)
    foreach(model ${LIB_SOURCES})
      get_filename_component(model ${model} ABSOLUTE)
      file(RELATIVE_PATH rel_pos ${CMAKE_CURRENT_SOURCE_DIR} ${model})
      set(output_file ${CMAKE_CURRENT_BINARY_DIR}/${rel_pos}.h)
      get_filename_component(output_file ${output_file} ABSOLUTE)
      list(APPEND output_files ${output_file})

      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${rel_pos}.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling Model ${model}"
        DEPENDS ${model}
          ${VulkanTestApplications_SOURCE_DIR}/cmake/convert_obj_to_c.py
        COMMAND ${PYTHON_EXECUTABLE}
          ${VulkanTestApplications_SOURCE_DIR}/cmake/convert_obj_to_c.py
            ${model} -o ${output_file}
      )
    endforeach()
    add_custom_target(${target}
      DEPENDS ${output_files})
    setup_folders(${target})
    set_target_properties(${target} PROPERTIES MODEL_OUT_FILES
      "${output_files}")
    set_target_properties(${target} PROPERTIES MODEL_LIB_DIR
      "${CMAKE_CURRENT_BINARY_DIR}")
  endif()
endfunction()

function(add_shader_library target)
  cmake_parse_arguments(LIB "" "" "SOURCES;SHADER_DEPS" ${ARGN})
  if (BUILD_APKS)
    add_custom_target(${target})
    set(ABSOLUTE_SOURCES)
    foreach(SOURCE ${LIB_SOURCES})
      get_filename_component(TEMP ${SOURCE} ABSOLUTE)
      list(APPEND ABSOLUTE_SOURCES ${TEMP})
    endforeach()
    set_target_properties(${target} PROPERTIES LIB_SRCS "${ABSOLUTE_SOURCES}")
    if (LIB_SHADER_DEPS)
      foreach(SHADER_DEP ${LIB_SHADER_DEPS})
        get_filename_component(TEMP ${SHADER_DEP} ABSOLUTE)
        set_target_properties(${target} PROPERTIES LIB_DEPS "${TEMP}")
      endforeach()
    else()
      set_target_properties(${target} PROPERTIES LIB_DEPS "")
    endif()
  else()
    set(output_files)
    foreach(shader ${LIB_SOURCES})
      get_filename_component(suffix ${shader} EXT)
      if (${suffix} MATCHES "\\.vert|\\.frag|\\.geom|\\.comp|\\.tesc|\\.tese")
        get_filename_component(temp ${shader} ABSOLUTE)
        file(RELATIVE_PATH rel_pos ${CMAKE_CURRENT_SOURCE_DIR} ${temp})
        set(output_file ${CMAKE_CURRENT_BINARY_DIR}/${rel_pos}.spv)
        get_filename_component(output_file ${output_file} ABSOLUTE)


        if (NOT EXISTS ${output_file}.d.cmake)
            execute_process(
              COMMAND
                ${CMAKE_COMMAND} -E copy
                ${VulkanTestApplications_SOURCE_DIR}/cmake/empty_cmake_dep.in.cmake
                ${output_file}.d.cmake)
        endif()

        include(${output_file}.d.cmake)

        set(ADDITIONAL_ARGS "")
        if (LIB_SHADER_DEPS)
          foreach(DEP ${LIB_SHADER_DEPS})
            if(NOT TARGET ${DEP})
              message(FATAL_ERROR "Could not find dependent shader library ${DEP}")
            endif()
            get_target_property(SRC_DIR ${DEP} SHADER_SOURCE_DIR)
            if(NOT SRC_DIR)
              message(FATAL_ERROR "Could not get shader source dir for ${DEP}")
            endif()
            list(APPEND ADDITIONAL_ARGS "-I${SRC_DIR}")
          endforeach()
        endif()

        get_filename_component(shader_name ${shader} NAME)

        if (${shader_name} MATCHES ".*\\.hlsl\\..*")
          # If an HLSL shader is found, compile it using glslc and DXC
          compile_hlsl_using_dxc(${shader} ${output_file} result)
          list(APPEND output_files ${result})
          compile_hlsl_using_glslc(${shader} ${output_file} result)
          list(APPEND output_files ${result})
        else()
          # Compile GLSL shaders through glslc
          compile_glsl_using_glslc(${shader} ${output_file})
          list(APPEND output_files ${output_file})
        endif()
      endif()
    endforeach()
    add_custom_target(${target}
      DEPENDS ${output_files})
    setup_folders(${target})
    set_target_properties(${target} PROPERTIES SHADER_OUT_FILES
      "${output_files}")
    set_target_properties(${target} PROPERTIES SHADER_LIB_DIR
      "${CMAKE_CURRENT_BINARY_DIR}")
    get_filename_component(TEMP ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
    set_target_properties(${target} PROPERTIES SHADER_SOURCE_DIR
      "${TEMP}")
  endif()
endfunction()

function(add_texture_library target)
  cmake_parse_arguments(LIB "" "TYPE" "SOURCES" ${ARGN})
  if (BUILD_APKS)
    add_custom_target(${target})
    set(ABSOLUTE_SOURCES)
    foreach(SOURCE ${LIB_SOURCES})
      get_filename_component(TEMP ${SOURCE} ABSOLUTE)
      list(APPEND ABSOLUTE_SOURCES ${TEMP})
    endforeach()
    set_target_properties(${target} PROPERTIES LIB_SRCS "${ABSOLUTE_SOURCES}")
    set_target_properties(${target} PROPERTIES LIB_DEPS "")
  else()
    set(output_files)
    foreach(texture ${LIB_SOURCES})
      get_filename_component(texture ${texture} ABSOLUTE)
      file(RELATIVE_PATH rel_pos ${CMAKE_CURRENT_SOURCE_DIR} ${texture})
      set(output_file ${CMAKE_CURRENT_BINARY_DIR}/${rel_pos}.h)
      get_filename_component(output_file ${output_file} ABSOLUTE)
      list(APPEND output_files ${output_file})

      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${rel_pos}.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling texture ${texture}"
        DEPENDS ${texture}
          ${VulkanTestApplications_SOURCE_DIR}/cmake/convert_img_to_c.py
        COMMAND ${PYTHON_EXECUTABLE}
          ${VulkanTestApplications_SOURCE_DIR}/cmake/convert_img_to_c.py
            ${texture} -o ${output_file}
      )
    endforeach()
    add_custom_target(${target}
      DEPENDS ${output_files})
    setup_folders(${target})
    set_target_properties(${target} PROPERTIES TEXTURE_OUT_FILES
      "${output_files}")
    set_target_properties(${target} PROPERTIES TEXTURE_LIB_DIR
      "${CMAKE_CURRENT_BINARY_DIR}")
  endif()
endfunction()
