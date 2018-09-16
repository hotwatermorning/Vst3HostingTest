cmake_minimum_required(VERSION 3.11)

if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(PROJECT_NAME "MyPortaudioTest")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Werror=return-type")

string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER)
if(${UPPER} STREQUAL "DEBUG")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_NDEBUG")
endif()


include_directories(
  "./ext/portaudio/include"
  "./ext/vst3sdk"
  "./ext/mirror_utf8cpp/source"
  "./src"
  )

file(GLOB_RECURSE FILES "./src/*.cpp" "./src/*.hpp" "./src/*.h")
message("Files ${FILES}")

add_executable(${PROJECT_NAME} ${FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "src/prefix.hpp")
set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")

if(${UPPER} STREQUAL "DEBUG")
  find_library(PORTAUDIO_LIB NAMES "libportaudio.dylib" PATHS "./ext/portaudio/build_debug")
  find_library(VST3_SDK_LIB NAMES "sdk" PATHS "./ext/vst3sdk/build_debug/lib/Debug")
  find_library(VST3_BASE_LIB NAMES "base" PATHS "./ext/vst3sdk/build_debug/lib/Debug")
else()
  find_library(PORTAUDIO_LIB NAMES "libportaudio.dylib" PATHS "./ext/portaudio/build_release")
  find_library(VST3_SDK_LIB NAMES "sdk" PATHS "./ext/vst3sdk/build_release/lib/Release")
  find_library(VST3_BASE_LIB NAMES "base" PATHS "./ext/vst3sdk/build_release/lib/Release")
endif()

target_link_libraries(
  ${PROJECT_NAME}
  ${PORTAUDIO_LIB}
  ${VST3_SDK_LIB}
  ${VST3_BASE_LIB}
  "-framework CoreServices"
  "-framework CoreFoundation"
  "-framework AudioUnit"
  "-framework AudioToolbox"
  "-framework CoreAudio"
  )


