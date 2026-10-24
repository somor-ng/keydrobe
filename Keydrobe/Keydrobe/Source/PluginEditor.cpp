#include "PluginEditor.h"

namespace
{
    struct KnobInfo { const char* label; const char* id; };
    const KnobInfo knobInfo[8] = {
        { "Brightness", "bright" }, { "Decay", "decay" },  { "Attack", "attack" }, { "Release", "release" },
        { "Space", "space" },       { "Chorus", "chorus" }, { "Tremolo", "trem" },  { "Volume", "vol" }
    };

    juce::Path gothicArch (juce::Rectangle<float> r, float h)
    {
        juce::Path p;
        p.startNewSubPath (r.getX(), r.getBottom());
        p.lineTo (r.getX(), r.getY() + h);
        p.quadraticTo (r.getX() + r.getWidth() * 0.08f, r.getY() + h * 0.2f, r.getCentreX(), r.getY());
        p.quadraticTo (r.getRight() - r.getWidth() * 0.08f, r.getY() + h * 0.2f, r.getRight(), r.getY() + h);
        p.lineTo (r.getRight(), r.getBottom());
        p.closeSubPath();
        return p;
    }

    void drawSparkle (juce::Graphics& g, float x, float y, float s)
    {
        g.drawLine (x - s, y, x + s, y, 1.0f);
        g.drawLine (x, y - s, x, y + s, 1.0f);
        g.drawLine (x - s * 0.5f, y - s * 0.5f, x + s * 0.5f, y + s * 0.5f, 0.6f);
        g.drawLine (x - s * 0.5f, y + s * 0.5f, x + s * 0.5f, y - s * 0.5f, 0.6f);
    }
}

//==============================================================================
KeydrobeEditor::KeydrobeEditor (KeydrobeProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    setSize (940, 664);

    for (auto* b : { &bPiano, &bBells, &bEP, &bPrev, &bNext, &bRand })
    {
        b->setColour (juce::TextButton::textColourOffId, RoseLNF::cream());
        b->setColour (juce::TextButton::textColourOnId, RoseLNF::gold());
        addAndMakeVisible (*b);
    }
    bPiano.setColour (juce::TextButton::buttonColourId, RoseLNF::kindColour (0));
    bBells.setColour (juce::TextButton::buttonColourId, RoseLNF::kindColour (1));
    bEP   .setColour (juce::TextButton::buttonColourId, RoseLNF::kindColour (2));
    bPrev .setColour (juce::TextButton::buttonColourId, juce::Colour (0xffc2185b));
    bNext .setColour (juce::TextButton::buttonColourId, juce::Colour (0xffc2185b));
    bRand .setColour (juce::TextButton::buttonColourId, juce::Colour (0xffe8a0d0));

    for (auto* b : { &bPiano, &bBells, &bEP })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (1);
    }
    bPiano.onClick = [this] { setKind (0, true); };
    bBells.onClick = [this] { setKind (1, true); };
    bEP   .onClick = [this] { setKind (2, true); };

    bPrev.onClick = [this]
    {
        const int cur = (int) proc.pPreset->load();
        setPreset ((cur - 1 + KeydrobePresets::total) % KeydrobePresets::total);
    };
    bNext.onClick = [this]
    {
        const int cur = (int) proc.pPreset->load();
        setPreset ((cur + 1) % KeydrobePresets::total);
    };
    bRand.onClick = [this]
    {
        setPreset (kind * KeydrobePresets::perKind + juce::Random::getSystemRandom().nextInt (KeydrobePresets::perKind));
    };

    list.setRowHeight (26);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    list.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (list);

    for (int i = 0; i < numKnobs; ++i)
    {
        knobs[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knobs[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (knobs[i]);

        labels[i].setText (knobInfo[i].label, juce::dontSendNotification);
        labels[i].setJustificationType (juce::Justification::centred);
        labels[i].setFont (juce::Font (juce::FontOptions ("Georgia", 15.0f, juce::Font::italic)));
        labels[i].setColour (juce::Label::textColourId, RoseLNF::cream());
        addAndMakeVisible (labels[i]);

        att[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, knobInfo[i].id, knobs[i]);
    }

    keyboard.setAvailableRange (36, 96);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (26.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfff7ead9));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff2b1630));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xffb8913f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour (0xaaff7ac8));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x55f2a3c9));
    addAndMakeVisible (keyboard);

    setKind ((int) proc.pPreset->load() / KeydrobePresets::perKind, false);
    startTimerHz (10);
    timerCallback();
}

KeydrobeEditor::~KeydrobeEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void KeydrobeEditor::setPreset (int idx)
{
    if (auto* prm = proc.apvts.getParameter ("preset"))
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) idx));
}

void KeydrobeEditor::setKind (int k, bool changePreset)
{
    kind = juce::jlimit (0, 2, k);
    bPiano.setToggleState (kind == 0, juce::dontSendNotification);
    bBells.setToggleState (kind == 1, juce::dontSendNotification);
    bEP   .setToggleState (kind == 2, juce::dontSendNotification);
    list.updateContent();
    list.repaint();

    if (changePreset)
        setPreset (kind * KeydrobePresets::perKind + juce::jmax (0, list.getSelectedRow()));
}

void KeydrobeEditor::timerCallback()
{
    const int idx = (int) proc.pPreset->load();
    if (idx == lastIdx) return;
    lastIdx = idx;

    const int k = idx / KeydrobePresets::perKind;
    if (k != kind) setKind (k, false);

    syncing = true;
    list.selectRow (idx % KeydrobePresets::perKind);
    syncing = false;
    repaint();
}

void KeydrobeEditor::selectedRowsChanged (int lastRow)
{
    if (syncing || lastRow < 0) return;
    setPreset (kind * KeydrobePresets::perKind + lastRow);
}

void KeydrobeEditor::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (selected)
        g.fillAll (RoseLNF::gold().withAlpha (0.22f));
    else if (row % 2)
        g.fillAll (juce::Colours::white.withAlpha (0.04f));

    const auto& p = KeydrobePresets::all()[(size_t) (kind * KeydrobePresets::perKind + row)];
    RoseLNF::drawRose (g, { 4.0f, 3.0f, (float) h - 6.0f, (float) h - 6.0f },
                       RoseLNF::kindColour (kind), (float) row * 0.4f, selected ? 0.35f : 0.0f);

    g.setColour (selected ? RoseLNF::gold() : RoseLNF::cream());
    g.setFont (juce::Font (juce::FontOptions ("Georgia", 15.0f, selected ? juce::Font::bold : juce::Font::plain)));
    g.drawText (p.name, h + 4, 0, w - h - 8, h, juce::Justification::centredLeft);
}

//==============================================================================
void KeydrobeEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float W = b.getWidth(), H = b.getHeight();

    // dusk sky: midnight plum -> mauve -> dream pink
    juce::ColourGradient sky (juce::Colour (0xff1b0f2e), 0, 0, juce::Colour (0xff7a4580), 0, H, false);
    sky.addColour (0.55, juce::Colour (0xff3d2050));
    g.setGradientFill (sky);
    g.fillAll();

    // dreamcore clouds
    juce::Random rnd (7);
    for (int i = 0; i < 14; ++i)
    {
        const float x = rnd.nextFloat() * W, y = rnd.nextFloat() * H, rw = 100.0f + rnd.nextFloat() * 160.0f;
        g.setColour (juce::Colour (0xfffbc7e6).withAlpha (0.06f));
        g.fillEllipse (x, y, rw, rw * 0.35f);
        g.fillEllipse (x + rw * 0.25f, y - rw * 0.1f, rw * 0.5f, rw * 0.35f);
    }

    // sparkles
    for (int i = 0; i < 55; ++i)
    {
        g.setColour (juce::Colour (0xffffe6f5).withAlpha (0.25f + rnd.nextFloat() * 0.5f));
        drawSparkle (g, rnd.nextFloat() * W, rnd.nextFloat() * H, 2.0f + rnd.nextFloat() * 4.0f);
    }

    // gilded double border + corner roses
    g.setColour (RoseLNF::gold());
    g.drawRect (b.reduced (6.0f), 2.0f);
    g.drawRect (b.reduced (12.0f), 1.0f);
    const float cs = 34.0f;
    RoseLNF::drawRose (g, { 3.0f, 3.0f, cs, cs }, RoseLNF::kindColour (0), 0.0f, 0.3f);
    RoseLNF::drawRose (g, { W - cs - 3.0f, 3.0f, cs, cs }, RoseLNF::kindColour (1), 1.0f, 0.3f);
    RoseLNF::drawRose (g, { 3.0f, H - cs - 3.0f, cs, cs }, RoseLNF::kindColour (2), 2.0f, 0.3f);
    RoseLNF::drawRose (g, { W - cs - 3.0f, H - cs - 3.0f, cs, cs }, RoseLNF::kindColour (0), 3.0f, 0.3f);

    // title
    const juce::Font titleFont (juce::FontOptions ("Georgia", 52.0f, juce::Font::bold | juce::Font::italic));
    g.setFont (titleFont);
    g.setColour (juce::Colour (0xffff7ac8).withAlpha (0.35f));
    g.drawText ("Keydrobe", juce::Rectangle<int> (2, 14, getWidth(), 64), juce::Justification::centred);
    g.setColour (RoseLNF::gold());
    g.drawText ("Keydrobe", juce::Rectangle<int> (0, 12, getWidth(), 64), juce::Justification::centred);

    g.setFont (juce::Font (juce::FontOptions ("Georgia", 16.0f, juce::Font::italic)));
    g.setColour (juce::Colour (0xfff2c6e4));
    g.drawText ("~ a wardrobe for your keys ~", juce::Rectangle<int> (0, 70, getWidth(), 22), juce::Justification::centred);

    // gothic arch panels
    const juce::Rectangle<float> leftPanel (20.0f, 100.0f, 340.0f, 405.0f);
    const juce::Rectangle<float> rightPanel (372.0f, 100.0f, 548.0f, 405.0f);
    for (auto panel : { leftPanel, rightPanel })
    {
        auto arch = gothicArch (panel, 34.0f);
        g.setColour (juce::Colour (0xff150a20).withAlpha (0.6f));
        g.fillPath (arch);
        g.setColour (RoseLNF::gold());
        g.strokePath (arch, juce::PathStrokeType (2.0f));
        g.setColour (RoseLNF::gold().withAlpha (0.5f));
        g.strokePath (gothicArch (panel.reduced (6.0f), 34.0f), juce::PathStrokeType (1.0f));
    }

    // current preset readout
    if (lastIdx >= 0)
    {
        const auto& p = KeydrobePresets::all()[(size_t) lastIdx];
        g.setFont (juce::Font (juce::FontOptions ("Georgia", 28.0f, juce::Font::bold | juce::Font::italic)));
        g.setColour (juce::Colour (0xfff8c8e6));
        g.drawText (p.name, juce::Rectangle<int> (390, 128, 512, 40), juce::Justification::centred);
        g.setFont (juce::Font (juce::FontOptions ("Georgia", 13.0f, juce::Font::italic)));
        g.setColour (RoseLNF::gold());
        g.drawText (juce::String (KeydrobePresets::kindName (p.kind)) + "  ·  No. " + juce::String (lastIdx + 1)
                        + " of " + juce::String (KeydrobePresets::total),
                    juce::Rectangle<int> (390, 164, 512, 20), juce::Justification::centred);
    }

    g.setFont (juce::Font (juce::FontOptions ("Georgia", 13.0f, juce::Font::italic)));
    g.setColour (juce::Colour (0xffe9b8dc).withAlpha (0.8f));
    g.drawText ("you are safe here  ·  the keys remember", juce::Rectangle<int> (0, 636, getWidth(), 20),
                juce::Justification::centred);
}

void KeydrobeEditor::resized()
{
    // left panel
    bPiano.setBounds (26, 138, 102, 34);
    bBells.setBounds (132, 138, 102, 34);
    bEP   .setBounds (238, 138, 102, 34);
    list.setBounds (28, 180, 316, 278);
    bPrev.setBounds (26, 464, 102, 34);
    bRand.setBounds (132, 464, 102, 34);
    bNext.setBounds (238, 464, 102, 34);

    // right panel: 4 x 2 rose knobs
    const int gx = 390, gy = 196, cw = 128, ch = 150;
    for (int i = 0; i < numKnobs; ++i)
    {
        const int cx = gx + (i % 4) * cw, cy = gy + (i / 4) * ch;
        knobs[i].setBounds (cx + 14, cy, 100, 100);
        labels[i].setBounds (cx, cy + 102, cw, 24);
    }

    keyboard.setBounds (20, 520, getWidth() - 40, 108);
}
