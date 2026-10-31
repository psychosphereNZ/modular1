cmake_minimum_required(VERSION 3.22)
project(TrinityModular VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND PLUGIN_FORMATS AU)
endif()

juce_add_plugin(TrinityModular
  COMPANY_NAME "G"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  PLUGIN_MANUFACTURER_CODE Gprd
  PLUGIN_CODE Trin
  FORMATS ${PLUGIN_FORMATS}
  PRODUCT_NAME "TrinityModular")

file(GLOB PLUGIN_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
target_sources(TrinityModular PRIVATE ${PLUGIN_SOURCES})
target_include_directories(TrinityModular PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Source
  ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(TrinityModular PRIVATE cxx_std_17)
target_compile_definitions(TrinityModular PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(TrinityModular
  PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
