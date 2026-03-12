cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(KimuVerb VERSION 0.1.0 LANGUAGES CXX)

# Enforce modern C++ and reproducible build flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)                 # Avoid compiler-specific extensions
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type for single-config generators (Ninja)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Configurable parameters (add/override at configure time with -D)
option(ENABLE_SANITIZERS "Enable Address/Undefined sanitizers for Debug builds" OFF)
option(ENABLE_CLANG_TIDY "Run clang-tidy if available" ON)
option(ENABLE_LTO "Enable link time optimization (LTO) when supported" ON)

# Which plugin formats to build (set these as needed)
option(BUILD_VST3 "Build VST3 format" ON)
option(BUILD_AU "Build AU format (macOS only)" ON)
option(BUILD_STANDALONE "Build standalone app" ON)

# Plugin metadata (override with -DPLUGIN_MANUFACTURER_CODE=XXXX)
set(PLUGIN_MANUFACTURER_CODE "Kimu" CACHE STRING "Four-character plugin manufacturer code")
set(PLUGIN_CODE "Kvrb" CACHE STRING "Four-character plugin code")
set(PLUGIN_PRODUCT_NAME "KimuVerb" CACHE STRING "Product name for the plugin")
option(COPY_PLUGIN_AFTER_BUILD "Copy plugin to common host folder after build (JUCE option)" FALSE)
# Build a FORMATS list from the selected options
set(PLUGIN_FORMATS_LIST "")
if(BUILD_VST3)
  list(APPEND PLUGIN_FORMATS_LIST VST3)
endif()
if(BUILD_AU)
  list(APPEND PLUGIN_FORMATS_LIST AU)
endif()
if(BUILD_STANDALONE)
  list(APPEND PLUGIN_FORMATS_LIST Standalone)
endif()
# Fallback to VST3 if nothing selected
if(NOT PLUGIN_FORMATS_LIST)
  list(APPEND PLUGIN_FORMATS_LIST VST3)
endif()

# JUCE settings (must be set before FetchContent_MakeAvailable)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 7.0.9
)
FetchContent_MakeAvailable(juce)

# Platform specific defines
if (WIN32)
  add_compile_definitions(NOMINMAX _USE_MATH_DEFINES)
endif()

# Enable LTO if requested and supported
if(ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
  if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "LTO requested but not supported: ${ipo_error}")
  endif()
endif()

# Create plugin target with configurable formats and metadata
juce_add_plugin(KimuVerb
    COMPANY_NAME "KimuAudio"
    BUNDLE_ID com.kimu.kimuverb
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE ${PLUGIN_MANUFACTURER_CODE}
    PLUGIN_CODE ${PLUGIN_CODE}
    FORMATS ${PLUGIN_FORMATS_LIST}
    PRODUCT_NAME ${PLUGIN_PRODUCT_NAME}
)

juce_generate_juce_header(KimuVerb)

set(SOURCES
  src/PluginProcessor.cpp
  src/PluginEditor.cpp
  src/PluginProcessor.h
  src/PluginEditor.h
)

target_sources(KimuVerb PRIVATE ${SOURCES})

# UI assets compiled into BinaryData for deployment
set(UI_ASSETS
  assets/ui/background/background.png
  "assets/ui/buttons/bypassbutton.png"
  "assets/ui/buttons/leftclick button.png"
  "assets/ui/buttons/powerbutton.png"
  "assets/ui/buttons/rightclickbutton.png"
  "assets/ui/kimuverb logo/kimuverb logo.png"
  "assets/ui/knobstrip/knob 1.png"
  "assets/ui/knobstrip/knob 2.png"
  "assets/ui/knobstrip/knob 3.png"
  "assets/ui/knobstrip/knobstrip example.png"
  assets/ui/orca/orca.png
  "assets/ui/Text Labels/mix textlabel.png"
  "assets/ui/Text Labels/predelay text label.png"
  "assets/ui/Text Labels/Screenshot 2026-03-10 162410.png"
  "assets/ui/Text Labels/size textlabel.png"
  "assets/ui/wave visualizer/wavevisualizer.png"
)

juce_add_binary_data(KimuVerbAssets SOURCES ${UI_ASSETS})

# Recommended compile features and flags
target_compile_features(KimuVerb PRIVATE cxx_std_17)

# Cross-compiler warning settings (use generator expressions)
target_compile_options(KimuVerb PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /Zc:__cplusplus>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion -Wno-unused-parameter>
)

# Optional sanitizers in Debug builds (only for GCC/Clang)
if(ENABLE_SANITIZERS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      target_compile_options(KimuVerb PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(KimuVerb PRIVATE -fsanitize=address,undefined)
    endif()
  else()
    message(WARNING "Sanitizers enabled but not supported for this compiler.")
  endif()
endif()

# Optional clang-tidy integration
if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set_target_properties(KimuVerb PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
  else()
    message(WARNING "clang-tidy requested but not found.")
  endif()
endif()

# Definitions for target
target_compile_definitions(KimuVerb
  PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_DARK_SPLASH_SCREEN=0
)
target_compile_definitions(KimuVerb PRIVATE 
    JUCE_PLUGINHOST_VST2=0 
    JUCE_VST2_CAN_REPLACE_VST3=0 
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_VST2_AAX_WRAPPERS=0
    JUCE_VST2_AU_WRAPPERS=0
    JUCE_VST2_RTAS_WRAPPERS=0
)


# Add the implementation file that you will create
target_sources(KimuVerb PRIVATE src/PluginProcessor_ARA.cpp)

# Link JUCE modules
target_link_libraries(KimuVerb
  PRIVATE
    KimuVerbAssets
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)

# Installer package configuration
# Set package metadata
set(CPACK_PACKAGE_NAME "KimuVerb")
set(CPACK_PACKAGE_VENDOR "KimuAudio")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KimuVerb Reverb Plugin")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Platform-specific packaging
if(WIN32)
  # Windows NSIS installer
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_NSIS_DISPLAY_NAME "KimuVerb Reverb")
  set(CPACK_NSIS_PACKAGE_NAME "KimuVerb")
  set(CPACK_NSIS_CONTACT "support@kimuaudio.com")
  set(CPACK_PACKAGE_FILE_NAME "KimuVerb_v1.0_Setup")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64\\\\KimuAudio\\\\KimuVerb")
  set(CPACK_NSIS_MODIFY_PATH ON)
  
  # Install VST3 and standalone
  if(TARGET KimuVerb_Standalone)
    install(TARGETS KimuVerb_Standalone
      RUNTIME DESTINATION "KimuVerb/Standalone"
    )
  endif()
  if(TARGET KimuVerb_VST3)
    install(DIRECTORY "$<TARGET_FILE_DIR:KimuVerb_VST3>/KimuVerb.vst3"
      DESTINATION "KimuVerb/VST3"
    )
  endif()
  
elseif(APPLE)
  # macOS DMG package
  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_DMG_VOLUME_NAME "KimuVerb")
  set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/installer/dmg_background.png")
  
  # Install VST3, AU, and standalone
  install(TARGETS KimuVerb
    BUNDLE DESTINATION "/Library/Audio/Plug-Ins/VST3"
    RUNTIME DESTINATION "/Applications"
  )
  
endif()

# Include CPack configuration
include(CPack)
