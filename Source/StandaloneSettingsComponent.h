#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"
using namespace juce;

class StandaloneSettingsComponent : public Component
{
public:
    StandaloneSettingsComponent(StandalonePluginHolder& pluginHolder, AudioDeviceManager& deviceManagerToUse, int maxAudioInputChannels,
                                int maxAudioOutputChannels)
        : owner(pluginHolder), deviceSelector(deviceManagerToUse, 0, maxAudioInputChannels, 0, maxAudioOutputChannels, true,
                                              (pluginHolder.processor.get() != nullptr && pluginHolder.processor->producesMidi()), true, false),
          shouldMuteLabel("Feedback Loop:", "Feedback Loop:"), shouldMuteButton("Mute audio input")
    {
        setOpaque(true);

        shouldMuteButton.setClickingTogglesState(true);
        shouldMuteButton.getToggleStateValue().referTo(owner.shouldMuteInput);

        addAndMakeVisible(deviceSelector);

        if (owner.getProcessorHasPotentialFeedbackLoop())
        {
            addAndMakeVisible(shouldMuteButton);
            addAndMakeVisible(shouldMuteLabel);

            shouldMuteLabel.attachToComponent(&shouldMuteButton, true);
        }

        settingsLookdAndFeel.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::transparentBlack);
        settingsLookdAndFeel.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::snow);
        settingsLookdAndFeel.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::snow);
        settingsLookdAndFeel.setColour(ComboBox::backgroundColourId, Colours::darkgrey.withAlpha(0.0f));
        settingsLookdAndFeel.setColour(ComboBox::outlineColourId, Colours::grey.withAlpha(0.8f));
        settingsLookdAndFeel.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        settingsLookdAndFeel.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        settingsLookdAndFeel.setColour(juce::PopupMenu::backgroundColourId, juce::Colours::grey.withAlpha(0.6f));
        settingsLookdAndFeel.setColour(juce::ListBox::backgroundColourId, Colours::transparentBlack);
        settingsLookdAndFeel.setColour(juce::ListBox::outlineColourId, Colours::grey.withAlpha(0.8f));

        deviceSelector.setLookAndFeel(&settingsLookdAndFeel);
    }

    void paint(Graphics& g) override { g.fillAll(juce::Colour::fromString("FF121212")); }

    void resized() override
    {
        const ScopedValueSetter<bool> scope(isResizing, true);

        auto r = getLocalBounds();

        if (owner.getProcessorHasPotentialFeedbackLoop())
        {
            auto itemHeight = deviceSelector.getItemHeight();
            auto extra = r.removeFromTop(itemHeight);

            auto seperatorHeight = (itemHeight >> 1);
            shouldMuteButton.setBounds(juce::Rectangle<int>(
                extra.proportionOfWidth(0.35f), seperatorHeight, extra.proportionOfWidth(0.60f), deviceSelector.getItemHeight()));

            r.removeFromTop(seperatorHeight);
        }

        deviceSelector.setBounds(r);
    }

    void childBoundsChanged(Component* childComp) override
    {
        if (!isResizing && childComp == &deviceSelector)
            setToRecommendedSize();
    }

    void setToRecommendedSize()
    {
        const auto extraHeight = [&]
        {
            if (!owner.getProcessorHasPotentialFeedbackLoop())
                return 0;

            const auto itemHeight = deviceSelector.getItemHeight();
            const auto separatorHeight = (itemHeight >> 1);
            return itemHeight + separatorHeight;
        }();

        setSize(getWidth(), deviceSelector.getHeight() + extraHeight);
    }

private:
    //==============================================================================
    StandalonePluginHolder& owner;
    AudioDeviceSelectorComponent deviceSelector;
    Label shouldMuteLabel;
    ToggleButton shouldMuteButton;
    bool isResizing = false;
    LookAndFeel_V4 settingsLookdAndFeel;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneSettingsComponent)
};