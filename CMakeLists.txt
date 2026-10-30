cmake_minimum_required(VERSION 3.22)
project(QuantizerPro VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(QuantizerPro
    COMPANY_NAME "ORYMY"
    PRODUCT_NAME "Quantizer Pro"
    PLUGIN_MANUFACTURER_CODE Orym
    PLUGIN_CODE Qprx
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png")

juce_add_binary_data(QuantizerAssets SOURCES
    assets/center.png
    assets/logo.png
    assets/sign.png
    assets/icon.png)

target_sources(QuantizerPro PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/QuantEngine.cpp)

target_compile_definitions(QuantizerPro PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(QuantizerPro
    PRIVATE
        QuantizerAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
