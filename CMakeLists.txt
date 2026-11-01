cmake_minimum_required(VERSION 3.21)

project(SonkupikStudioNative VERSION 1.1.4 LANGUAGES CXX)

# V1_1_3_SMART_INSTALLER_RC_QUALIFICATION_ANCHOR
# Candidate source version only: v1.1.2 is the accepted immutable public stable.
# PR #183/#184 updater + offline-first fixes and PR #185/#186 Inno scope
# preflight/Smart Installer are included in this future patch. The RC workflow
# must package exact one-commit real-app binaries and keep legacy Machine/
# Per-User updater assets. No stable publication before owner acceptance.
#
# V1_1_2_PATCH_RELEASE_QUALIFICATION_ANCHOR
# v1.1.1 is the first patch release after public v1.1.0. The RC must contain
# the exact current-main preset bank plus the post-v1.1.0 System-toggle UX,
# capture-backed VR OFF fixes, disconnect/offline handoff, semantic preset
# hardening and CI regression closure on one immutable SHA. Stable promotion
# must reuse the accepted RC installer bytes without rebuilding.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Network)
find_package(Git QUIET)
qt_standard_project_setup(REQUIRES 6.8)
# POSIX_K500_TRANSPORT_V1 — build real HID/serial backend only on Mac/Linux.
# Never add QtSerialPort/hidapi to the long-qualified Windows toolchain.
if(NOT WIN32)
    find_package(Qt6 6.8 REQUIRED COMPONENTS SerialPort)
    find_path(K500_HIDAPI_INCLUDE_DIR NAMES hidapi/hidapi.h REQUIRED)
    find_library(K500_HIDAPI_LIBRARY NAMES hidapi-hidraw hidapi REQUIRED)
    set(K500_NATIVE_TRANSPORT_SOURCE src/k500/K500PosixIo.cpp)
else()
    set(K500_NATIVE_TRANSPORT_SOURCE src/k500/K500WinIo.cpp)
endif()


set(SONKUPIK_GIT_SHA "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE SONKUPIK_GIT_RESULT
        OUTPUT_VARIABLE SONKUPIK_GIT_SHA_CANDIDATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(SONKUPIK_GIT_RESULT EQUAL 0 AND NOT SONKUPIK_GIT_SHA_CANDIDATE STREQUAL "")
        set(SONKUPIK_GIT_SHA "${SONKUPIK_GIT_SHA_CANDIDATE}")
    endif()
endif()

qt_add_executable(sonkupik_studio_native WIN32
    src/main.cpp
    src/AppVersionInit.cpp
    src/DonationPromptController.cpp
    src/DonationPromptController.h
    src/StudioEngine.cpp
    src/StudioEngine.h
    src/FxPeqVisualResponse.h
    src/DevicePerformanceMonitor.cpp
    src/DevicePerformanceMonitor.h
    src/RuntimeMetrics.cpp
    src/RuntimeMetrics.h
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
    src/k500/K500CanonicalState.cpp
    src/k500/K500CanonicalState.h
    src/k500/K500TransactionScheduler.cpp
    src/k500/K500TransactionScheduler.h
    src/k500/K500ResponseParser.cpp
    src/k500/K500ResponseParser.h
    ${K500_NATIVE_TRANSPORT_SOURCE}
    src/k500/K500WinIo.h
    src/k500/K500DeviceManager.cpp
    src/k500/K500DeviceManager.h
    src/k500/K500DeviceManagerShutdown.cpp
)
target_compile_definitions(sonkupik_studio_native PRIVATE
    SONKUPIK_VERSION=\"${PROJECT_VERSION}\"
    SONKUPIK_GIT_SHA=\"${SONKUPIK_GIT_SHA}\"
)
target_include_directories(sonkupik_studio_native PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# UPDATE_HANDOFF_P1 — standalone Win32 coordinator with no Qt runtime dependency.
# Its executable is packaged with the app and copied to the update cache
# before replacement so Inno never attempts to overwrite a running helper.
if(WIN32)
    add_executable(k500_update_helper WIN32 packaging/windows/update_helper.cpp)
    target_link_libraries(k500_update_helper PRIVATE shell32 bcrypt user32)
    set_target_properties(k500_update_helper PROPERTIES OUTPUT_NAME "SonKuPik-K500-Updater")
    add_custom_command(TARGET k500_update_helper POST_BUILD
        COMMAND $<TARGET_FILE:k500_update_helper> --self-test
        COMMENT "Running native update coordinator self-test"
    )
endif()



# P0_RUNTIME_TELEMETRY_V1 — deterministic, hardware-free resource qualification.
add_executable(k500_p0_perf_selftest
    src/P0PerfSelfTestMain.cpp
    src/RuntimeMetrics.cpp
    src/RuntimeMetrics.h
    src/StudioEngine.cpp
    src/StudioEngine.h
    src/k500/K500Protocol.cpp
    src/k500/K500Protocol.h
    src/k500/K500Frame.cpp
    src/k500/K500Frame.h
)
target_link_libraries(k500_p0_perf_selftest PRIVATE Qt6::Core)
if(WIN32)
    target_link_libraries(k500_p0_perf_selftest PRIVATE psapi)
endif()

# P1_CANONICAL_DEVICE_STATE_V1 — hardware-free session/state qualification.
add_executable(k500_p1_state_selftest
    src/k500/P1CanonicalStateSelfTestMain.cpp
    src/k500/K500CanonicalState.cpp
    src/k500/K500CanonicalState.h
)
target_include_directories(k500_p1_state_selftest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(k500_p1_state_selftest PRIVATE Qt6::Core)

# P2_DETERMINISTIC_TRANSACTION_SCHEDULER_V1 — bounded/latest-wins host scheduling.
add_executable(k500_p2_scheduler_selftest
    src/k500/P2TransactionSchedulerSelfTestMain.cpp
    src/k500/K500TransactionScheduler.cpp
    src/k500/K500TransactionScheduler.h
)
target_include_directories(k500_p2_scheduler_selftest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(k500_p2_scheduler_selftest PRIVATE Qt6::Core)

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

# P3_WINDOWS_RAII_V1 — mechanical native-resource ownership qualification.
if(WIN32)
    add_executable(k500_p3_win_resource_test
        src/k500/P3WinResourceSelfTestMain.cpp
        src/k500/WinResourceGuards.h
        src/RuntimeMetrics.cpp
        src/RuntimeMetrics.h
    )
    target_include_directories(k500_p3_win_resource_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(k500_p3_win_resource_test PRIVATE Qt6::Core setupapi hid psapi)

    add_executable(k500_p3_transport_shutdown_test
        src/k500/P3TransportShutdownSelfTestMain.cpp
        src/k500/K500WinIo.cpp
        src/k500/K500WinIo.h
        src/k500/K500Frame.cpp
        src/k500/K500Frame.h
        src/k500/WinResourceGuards.h
        src/RuntimeMetrics.cpp
        src/RuntimeMetrics.h
    )
    target_include_directories(k500_p3_transport_shutdown_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(k500_p3_transport_shutdown_test PRIVATE Qt6::Core setupapi hid psapi)
endif()

add_executable(k500_protocol_selftest
    src/k500/ProtocolSelfTestMain.cpp
    src/k500/K500Protocol.cpp
    src/k500/K500Protocol.h
    src/k500/K500Frame.cpp
    src/k500/K500Frame.h
)
target_include_directories(k500_protocol_selftest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/k500")
target_link_libraries(k500_protocol_selftest PRIVATE Qt6::Core)

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
    src/k500/K500Protocol.cpp
    src/k500/K500Protocol.h
    src/k500/K500Frame.cpp
    src/k500/K500Frame.h
)
target_link_libraries(k500_p42_batch_test PRIVATE Qt6::Core Qt6::Network)

# DONATION_PROMPT_CALENDAR_SELF_TEST_V1 — deterministic, hardware-free
# qualification for distinct-day counting and QA preview isolation.
add_executable(k500_donation_prompt_selftest
    src/DonationPromptSelfTestMain.cpp
    src/DonationPromptController.cpp
    src/DonationPromptController.h
)
target_include_directories(k500_donation_prompt_selftest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(k500_donation_prompt_selftest PRIVATE Qt6::Core)
add_custom_command(TARGET k500_donation_prompt_selftest POST_BUILD
    COMMAND $<TARGET_FILE:k500_donation_prompt_selftest>
    COMMENT "Running donation prompt distinct-day self-test"
)

# UPDATE_METADATA_SELF_TEST_V1 — run the production updater manifest/checksum
# parser without network access, installer execution, or K500 hardware.
add_executable(k500_update_selftest
    src/AppUpdateSelfTestMain.cpp
    src/AppUpdateManager.cpp
    src/AppUpdateManagerSelfTest.cpp
    src/AppUpdateManager.h
)
target_include_directories(k500_update_selftest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(k500_update_selftest PRIVATE Qt6::Core Qt6::Network Qt6::Qml)
if(WIN32)
    target_link_libraries(k500_update_selftest PRIVATE shell32 advapi32)
endif()
add_custom_command(TARGET k500_update_selftest POST_BUILD
    COMMAND $<TARGET_FILE:k500_update_selftest>
    COMMENT "Running deterministic updater metadata self-test"
)

set_source_files_properties(
    qml/theme/Theme.qml
    qml/theme/SupportLinks.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(sonkupik_studio_native
    URI SonkupikStudio
    VERSION 1.0
    DEPENDENCIES QtQuick
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/theme/SupportLinks.qml
        qml/components/TopBar.qml
        qml/components/DonationPrompt.qml
        qml/components/AboutDialog.qml
        qml/components/UpdateDialog.qml
        qml/components/SectionDrawer.qml
        qml/components/SoftButton.qml
        qml/components/SystemToggleRow.qml
        qml/components/LucideIcon.qml
        qml/components/StudioComboBox.qml
        qml/components/StudioPanel.qml
        qml/components/StudioKnob.qml
        qml/components/StudioFader.qml
        qml/components/LevelMeter.qml
        qml/components/ChannelStrip.qml
        qml/components/EqGraph.qml
        qml/components/SectionEqGraph.qml
        qml/components/CrossoverCaptionLayout.js
        qml/components/FxPeqVisualResponse.js
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
        qml/components/SystemWorkspaceImpl.qml
        qml/components/OperationFeedback.qml
        qml/components/MassUploadTransferWindow.qml
        qml/components/SectionWorkspace.qml
    SOURCES
        src/AppUpdateManager.cpp
        src/AppUpdateManager.h
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

# DONATION_QRIS_OFFICIAL_ASSET_GATE_V1 — never fabricate a payment QR.
# The app remains fully functional when the official static merchant PNG is
# absent. If the owner supplies the verified image at this exact path, CMake
# embeds it automatically and DonationPromptController enables the prompt.
set(SONKUPIK_QRIS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/support/qris-sonkupik.png")
if(EXISTS "${SONKUPIK_QRIS_FILE}")
    set_source_files_properties("${SONKUPIK_QRIS_FILE}"
        PROPERTIES QT_RESOURCE_ALIAS "support/qris-sonkupik.png")
    qt_add_resources(sonkupik_studio_native donation_qris_asset
        PREFIX "/"
        FILES "${SONKUPIK_QRIS_FILE}"
    )
endif()

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
# GEN5_D2R_PRIMARY_LIBRARY_V1: 01–10 owner-selected D2R family, 11–20 archived GEN3 Bluetooth.
# 41 is an EXACT original D2R GOLDEN binary; never patch its embedded name/checksum.
set(SONKUPIK_PRESET_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/01_ARTIST_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/02_MC_HOST_RADIO_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/03_DANGDUT_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/04_ROCK_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/05_ACOUSTIC_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/06_JAZZ_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/07_MELAYU_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/08_QORI_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/09_ADZAN_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/10_BALLAD_D2R.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/11_ARTIST_GEN3_ARI.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/12_PODCAST_REBORN.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/13_DANGDUT_GEN3_ARI.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/14_KARAOKE_ARTIST.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/15_AKUSTIK_GEN3_ARI.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/16_IMAM_QORI_GEN3.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/17_JAZZ_GEN3_ARI.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/18_ROCK_GEN3_ARI.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/19_MC_CERAMAH_GEN3.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/20_ADZAN_MEKAH_GEN3.k500"
)
# UAUDIO_COMPANION_PRESET_BANK_V1 — move legacy GEN3 companions to 21–30;
# generate from exact archived GEN3 BT 11–20, never from GEN5 D2R.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(SONKUPIK_UAUDIO_PRESET_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated-uaudio-presets")
execute_process(
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_uaudio_presets.py"
        --source "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets"
        --output "${SONKUPIK_UAUDIO_PRESET_DIR}"
    RESULT_VARIABLE SONKUPIK_UAUDIO_RESULT
    OUTPUT_VARIABLE SONKUPIK_UAUDIO_LOG
    ERROR_VARIABLE SONKUPIK_UAUDIO_ERROR
)
if(NOT SONKUPIK_UAUDIO_RESULT EQUAL 0)
    message(FATAL_ERROR "Official UAUDIO preset generation failed: ${SONKUPIK_UAUDIO_ERROR}")
endif()
message(STATUS "${SONKUPIK_UAUDIO_LOG}")
set(SONKUPIK_UAUDIO_PRESET_FILES)
set(SONKUPIK_UAUDIO_INDEX 21)
foreach(PRESET_FILE IN LISTS SONKUPIK_PRESET_FILES)
    get_filename_component(ARCHIVE_STEM "${PRESET_FILE}" NAME_WE)
    if(ARCHIVE_STEM MATCHES "^(1[1-9]|20)_")
        string(REGEX REPLACE "^[0-9][0-9]_" "" BASE_STEM "${ARCHIVE_STEM}")
        list(APPEND SONKUPIK_UAUDIO_PRESET_FILES
            "${SONKUPIK_UAUDIO_PRESET_DIR}/${SONKUPIK_UAUDIO_INDEX}_${BASE_STEM}_UAUDIO.k500")
        math(EXPR SONKUPIK_UAUDIO_INDEX "${SONKUPIK_UAUDIO_INDEX} + 1")
    endif()
endforeach()
if(NOT SONKUPIK_UAUDIO_INDEX EQUAL 31)
    message(FATAL_ERROR "Expected 10 archived GEN3 source files for UAUDIO bank")
endif()
list(APPEND SONKUPIK_PRESET_FILES ${SONKUPIK_UAUDIO_PRESET_FILES})
# Historic SONKUPIK presets 31–40 are exact original bytes; Golden always last.
list(APPEND SONKUPIK_PRESET_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/31_KONSER_NYANYI.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/32_MC_HOST_RADIO.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/33_KAR_DANGDUT.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/34_POP_ROCK_BALLAD.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/35_POP_KENANGAN_V2.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/36_SHOLAWAT_SYAHDU.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/37_JAZZ_LOUNGE.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/38_BLUES_CLUB.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/39_ACOUSTIC_NATURAL.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/40_REGGAE_DUB.k500"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/presets/41_GOLDEN_D2R.k500"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${SONKUPIK_PRESET_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_uaudio_presets.py")

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
    target_link_libraries(sonkupik_studio_native PRIVATE setupapi hid psapi shell32 advapi32 user32)
endif()
if(NOT WIN32)
    target_include_directories(sonkupik_studio_native PRIVATE "${K500_HIDAPI_INCLUDE_DIR}")
    target_link_libraries(sonkupik_studio_native PRIVATE Qt6::SerialPort "${K500_HIDAPI_LIBRARY}")
endif()


# Keep the internal executable name stable for regression harnesses; user-facing
# product/package identity is SonKuPik K500.
set_target_properties(sonkupik_studio_native PROPERTIES
    OUTPUT_NAME "SONKUPIK-STUDIO-Native-UI"
)
if(APPLE)
    set_target_properties(sonkupik_studio_native PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "SonKuPik K500"
        MACOSX_BUNDLE_GUI_IDENTIFIER "audio.sonkupik.k500"
    )
endif()

# Qt's supported QML deployment script embeds Qt plugins and libraries.
# Windows continues to use its exact qualified Per-User Inno setup workflow.
if(NOT WIN32)
    include(GNUInstallDirs)
    install(TARGETS sonkupik_studio_native
        BUNDLE DESTINATION .
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    qt_generate_deploy_qml_app_script(
        TARGET sonkupik_studio_native
        OUTPUT_SCRIPT k500_qml_deploy
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${k500_qml_deploy})
endif()


qt_import_qml_plugins(sonkupik_studio_native)