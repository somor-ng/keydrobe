#pragma once
#include "PluginProcessor.h"

//==============================================================================
// Medieval-gilt + dreamcore look. Every button and knob wears a rose.
struct RoseLNF : public juce::LookAndFeel_V4
{
    static juce::Colour gold()  { return juce::Colour (0xffd9b45a); }
    static juce::Colour cream() { return juce::Colour (0xfffbeedd); }

    static juce::Colour kindColour (int kind)
    {
        return kind == 0 ? juce::Colour (0xffb3123f)    // piano: deep crimson
             : kind == 1 ? juce::Colour (0xfff2a3c9)    // bells: dream pink
                         : juce::Colour (0xff8e5ad7);   // e-piano: violet
    }

    static void drawRose (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c,
                          float rot, float glow = 0.0f)
    {
        const auto ctr = r.getCentre();
        const float R = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
        if (R < 2.0f) return;

        if (glow > 0.0f)
        {
            juce::ColourGradient cg (c.withAlpha (glow), ctr, c.withAlpha (0.0f), ctr.translated (R * 1.5f, 0), true);
            g.setGradientFill (cg);
            g.fillEllipse (ctr.x - R * 1.5f, ctr.y - R * 1.5f, R * 3.0f, R * 3.0f);
        }

        static const int petalCount[4] = { 5, 5, 4, 3 };
        for (int layer = 0; layer < 4; ++layer)
        {
            const float rad = R * (0.96f - (float) layer * 0.2f);
            const float pr  = rad * (0.52f - (float) layer * 0.04f);
            for (int p = 0; p < petalCount[layer]; ++p)
            {
                const float a = rot + (float) layer * 0.7f
                              + (float) p * juce::MathConstants<float>::twoPi / (float) petalCount[layer];
                const float cx = ctr.x + std::cos (a) * (rad - pr);
                const float cy = ctr.y + std::sin (a) * (rad - pr);
                g.setColour (c.darker (0.55f - (float) layer * 0.17f));
                g.fillEllipse (cx - pr, cy - pr, pr * 2.0f, pr * 2.0f);
                g.setColour (c.brighter (0.5f).withAlpha (0.65f));
                g.drawEllipse (cx - pr, cy - pr, pr * 2.0f, pr * 2.0f, juce::jmax (0.6f, R * 0.04f));
            }
        }

        juce::Path sp;
        for (float t = 0.0f; t < 12.0f; t += 0.25f)
        {
            const float rr = R * 0.05f + t * R * 0.028f;
            const juce::Point<float> pt (ctr.x + std::cos (t + rot) * rr, ctr.y + std::sin (t + rot) * rr);
            if (t == 0.0f) sp.startNewSubPath (pt); else sp.lineTo (pt);
        }
        g.setColour (c.brighter (0.8f));
        g.strokePath (sp, juce::PathStrokeType (juce::jmax (0.7f, R * 0.05f)));
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (juce::FontOptions ("Georgia", 16.0f, juce::Font::bold));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bg,
                               bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.5f);
        const bool on = b.getToggleState();
        const float corner = r.getHeight() * 0.5f;

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a1530), r.getTopLeft(),
                                                 juce::Colour (0xff4a2243), r.getBottomLeft(), false));
        g.fillRoundedRectangle (r, corner);
        g.setColour (juce::Colour (on ? 0xfff2d28a : 0xffb8913f).withAlpha (over ? 1.0f : 0.85f));
        g.drawRoundedRectangle (r, corner, down ? 3.0f : 1.8f);

        const float d = r.getHeight() - 4.0f;
        const float rot = (on ? 0.5f : 0.0f) + (over ? 0.3f : 0.0f);
        drawRose (g, { r.getX() + 2.0f, r.getY() + 2.0f, d, d }, bg, rot, on ? 0.4f : 0.0f);
        drawRose (g, { r.getRight() - d - 2.0f, r.getY() + 2.0f, d, d }, bg, -rot, on ? 0.4f : 0.0f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
        const float R = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
        const auto ctr = r.getCentre();
        const float ang = startAngle + pos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc (ctr.x, ctr.y, R, R, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff3b2140));
        g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc (ctr.x, ctr.y, R, R, 0.0f, startAngle, ang, true);
        g.setColour (gold());
        g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        drawRose (g, r.reduced (R * 0.2f), juce::Colour (0xffc2185b), ang, 0.25f);

        const float px = ctr.x + std::sin (ang) * R * 0.95f;
        const float py = ctr.y - std::cos (ang) * R * 0.95f;
        g.setColour (cream());
        g.fillEllipse (px - 3.5f, py - 3.5f, 7.0f, 7.0f);
    }
};

//==============================================================================
class KeydrobeEditor : public juce::AudioProcessorEditor,
                       private juce::ListBoxModel,
                       private juce::Timer
{
public:
    explicit KeydrobeEditor (KeydrobeProcessor&);
    ~KeydrobeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // ListBoxModel
    int getNumRows() override { return KeydrobePresets::perKind; }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void selectedRowsChanged (int lastRow) override;

    void timerCallback() override;
    void setKind (int k, bool changePreset);
    void setPreset (int idx);

    KeydrobeProcessor& proc;
    RoseLNF lnf;

    juce::TextButton bPiano { "Piano" }, bBells { "Bells" }, bEP { "E-Piano" },
                     bPrev { "Prev" }, bNext { "Next" }, bRand { "Surprise" };
    juce::ListBox list { "presets", this };

    static constexpr int numKnobs = 8;
    juce::Slider knobs[numKnobs];
    juce::Label labels[numKnobs];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att[numKnobs];

    juce::MidiKeyboardComponent keyboard;

    int kind = 0, lastIdx = -1;
    bool syncing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeydrobeEditor)
};
