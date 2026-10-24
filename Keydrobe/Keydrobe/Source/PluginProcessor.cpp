#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    // time (seconds) for the signal to fall ~60 dB
    float coefFor (float seconds, double sr)
    {
        return std::exp (-6.9f / (std::max (seconds, 0.001f) * (float) sr));
    }

    struct KeySound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    class KeyVoice : public juce::SynthesiserVoice
    {
    public:
        explicit KeyVoice (KeydrobeProcessor& p) : proc (p) {}

        bool canPlaySound (juce::SynthesiserSound* s) override
        {
            return dynamic_cast<KeySound*> (s) != nullptr;
        }

        void startNote (int midi, float vel, juce::SynthesiserSound*, int) override
        {
            patch = &proc.currentPatch();
            sr = (float) getSampleRate();
            f0 = (float) juce::MidiMessage::getMidiNoteInHertz (midi);
            velocity = vel;
            n = 0;
            releasing = false;
            relGain = 1.0f;
            ampEnv = 1.0f;
            lp = 0.0f;
            hamEnv = 1.0f;
            fmEnv = 1.0f;
            modPh = modPh2 = 0.0f;

            const float bm = 0.4f + proc.pBright->load() * 1.2f;
            const float dm = 0.4f + proc.pDecay->load() * 1.2f;
            bright = juce::jlimit (0.02f, 1.0f, patch->bright * bm);
            decay  = juce::jlimit (0.1f, 20.0f, patch->decay * dm * std::exp2 (-(float) (midi - 60) / 48.0f));
            attack = patch->attack + proc.pAttack->load() * 0.6f;

            const float r = proc.pRelease->load();
            const float relSec = 0.05f + r * r * 3.0f;
            relCoef = coefFor (relSec, sr);

            trem = juce::jlimit (0.0f, 1.0f, patch->trem * proc.pTrem->load() * 2.0f);
            tremPh = 0.0f;
            tremInc = twoPi * patch->tremRate / sr;

            pan = juce::jlimit (-1.0f, 1.0f, (float) (midi - 60) / 36.0f) * 0.4f;
            const float a = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            gainL = std::cos (a) * 1.414f;
            gainR = std::sin (a) * 1.414f;
            level = 0.3f + 0.7f * vel;

            const float cA = std::exp2 (patch->detune / 2400.0f);
            const float cB = 1.0f / cA;
            ampCoef = coefFor (decay, sr);

            const float cutoff = juce::jlimit (300.0f, 18000.0f, f0 * (3.0f + bright * 14.0f) * (0.5f + velocity));
            lpA = 1.0f - std::exp (-twoPi * cutoff / sr);

            for (int k = 0; k < maxP; ++k)
            {
                amp[k] = 0; env[k] = 1; coef[k] = 1;
                ph[k][0] = ph[k][1] = 0; inc[k][0] = inc[k][1] = 0;
            }

            switch (patch->kind)
            {
                case Kind::Piano:
                {
                    nP = 8;
                    float sum = 0;
                    const float slope = std::max (0.3f, 0.7f + (1.0f - bright) * 2.4f - velocity * 0.5f);
                    for (int k = 0; k < nP; ++k)
                    {
                        const float nn = (float) (k + 1);
                        const float ratio = nn * std::sqrt (1.0f + patch->inharm * nn * nn);
                        if (f0 * ratio > 0.45f * sr) continue;
                        amp[k] = std::pow (nn, -slope);
                        sum += amp[k];
                        coef[k] = coefFor (decay / (1.0f + 0.6f * (float) k), sr);
                        inc[k][0] = twoPi * f0 * ratio * cA / sr;
                        inc[k][1] = twoPi * f0 * ratio * cB / sr;
                        ph[k][1] = rng.nextFloat() * twoPi;
                    }
                    norm = 0.45f / std::max (sum, 0.1f);
                    hamCoef = coefFor (0.03f, sr);
                    break;
                }
                case Kind::Bells:
                {
                    static const float base[7] = { 1.0f, 2.0f, 2.76f, 4.07f, 5.40f, 8.93f, 11.34f };
                    nP = 7;
                    float sum = 0;
                    const float stretch = 0.6f + 0.8f * patch->inharm;
                    for (int k = 0; k < nP; ++k)
                    {
                        const float ratio = 1.0f + (base[k] - 1.0f) * stretch;
                        if (f0 * ratio > 0.45f * sr) continue;
                        amp[k] = std::pow (0.5f + 0.45f * bright, (float) k);
                        sum += amp[k];
                        coef[k] = coefFor (decay / (1.0f + 0.45f * (float) k), sr);
                        inc[k][0] = twoPi * f0 * ratio * cA / sr;
                    }
                    norm = 0.6f / std::max (sum, 0.1f);
                    fmCoef = coefFor (std::max (patch->fmDecay, 0.02f), sr);
                    fmIdx = patch->fmIndex * (0.5f + velocity);
                    modInc = twoPi * f0 * 3.5f / sr;
                    hamCoef = coefFor (0.015f, sr);
                    break;
                }
                case Kind::EPiano:
                {
                    inc[0][0] = twoPi * f0 * cA / sr;
                    inc[0][1] = twoPi * f0 * cB / sr;
                    modInc  = twoPi * f0 * patch->inharm * cA / sr;
                    modInc2 = twoPi * f0 * patch->inharm * cB / sr;
                    inc[1][0] = twoPi * f0 * 14.0f / sr;
                    fmCoef = coefFor (std::max (patch->fmDecay, 0.05f), sr);
                    fmIdx = patch->fmIndex;
                    hamCoef = coefFor (0.12f, sr);
                    break;
                }
            }
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (allowTailOff) releasing = true;
            else clearCurrentNote();
        }

        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
        {
            if (patch == nullptr) return;

            for (int i = 0; i < num; ++i)
            {
                if (! isVoiceActive()) return;

                float s = tick();

                const float atk = attack <= 0.0005f ? 1.0f : std::min (1.0f, (float) n / (attack * sr));
                ++n;
                s *= atk;

                if (trem > 0.0f)
                {
                    s *= 1.0f - trem * 0.5f * (1.0f + std::sin (tremPh));
                    tremPh += tremInc;
                    if (tremPh > twoPi) tremPh -= twoPi;
                }

                if (releasing) { relGain *= relCoef; s *= relGain; }

                s *= level * 0.35f;

                if (out.getNumChannels() > 1)
                {
                    out.addSample (0, start + i, s * gainL);
                    out.addSample (1, start + i, s * gainR);
                }
                else out.addSample (0, start + i, s);

                ampEnv *= ampCoef;
                if (ampEnv < 0.0005f || relGain < 0.001f)
                {
                    clearCurrentNote();
                    break;
                }
            }
        }

    private:
        static constexpr int maxP = 8;

        float tick()
        {
            float o = 0.0f;

            switch (patch->kind)
            {
                case Kind::Piano:
                {
                    for (int k = 0; k < nP; ++k)
                    {
                        if (amp[k] == 0.0f) continue;
                        const float a = env[k] * amp[k];
                        env[k] *= coef[k];
                        o += a * (std::sin (ph[k][0]) + std::sin (ph[k][1]));
                        advance (ph[k][0], inc[k][0]);
                        advance (ph[k][1], inc[k][1]);
                    }
                    o *= norm;
                    if (hamEnv > 0.001f)
                    {
                        o += (rng.nextFloat() * 2.0f - 1.0f) * hamEnv * patch->noise * velocity * 0.6f;
                        hamEnv *= hamCoef;
                    }
                    lp += lpA * (o - lp);
                    o = lp;
                    break;
                }
                case Kind::Bells:
                {
                    fmEnv *= fmCoef;
                    const float mod = std::sin (modPh) * fmIdx * fmEnv;
                    advance (modPh, modInc);
                    for (int k = 0; k < nP; ++k)
                    {
                        if (amp[k] == 0.0f) continue;
                        const float a = env[k] * amp[k];
                        env[k] *= coef[k];
                        o += a * std::sin (ph[k][0] + (k < 2 ? mod : 0.0f));
                        advance (ph[k][0], inc[k][0]);
                    }
                    o *= norm;
                    if (hamEnv > 0.001f)
                    {
                        o += (rng.nextFloat() * 2.0f - 1.0f) * hamEnv * patch->noise * 0.3f * velocity;
                        hamEnv *= hamCoef;
                    }
                    break;
                }
                case Kind::EPiano:
                {
                    fmEnv *= fmCoef;
                    const float idx = fmIdx * (0.35f + velocity) * (0.25f + 0.75f * fmEnv);
                    const float body = std::sin (ph[0][0] + idx * std::sin (modPh))
                                     + std::sin (ph[0][1] + idx * std::sin (modPh2));
                    advance (ph[0][0], inc[0][0]);
                    advance (ph[0][1], inc[0][1]);
                    advance (modPh, modInc);
                    advance (modPh2, modInc2);

                    float tine = 0.0f;
                    if (hamEnv > 0.001f)
                    {
                        tine = std::sin (ph[1][0]) * hamEnv * bright * velocity * 0.5f;
                        tine += (rng.nextFloat() * 2.0f - 1.0f) * hamEnv * patch->noise * 0.4f;
                        advance (ph[1][0], inc[1][0]);
                        hamEnv *= hamCoef;
                    }

                    o = body * 0.5f * ampEnv + tine;
                    if (patch->drive > 0.0f)
                        o = std::tanh (o * (1.0f + patch->drive * 7.0f)) / (1.0f + patch->drive * 0.5f);
                    lp += lpA * (o - lp);
                    o = lp;
                    break;
                }
            }
            return o;
        }

        static void advance (float& p, float inc_)
        {
            p += inc_;
            if (p > twoPi) p -= twoPi;
        }

        KeydrobeProcessor& proc;
        const Patch* patch = nullptr;
        juce::Random rng;

        int nP = 0;
        float amp[maxP]{}, env[maxP]{}, coef[maxP]{}, ph[maxP][2]{}, inc[maxP][2]{};
        float sr = 44100, f0 = 440, velocity = 1, bright = .5f, decay = 2, attack = 0;
        float ampEnv = 1, ampCoef = 1, relGain = 1, relCoef = 0.999f, level = 1, norm = 1;
        float hamEnv = 1, hamCoef = 0.99f, fmEnv = 1, fmCoef = 0.999f, fmIdx = 0;
        float modPh = 0, modPh2 = 0, modInc = 0, modInc2 = 0;
        float trem = 0, tremPh = 0, tremInc = 0, pan = 0, gainL = 1, gainR = 1;
        float lp = 0, lpA = 1;
        int n = 0;
        bool releasing = false;
    };
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout KeydrobeProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterInt> (ParameterID { "preset", 1 }, "Preset", 0, KeydrobePresets::total - 1, 0));

    auto f = [&] (const char* id, const char* name, float def)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                      NormalisableRange<float> (0.0f, 1.0f, 0.001f), def));
    };
    f ("bright",  "Brightness", 0.5f);
    f ("decay",   "Decay",      0.5f);
    f ("attack",  "Attack",     0.0f);
    f ("release", "Release",    0.4f);
    f ("space",   "Space",      0.5f);
    f ("chorus",  "Chorus",     0.5f);
    f ("trem",    "Tremolo",    0.5f);
    f ("vol",     "Volume",     0.7f);
    return l;
}

KeydrobeProcessor::KeydrobeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KEYDROBE", createLayout())
{
    pPreset  = apvts.getRawParameterValue ("preset");
    pBright  = apvts.getRawParameterValue ("bright");
    pDecay   = apvts.getRawParameterValue ("decay");
    pAttack  = apvts.getRawParameterValue ("attack");
    pRelease = apvts.getRawParameterValue ("release");
    pSpace   = apvts.getRawParameterValue ("space");
    pChorus  = apvts.getRawParameterValue ("chorus");
    pTrem    = apvts.getRawParameterValue ("trem");
    pVol     = apvts.getRawParameterValue ("vol");

    synth.addSound (new KeySound());
    for (int i = 0; i < 16; ++i)
        synth.addVoice (new KeyVoice (*this));
}

void KeydrobeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    chorus.prepare (spec);
    chorus.setRate (0.45f);
    chorus.setDepth (0.45f);
    chorus.setCentreDelay (8.0f);
    chorus.setFeedback (0.0f);
    chorus.setMix (0.0f);

    reverb.setSampleRate (sampleRate);
    reverb.reset();
}

bool KeydrobeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void KeydrobeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);
    synth.renderNextBlock (buffer, midi, 0, numSamples);

    const Patch& p = currentPatch();

    chorus.setMix (juce::jlimit (0.0f, 0.9f, p.chorus * pChorus->load() * 2.0f));
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }

    const float space = juce::jlimit (0.0f, 1.0f, p.reverb * pSpace->load() * 2.0f);
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.35f + 0.6f * space;
    rp.damping  = 0.45f;
    rp.wetLevel = space * 0.45f;
    rp.dryLevel = 1.0f - space * 0.25f;
    rp.width    = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    if (buffer.getNumChannels() >= 2)
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);

    buffer.applyGain (pVol->load() * 1.3f);
}

juce::AudioProcessorEditor* KeydrobeProcessor::createEditor()
{
    return new KeydrobeEditor (*this);
}

void KeydrobeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void KeydrobeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KeydrobeProcessor();
}
