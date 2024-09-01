# NAM JUCE: Neural Amp Modeler JUCE Implementation

A JUCE implementation of Steven Atkinson's [NeuralAmpModelerPlugin](https://github.com/sdatkinson/NeuralAmpModelerPlugin). This Repository is still a work-in-proress, but the basic functionality is there.

</br>
<p align="center">
    <img src="https://i.allthepics.net/2024/08/12/namJuce.gif" alt="animated" />
</p>


## Building

```bash
git clone https://github.com/tr3m/nam-juce
cd nam-juce
```

### Windows

```bash
cmake -B build
cmake --build build --config Release -j %NUMBER_OF_PROCESSORS% 
```
The `%NUMBER_OF_PROCESSORS%` environment variable is for cmd. The Powershell/New Windows Terminal equivalent is `$ENV:NUMBER_OF_PROCESSORS`.

### MacOS

```bash
cmake -B build
cmake --build build -- -j $(sysctl -n hw.physicalcpu)
```

### Linux

```bash
cmake -B build
cmake --build build -- -j $(nproc)
```

Git sumbodules dont need to be initialized manually. CMake will initialize the appropriate submodules depending on the defined flags.

### Optional CMake Flags

* `-DUSE_NATIVE_ARCH=1`
    * Enables processor-specific optimizations for modern x64 processors.
* `-DCMAKE_PREFIX_PATH=<PATH/TO/JUCE>`
    * Use a global installation of JUCE instead of the repo submodule.
* `-DASIO_PATH=<PATH_TO_ASIO_SDK>` (Windows only)
    * Enables ASIO support for the Standalone Application.

<br/>

The resulting binaries can be found under <u>`build/NEURAL_AMP_MODELER_artefacts/Release/`</u>.

## Supported Platforms

- Windows
- MacOS
- Linux

## Supported Formats

- VST3
- AU
- Standalone Application

<br/>

Note: The Standalone application for Windows doesn't support ASIO by default. For ASIO support a path to Steingberg's ASIO SDK needs to be provided by using the `ASIO_PATH` flag with CMake.

More plugin formats like LV2 and Legacy VST can be built by providing the appropriate SDK paths and setting the corresponding JUCE flags in the main `CMakeLists.txt` file.

## Getting Models
You can find Models and Impulse Responses shared by the community on [ToneHunt](https://tonehunt.org).