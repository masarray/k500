cmake_minimum_required(VERSION 3.21)

project(K500P4Sanitizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core)

# P3_SHUTDOWN_COMPILE_PROBE_V1
# P4 is stacked directly on P3, so this small object target gives fast compile
# coverage for the final P3 aboutToQuit/shutdown translation unit while the full
# packaging matrix runs independently.
add_library(k500_p3_shutdown_compile_probe OBJECT
    ../../src/k500/K500DeviceManagerShutdown.cpp
    ../../src/k500/K500DeviceManager.h
    ../../src/k500/K500WinIo.h
    ../../src/k500/K500Controller.h
)
target_include_directories(k500_p3_shutdown_compile_probe PRIVATE ../../src ../../src/k500)
target_link_libraries(k500_p3_shutdown_compile_probe PRIVATE Qt6::Core)

# P4_MSVC_ASAN_V1
# Sanitizer instrumentation stays isolated from the shipping executable and Qt
# binaries. These targets instrument first-party parser/codec/transport paths.
add_executable(k500_p4_parser_fuzz
    ../../src/k500/P4ParserFuzzSelfTestMain.cpp
    ../../src/k500/K500ResponseParser.cpp
    ../../src/k500/K500ResponseParser.h
    ../../src/k500/K500Frame.cpp
    ../../src/k500/K500Frame.h
)
target_include_directories(k500_p4_parser_fuzz PRIVATE ../../src/k500)
target_link_libraries(k500_p4_parser_fuzz PRIVATE Qt6::Core)

add_executable(k500_p4_preset_codec_fuzz
    ../../src/k500/P4PresetCodecFuzzSelfTestMain.cpp
    ../../src/k500/K500PresetCodec.cpp
    ../../src/k500/K500PresetCodec.h
)
target_include_directories(k500_p4_preset_codec_fuzz PRIVATE ../../src/k500)
target_link_libraries(k500_p4_preset_codec_fuzz PRIVATE Qt6::Core)

if(WIN32)
    add_executable(k500_p4_lifecycle_asan
        ../../src/k500/P4LifecycleAsanSelfTestMain.cpp
        ../../src/k500/K500WinIo.cpp
        ../../src/k500/K500WinIo.h
        ../../src/k500/K500Frame.cpp
        ../../src/k500/K500Frame.h
        ../../src/k500/WinResourceGuards.h
    )
    target_include_directories(k500_p4_lifecycle_asan PRIVATE ../../src ../../src/k500)
    target_link_libraries(k500_p4_lifecycle_asan PRIVATE Qt6::Core setupapi hid)
endif()

set(P4_ASAN_TARGETS
    k500_p4_parser_fuzz
    k500_p4_preset_codec_fuzz
)
if(WIN32)
    list(APPEND P4_ASAN_TARGETS k500_p4_lifecycle_asan)
endif()

if(MSVC)
    foreach(target IN LISTS P4_ASAN_TARGETS)
        target_compile_options(${target} PRIVATE /fsanitize=address /Zi)
        target_link_options(${target} PRIVATE /fsanitize=address)
    endforeach()
endif()
