cmake_minimum_required(VERSION 3.21)

project(SonkupikStudioNative VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Network)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(sonkupik_studio_native WIN32
    src/main.cpp
    src/AppVersionInit.cpp
    src/StudioEngine.cpp
    src/StudioEngine.h
    src/k500/K500Frame.cpp
    src/k500/K500Frame.h
    src/k500/K500Protocol.cpp
    src/k500/K500Protocol.h
    src/k500/K500PresetProtocol.cpp
    src/k500/K500PresetProtocol.h
    src/k500/K500PresetCodec.cpp
    src/k500/K500PresetCodec.h
    src/k500/K500PresetEditMapper.cpp
    src/k500/K500PresetEditMapper.h
    src/k500/K500PresetFileBridge.cpp
    src/k500/K500PresetFileBridge.h
    src/k500/K500PresetFileBridgeAccess.cpp
    src/k500/K500OfficialPresetSync.cpp
    src/k500/K500PresetBatchBridge.cpp
    src/k500/K500PresetManager.cpp
    src/k500/K500PresetManager.h
    src/k500/K500PresetManagerBridge.cpp
    src/k500/K500PresetUploadBridge.cpp
    src/k500/K500Controller.cpp
    src/k500/K500Controller.h
    src/k500/K500ResponseParser.cpp
    src/k500/K500ResponseParser.h
    src/k500/K500WinIo.cpp
    src/k500/K500WinIo.h
    src/k500/K500DeviceManager.cpp
    src/k500/K500DeviceManager.h
)
target_compile_definitions(sonkupik_studio_native PRIVATE SONKUPIK_VERSION=\"${PROJECT_VERSION}\")
target_include_directories(sonkupik_studio_native PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# P0_RUNTIME_TELEMETRY_V1 — deterministic, hardware-free resource qualification.
add_executable(k500_p0_perf_selftest
    src/P0PerfSelfTestMain.cpp
    src/RuntimeMetrics.cpp
    src/RuntimeMetrics.h
    src/StudioEngine.cpp
    src/StudioEngine.h
)
target_link_libraries(k500_p0_perf_selftest PRIVATE Qt6::Core)
if(WIN32)
    target_link_libraries(k500_p0_perf_selftest PRIVATE psapi)
endif()

# P2_TRANSPORT_LIFECYCLE_STRESS_V1 — hardware-free worker lifecycle qualification.
if(WIN32)
    add_executable(k500_p2_transport_lifecycle_test
        src/k500/P2TransportLifecycleSelfTestMain.cpp
        src/k500/K500WinIo.cpp
        src/k500/K500WinIo.h
        src/k500/K500Frame.cpp
        src/k500/K500Frame.h
        src/RuntimeMetrics.cpp
        src/RuntimeMetrics.h
    )
    target_include_directories(k500_p2_transport_lifecycle_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(k500_p2_transport_lifecycle_test PRIVATE Qt6::Core setupapi hid psapi)
endif()

add_executable(k500_p2_selftest
    src/k500/P2SelfTestMain.cpp
    src/k500/K500Frame.cpp
    src/k500/K500Frame.h
    src/k500/K500PresetProtocol.cpp
    src/k500/K500PresetProtocol.h
)
target_link_libraries(k500_p2_selftest PRIVATE Qt6::Core)

add_executable(k500_p3_selftest
    src/k500/P3SelfTestMain.cpp
    src/k500/K500PresetCodec.cpp
    src/k500/K500PresetCodec.h
)
target_link_libraries(k500_p3_selftest PRIVATE Qt6::Core)

add_executable(k500_p32_corpus_test
    src/k500/P32CorpusTestMain.cpp
    src/k500/K500PresetCodec.cpp
    src/k500/K500PresetCodec.h
)
target_link_libraries(k500_p32_corpus_test PRIVATE Qt6::Core)

add_executable(k500_p34_edit_persistence_test
    src/k500/P34EditPersistenceTestMain.cpp
    src/k500/K500PresetEditMapper.cpp
    src/k500/K500PresetEditMapper.h
    src/k500/K500PresetCodec.cpp
    src/k500/K500PresetCodec.h
)
target_link_libraries(k500_p34_edit_persistence_test PRIVATE Qt6::Core)

add_executable(k500_p42_batch_test
    src/k500/P42BatchTestMain.cpp
    src/k500/K500PresetBatchBridge.cpp
    src/k500/K500PresetFileBridge.cpp
    src/k500/K500PresetFileBridge.h
    src/k500/K500OfficialPresetSync.cpp
    src/k500/K500PresetEditMapper.cpp
    src/k500/K500PresetEditMapper.h
    src/k500/K500PresetCodec.cpp
    src/k500/K500PresetCodec.h
    src/StudioEngine.cpp
    src/StudioEngine.h
)
target_link_libraries(k500_p42_batch_test PRIVATE Qt6::Core Qt6::Network)

set_source_files_properties(qml/theme/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(sonkupik_studio_native
    URI SonkupikStudio
    VERSION 1.0
    DEPENDENCIES QtQuick
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/components/TopBar.qml
        qml/components/SectionDrawer.qml
        qml/components/SoftButton.qml
        qml/components/LucideIcon.qml
        qml/components/StudioComboBox.qml
        qml/components/StudioPanel.qml
        qml/components/StudioKnob.qml
        qml/components/StudioFader.qml
        qml/components/LevelMeter.qml
        qml/components/ChannelStrip.qml
        qml/components/EqGraph.qml
        qml/components/SectionEqGraph.qml
        qml/components/SectionEqGraphHost.qml
        qml/components/LocalEqModel.qml
        qml/components/CompressorGraph.qml
        qml/components/KeyControl.qml
        qml/components/MusicControlPanel.qml
        qml/components/ValueField.qml
        qml/components/ParameterSlider.qml
        qml/components/InputFader.qml
        qml/components/PresetPanel.qml
        qml/components/BandInspector.qml
        qml/components/CrossoverInspector.qml
        qml/components/MusicInputPanel.qml
        qml/components/MusicTonePanel.qml
        qml/components/FilterPanel.qml
        qml/components/MasterStripPanel.qml
        qml/components/RackFaderPanel.qml
        qml/components/RackDynamicsPanel.qml
        qml/components/RackFilterPanel.qml
        qml/components/SystemWorkspace.qml
        qml/components/MassUploadTransferWindow.qml
        qml/components/SectionWorkspace.qml
    SOURCES
        src/EqCurveItem.cpp
        src/EqCurveItem.h
)

# BRAND_SONKUPIK_K500_V1 — one authoritative visual identity for app/header/web.
set(SONKUPIK_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/assets/SonKuPik-k500-logo.png")
set_source_files_properties("${SONKUPIK_LOGO}" PROPERTIES QT_RESOURCE_ALIAS "assets/SonKuPik-k500-logo.png")
qt_add_resources(sonkupik_studio_native brand_assets
    PREFIX "/"
    FILES "${SONKUPIK_LOGO}"
)

set(SONKUPIK_FONT_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/PlusJakartaSans-Regular.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/PlusJakartaSans-Medium.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/PlusJakartaSans-SemiBold.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/PlusJakartaSans-Bold.ttf"
)
foreach(FONT_FILE IN LISTS SONKUPIK_FONT_FILES)
    get_filename_component(FONT_NAME "${FONT_FILE}" NAME)
    set_source_files_properties("${FONT_FILE}" PROPERTIES QT_RESOURCE_ALIAS "fonts/${FONT_NAME}")
endforeach()
qt_add_resources(sonkupik_studio_native embedded_fonts
    PREFIX "/"
    FILES ${SONKUPIK_FONT_FILES}
)

# Bundled official presets are the offline fallback. Runtime GitHub sync may
# replace/add validated files in the user cache, never mutate these resources.
set(SONKUPIK_PRESET_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/01_ALL_GENRE.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/02_BROADCAST.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/03_DANGDUT_SUPREME.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/04_ROCK.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/05_POP_KENANGAN.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/06_QORI_SHOLAWAT.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/07_JAZZ.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/08_BLUES.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/09_ACOUSTIC.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/10_REGGAE.k500"
)
foreach(PRESET_FILE IN LISTS SONKUPIK_PRESET_FILES)
    get_filename_component(PRESET_NAME "${PRESET_FILE}" NAME)
    set_source_files_properties("${PRESET_FILE}" PROPERTIES QT_RESOURCE_ALIAS "presets/${PRESET_NAME}")
endforeach()
qt_add_resources(sonkupik_studio_native built_in_presets
    PREFIX "/"
    FILES ${SONKUPIK_PRESET_FILES}
)

target_link_libraries(sonkupik_studio_native
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Network
)

if(WIN32)
    target_link_libraries(sonkupik_studio_native PRIVATE setupapi hid)
endif()

# Keep the internal executable name stable for regression harnesses; user-facing
# product/package identity is SonKuPik K500.
set_target_properties(sonkupik_studio_native PROPERTIES
    OUTPUT_NAME "SONKUPIK-STUDIO-Native-UI"
)

qt_import_qml_plugins(sonkupik_studio_native)