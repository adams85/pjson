cmake_minimum_required(VERSION 3.15.0)
project(pjson VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

macro(configure_compiler TARGET_NAME)
  target_compile_features(${TARGET_NAME} PUBLIC c_std_99)
  if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
  else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wunused-value)
  endif()
endmacro()

# Library

set(PJSON_LIB_SOURCES src/pjson.c src/pjson.h src/pjson_config.h)
add_library(pjson STATIC ${PJSON_LIB_SOURCES})
configure_compiler(pjson)

# Sample

file (GLOB PJSON_SAMPLE_SOURCES sample/*.c)
add_executable(pjson_sample ${PJSON_SAMPLE_SOURCES})
configure_compiler(pjson_sample)
set_target_properties(pjson_sample PROPERTIES OUTPUT_NAME "pjson")

target_include_directories(pjson_sample PRIVATE src/ sample/)

target_link_libraries(pjson_sample
  pjson
)
