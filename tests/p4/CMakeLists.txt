cmake_minimum_required(VERSION 3.21)

project(K500P4Sanitizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core)

# P4_MSVC_ASAN_V1
# Keep sanitizer instrumentation isolated from the shipping executable and Qt
# binaries. This target instruments only first-party parser/frame/harness code.
add_executable(k500_p4_parser_fuzz
    ../../src/k500/P4ParserFuzzSelfTestMain.cpp
    ../../src/k500/K500ResponseParser.cpp
    ../../src/k500/K500ResponseParser.h
    ../../src/k500/K500Frame.cpp
    ../../src/k500/K500Frame.h
)

target_include_directories(k500_p4_parser_fuzz PRIVATE ../../src/k500)
target_link_libraries(k500_p4_parser_fuzz PRIVATE Qt6::Core)

if(MSVC)
    target_compile_options(k500_p4_parser_fuzz PRIVATE /fsanitize=address /Zi)
    target_link_options(k500_p4_parser_fuzz PRIVATE /fsanitize=address)
endif()
