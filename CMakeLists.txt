cmake_minimum_required(VERSION 3.15.0)

project(pjson VERSION 0.0.1 LANGUAGES C)

include(FetchContent)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type was specified. Setting build type to 'Debug'.")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

macro(configure_compiler TARGET_NAME)
  target_compile_features(${TARGET_NAME} PUBLIC c_std_99)
  if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
  else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wunused-value)
  endif()
endmacro()

# Project-wide configuration
include_directories("${PROJECT_SOURCE_DIR}/src")

# Library
set(PJSON_LIB_SOURCES src/pjson.c src/pjson.h src/pjson_config.h)
add_library(pjson STATIC ${PJSON_LIB_SOURCES})
configure_compiler(pjson)

# Build sample and test only when not used as a dependency
# (see also: https://www.foonathan.net/2022/06/cmake-fetchcontent/)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # Sample
  file (GLOB PJSON_SAMPLE_SOURCES sample/*.c sample/*.h)
  add_executable(pjson_sample ${PJSON_SAMPLE_SOURCES})
  configure_compiler(pjson_sample)
  set_target_properties(pjson_sample PROPERTIES OUTPUT_NAME "pjson")

  target_include_directories(pjson_sample PRIVATE sample/ shared/)

  target_link_libraries(pjson_sample PRIVATE
    pjson
  )

  # Test
  include(CTest)

  option(PJSON_ENABLE_CODE_COVERAGE "Enable code coverage for tests" OFF)

  if(BUILD_TESTING)
    FetchContent_Declare(
      unity
      GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity
      GIT_TAG        v2.6.0
    )

    set(UNITY_EXTENSION_FIXTURE ON CACHE INTERNAL "Compiles Unity with the \"fixture\" extension.")

    FetchContent_MakeAvailable(unity)

    enable_testing()

    file (GLOB PJSON_TEST_SOURCES test/*.c test/*.h)
    add_executable(pjson_test ${PJSON_TEST_SOURCES})
    configure_compiler(pjson_test)

    target_include_directories(pjson_test PRIVATE test/ shared/)
    target_compile_definitions(pjson_test PRIVATE PJSON_CONFIG_H="pjson_config_test.h")

    target_link_libraries(pjson_test PRIVATE
      unity
      pjson
    )

    if(PJSON_ENABLE_CODE_COVERAGE)
      include(cmake/CodeCoverage.cmake)

      if(NOT LCOV_PATH)
        message(FATAL_ERROR "lcov not found! Aborting...")
      endif()

      message(STATUS "Code coverage support is enabled.")
      append_coverage_compiler_flags()
      setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE pjson_test
        EXCLUDE
          "${PROJECT_SOURCE_DIR}/samples/*"
          "${PROJECT_SOURCE_DIR}/shared/*"
          "${PROJECT_SOURCE_DIR}/test/*"
      )
    else()
      message(STATUS "Code coverage support is disabled.")
    endif()

    add_test(NAME tests COMMAND pjson_test)
  endif()
endif()
