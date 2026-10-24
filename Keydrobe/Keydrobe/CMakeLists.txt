cmake_minimum_required(VERSION 3.22)
project(Keydrobe VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KEYDROBE_COPY_AFTER_BUILD "Copy the plugin to the system plugin folder after building" ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Keydrobe
    COMPANY_NAME "Keydrobe Atelier"
    PLUGIN_MANUFACTURER_CODE Kyat
    PLUGIN_CODE Kdrb
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${KEYDROBE_COPY_AFTER_BUILD}
    FORMATS VST3 Standalone
    PRODUCT_NAME "Keydrobe")

target_sources(Keydrobe PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Keydrobe PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Keydrobe
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
