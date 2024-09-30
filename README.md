<div id="namjuce-icon" align="center">
    <br />
    <img src="./Assets/ICON.png" alt="nam-juce icon" width="128"/>
    <h1>NAM JUCE</h1>
    <h3>Neural Amp Modeler JUCE Implementation</h3>
</div>


<div id="badges" align="center">

[![current release](https://img.shields.io/github/release/tr3m/nam-juce.svg)](https://github.com/tr3m/nam-juce/releases)
[![chocolatey](https://img.shields.io/chocolatey/v/nam-juce)](https://community.chocolatey.org/packages/nam-juce/)
[![license](https://img.shields.io/github/license/tr3m/nam-juce.svg)](https://github.com/tr3m/nam-juce/blob/master/LICENSE.txt)
</div>

<div id="badges" align="center">

**A JUCE implementation of Steven Atkinson's [NeuralAmpModelerPlugin](https://github.com/sdatkinson/NeuralAmpModelerPlugin). This Repository is still a work-in-proress, but the basic functionality is there.**
</div>

</br>
<p align="center">
    <img src="https://i.allthepics.net/2024/08/12/namJuce.gif" alt="animated" />
</p>

## Table of Contents

- [Installation](#installation)
  - [Releases](#releases)
  - [Chocolatey (Windows)](#chocolatey)
- [Building](#building)
    - [Optional CMake Flags](#optional-flags)
- [Supported Platforms](#supported-platforms)
- [Supported Formats](#supported-formats)
- [Getting Amp Models](#getting-models)

## <a id="installation"></a> Installation

### <a id="releases"></a> Releases
The latest version for Windows and MacOS can be found in the [Releases](https://github.com/Tr3m/nam-juce/releases) page.

### <a id="chocolatey"></a> Chocolatey (Windows)
For windows, the Chocolatey package can be installed by running:
```bash
choco install nam-juce
```

## <a id="building"></a> Building

```bash
git clone https://github.com/tr3m/nam-juce
cd nam-juce
```

Git sumbodules dont need to be initialized manually. CMake will initialize the appropriate submodules depending on the defined flags.

### <a id="building-windows"></a> Windows

```bash
cmake -B build
cmake --build build --config Release -j %NUMBER_OF_PROCESSORS% 
```
The `%NUMBER_OF_PROCESSORS%` environment variable is for cmd. The Powershell/New Windows Terminal equivalent is `$ENV:NUMBER_OF_PROCESSORS`.

### <a id="building-macos"></a> MacOS

```bash
cmake -B build
cmake --build build -- -j $(sysctl -n hw.physicalcpu)
```

### <a id="building-linux"></a> Linux

```bash
cmake -B build
cmake --build build -- -j $(nproc)
```

Linux dependencies for JUCE can be found [here](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md). Keep in mind that the packages they list are meant for Ubuntu, so you might have to do your own research depending on your distro.

### <a id="optional-flags"></a> Optional CMake Flags

* `-DUSE_NATIVE_ARCH=1`
    * Enables processor-specific optimizations for modern x64 processors.
* `-DCMAKE_PREFIX_PATH=<PATH/TO/JUCE>`
    * Use a global installation of JUCE instead of the repo submodule.
* `-DASIO_PATH=<PATH_TO_ASIO_SDK>` (Windows only)
    * Enables ASIO support for the Standalone Application.

<br/>

The resulting binaries can be found under <u>`build/NEURAL_AMP_MODELER_artefacts/Release/`</u>.

## <a id="supported-platforms"></a> Supported Platforms

- Windows
- MacOS
- Linux

## <a id="suppoprted-formats"></a> Supported Formats

- VST3
- AU
- Standalone Application

<br/>

Note: The Standalone application for Windows doesn't support ASIO by default. For ASIO support a path to Steingberg's ASIO SDK needs to be provided by using the `ASIO_PATH` flag with CMake.

More plugin formats like LV2 and Legacy VST can be built by providing the appropriate SDK paths and setting the corresponding JUCE flags in the main `CMakeLists.txt` file.

## <a id="getting-models"></a> Getting Models
You can find Models and Impulse Responses shared by the community on [ToneHunt](https://tonehunt.org).