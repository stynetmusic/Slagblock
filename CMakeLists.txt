cmake_minimum_required(VERSION 3.22)
project(Slagblock VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# Путь к JUCE
add_subdirectory($ENV{HOME}/JUCE JUCE)

juce_add_plugin(Slagblock
    COMPANY_NAME "Stynetmusic"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Styn
    PLUGIN_CODE Slgb
    FORMATS VST3
    PRODUCT_NAME "Slagblock"
)

target_sources(Slagblock PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/engine/FMEngine.h
    Source/engine/NeuroFX.h
    Source/engine/ShaperImitation.h
    Source/engine/ChaosMotion.h
)

target_include_directories(Slagblock PRIVATE Source)


target_compile_definitions(Slagblock PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Slagblock PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
