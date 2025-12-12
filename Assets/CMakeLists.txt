file(GLOB SRC_FILES    
    "Buttons/*.png"
    "PresetManagerAssets/*.png"
    "ButtonLabels/*.png"
)

juce_add_binary_data(BinaryData SOURCES
    knob.png
    arrowexpand.png
    arrowcontract.png
    knob_minimal.png
    knob_pre_effects.png
    knob_post_effects.png
    AmpModels/tworock.nam
    PreIcon.png
    AmpIcon.png
    PostIcon.png
    backgroundpre.png
    background_pre_no_knobs.png
    backgroundamp.png
    backgroundpost.png
    PedalButtonOn.png
    PedalButtonOff.png
    KnobOnPostEffects.png
    KnobOffPostEffects.png
    ${SRC_FILES}
)

# Need to build BinaryData with -fPIC flag on Linux
set_target_properties(BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE)