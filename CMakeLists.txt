cmake_minimum_required(VERSION 3.20)
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
endif()
project(rotator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_library(rotator_model STATIC src/model.cpp src/song_io.cpp src/ed_event_ops.cpp src/ed_matrix_ops.cpp src/ed_hyper_ops.cpp src/recorder.cpp)
target_include_directories(rotator_model PUBLIC src)
target_include_directories(rotator_model SYSTEM PRIVATE vendor/reaper-sdk/WDL)
target_compile_options(rotator_model PRIVATE -Wall -Wextra)

foreach(mod reaper_rotator rotator_core)
  add_library(${mod} MODULE)
  target_include_directories(${mod} PRIVATE src)
  target_include_directories(${mod} SYSTEM PRIVATE vendor/reaper-sdk/sdk vendor/reaper-sdk/WDL)
  target_compile_options(${mod} PRIVATE -Wall -Wextra)
  set_target_properties(${mod} PROPERTIES PREFIX "" SUFFIX ".dylib")
endforeach()

target_sources(rotator_core PRIVATE src/core.cpp src/reaper_out.cpp src/ui.cpp src/ed_event.cpp src/ed_matrix.cpp src/ed_hyper.cpp src/st.cpp)
target_compile_definitions(rotator_core PRIVATE ROTATOR_FONT_DIR="${CMAKE_SOURCE_DIR}/vendor/fonts")
target_include_directories(rotator_core SYSTEM PRIVATE vendor/reaimgui)
target_link_libraries(rotator_core PRIVATE rotator_model)

target_sources(reaper_rotator PRIVATE src/loader.cpp)
target_compile_definitions(reaper_rotator PRIVATE
  ROTATOR_CORE_PATH="$<TARGET_FILE:rotator_core>"
  ROTATOR_BRIDGE_DIR="${CMAKE_SOURCE_DIR}/.bridge")
add_dependencies(reaper_rotator rotator_core)

enable_testing()
add_executable(model_tests tests/model_tests.cpp tests/song_io_tests.cpp tests/ed_event_tests.cpp tests/ed_matrix_tests.cpp tests/ed_hyper_tests.cpp tests/recorder_tests.cpp)
target_link_libraries(model_tests PRIVATE rotator_model)
target_include_directories(model_tests PRIVATE vendor/doctest)
add_test(NAME model_tests COMMAND model_tests)

set(REAPER_USERPLUGINS "$ENV{HOME}/Library/Application Support/REAPER/UserPlugins")
add_custom_target(install_link
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:reaper_rotator> "${REAPER_USERPLUGINS}/reaper_rotator.dylib"
  DEPENDS reaper_rotator)
