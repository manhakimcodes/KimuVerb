# KimuVerb

JUCE-based VST3 reverb plugin using CMake. UI features an orca silhouette with body-part knob placement.

## Requirements
- Windows 10/11
- CMake 3.22+
- Visual Studio 2022 with Desktop C++
- Git

## Build
PowerShell:
```
mkdir build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Notes:
- JUCE is fetched automatically via FetchContent; internet is required on first configure.
- VST3 SDK is bundled in JUCE; no separate install needed.

The VST3 will be copied to your default JUCE plugin folder if available; otherwise find it under `build/KimuVerb_artefacts/Release/VST3/`.

## Run
Open your DAW that supports VST3 and rescan plugins. The plugin is named "KimuVerb" under manufacturer "KimuAudio".
