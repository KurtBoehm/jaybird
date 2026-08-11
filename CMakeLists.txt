# This file is part of https://github.com/KurtBoehm/jaybird.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.20)
project(jaybird LANGUAGES CXX)

# Build options.
option(JAYBIRD_BUILD_TESTS "Build tests" OFF)

include(FetchContent)

FetchContent_Declare(
  thesauros
  GIT_REPOSITORY https://github.com/KurtBoehm/thesauros.git
  GIT_TAG main
)
FetchContent_MakeAvailable(thesauros)

FetchContent_Declare(
  nlohmann_json
  URL
    https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  URL_HASH
    SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  ${PROJECT_NAME}
  INTERFACE
    thesauros::thesauros
    nlohmann_json::nlohmann_json
)

if(JAYBIRD_BUILD_TESTS)
  # Fetch cmop to add functions that add target options.
  FetchContent_Declare(
    cmop
    GIT_REPOSITORY https://github.com/KurtBoehm/cmop.git
    GIT_TAG main
  )
  FetchContent_MakeAvailable(cmop)

  include(CTest)

  set(
    jaybird_tests
    "serialization"
  )

  foreach(test_name ${jaybird_tests})
    set(test_target ${PROJECT_NAME}_${test_name})
    add_executable(${test_target} test/${test_name}.cpp)
    target_compile_features(${test_target} PRIVATE cxx_std_23)
    target_link_libraries(${test_target} PRIVATE ${PROJECT_NAME})
    cmop_add_warnings(${test_target})
    cmop_add_info(${test_target})

    add_test(NAME "${PROJECT_NAME}.${test_name}" COMMAND "${test_target}")
  endforeach()
endif()
