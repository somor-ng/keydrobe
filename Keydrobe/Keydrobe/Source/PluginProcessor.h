#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Presets.h"

class KeydrobeProcessor : public juce::AudioProcessor
{
public:
    KeydrobeProcessor();
    ~KeydrobeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Keydrobe"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    const Patch& currentPatch() const
    {
        const int idx = juce::jlimit (0, KeydrobePresets::total - 1, (int) pPreset->load());
        return KeydrobePresets::all()[(size_t) idx];
    }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    std::atomic<float> *pPreset, *pBright, *pDecay, *pAttack, *pRelease,
                       *pSpace, *pChorus, *pTrem, *pVol;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::Synthesiser synth;
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeydrobeProcessor)
};
