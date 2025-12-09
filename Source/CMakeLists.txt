target_sources(${PROJECT_NAME}
PRIVATE
    ResamplingNAM.h
    NeuralAmpModeler.cpp
    NeuralAmpModeler.h
    StatusedTrigger.cpp
    StatusedTrigger.h
    ToneStack.cpp
    ToneStack.h
    NamEditor.h
    NamEditor.cpp
    PluginEditor.cpp
    PluginEditor.h
    PluginProcessor.cpp
    PluginProcessor.h
    MyLookAndFeel.h
    TopBarComponent.cpp
    TopBarComponent.h
    DoublerProcessor.h
    pedals/Compressor/CompressorProcessor.h
    pedals/Reverb/ReverbProcessor.h
    pedals/TubeScreamer/TSProcessor.h
    pedals/TubeScreamer/dsp/ClippingStage.cpp
    pedals/TubeScreamer/dsp/ClippingStage.h
    pedals/TubeScreamer/dsp/ToneStage.cpp
    pedals/TubeScreamer/dsp/ToneStage.h
    pedals/TubeScreamer/dsp/ClipWDFa.h
    pedals/TubeScreamer/dsp/ClipWDFb.h
    pedals/TubeScreamer/dsp/ClipWDFc.h
    architecture.hpp
)

add_subdirectory(CustomStandaloneWindow)
include_directories(CustomStandaloneWindow)

add_subdirectory(PresetManager)
include_directories(PresetManager)

