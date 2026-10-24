#pragma once
#include <juce_core/juce_core.h>
#include <vector>

// Keydrobe preset library.
// 3 instruments x 12 base characters x 10 "coats" = 360 presets (120 per instrument).

enum class Kind { Piano = 0, Bells = 1, EPiano = 2 };

struct Patch
{
    juce::String name;
    Kind  kind;
    float inharm;   // piano: string stiffness | bells: partial stretch 0..1 | epiano: FM modulator ratio
    float fmIndex;  // FM depth (bells shimmer / epiano body)
    float fmDecay;  // seconds the FM depth takes to fade
    float bright;   // 0..1 tone brightness
    float decay;    // seconds (approx. -60 dB)
    float attack;   // seconds
    float detune;   // cents of string / tine spread
    float noise;    // hammer / key noise amount
    float drive;    // 0..1 saturation (epiano grit)
    float trem;     // tremolo depth 0..1
    float tremRate; // Hz
    float reverb;   // 0..1
    float chorus;   // 0..1
};

namespace KeydrobePresets
{
    constexpr int perKind = 120;
    constexpr int total   = perKind * 3;

    inline const char* kindName (Kind k)
    {
        return k == Kind::Piano ? "Piano" : k == Kind::Bells ? "Bells" : "E-Piano";
    }

    struct Coat { const char* name; float bright, decay, attack, noise, drive, rev, chorus, detune, fm; };

    inline std::vector<Patch> build()
    {
        const Kind P = Kind::Piano, B = Kind::Bells, E = Kind::EPiano;

        // name, kind, inharm, fmIndex, fmDecay, bright, decay, attack, detune, noise, drive, trem, tremRate, reverb, chorus
        const Patch piano[12] = {
            { "Haunted Nursery", P, .0016f, 0, 0, .30f, 3.2f, .004f, 11, .25f, 0,   .18f, 3.2f, .65f, .30f },
            { "Crypt Grand",     P, .0006f, 0, 0, .22f, 5.5f, .006f,  5, .30f, 0,   0,    0,    .75f, .10f },
            { "Moonlit Ballroom",P, .0004f, 0, 0, .45f, 5.0f, .004f,  4, .22f, 0,   0,    0,    .55f, .15f },
            { "Cathedral Felt",  P, .0003f, 0, 0, .18f, 4.5f, .012f,  3, .45f, 0,   0,    0,    .80f, .05f },
            { "Dusty Upright",   P, .0020f, 0, 0, .38f, 2.4f, .003f, 14, .45f, .05f,0,    0,    .30f, .10f },
            { "Toy Piano",       P, .0060f, 0, 0, .85f, 1.0f, .001f,  8, .15f, 0,   0,    0,    .20f, .10f },
            { "Sunny Parlor",    P, .0005f, 0, 0, .70f, 3.0f, .003f,  3, .25f, 0,   0,    0,    .30f, .20f },
            { "Candy Keys",      P, .0030f, 0, 0, .88f, 1.4f, .002f,  6, .10f, 0,   0,    0,    .35f, .35f },
            { "Honeyed Felt",    P, .0004f, 0, 0, .50f, 3.5f, .008f,  3, .40f, 0,   0,    0,    .40f, .20f },
            { "Fairy Cottage",   P, .0010f, 0, 0, .78f, 2.8f, .003f,  5, .15f, 0,   .10f, 5.0f, .60f, .50f },
            { "Lullaby Cloud",   P, .0003f, 0, 0, .28f, 5.0f, .015f,  5, .20f, 0,   .20f, 2.0f, .85f, .45f },
            { "Ragtime Tavern",  P, .0025f, 0, 0, .65f, 2.0f, .002f, 16, .35f, .10f,0,    0,    .20f, .05f },
        };

        const Patch bells[12] = {
            { "Cathedral Tower",  B, .55f, .8f, 2.0f, .55f, 7.0f, .001f,  2, .25f, 0,   0,    0,    .80f, .10f },
            { "Music Box",        B, .30f, 0,   0,    .75f, 2.2f, .001f,  3, .10f, 0,   0,    0,    .35f, .10f },
            { "Glass Chime",      B, .70f, 1.2f,1.5f, .85f, 5.0f, .001f,  4, .05f, 0,   .10f, 4.0f, .55f, .30f },
            { "Ghost Handbell",   B, .60f, 1.5f,3.0f, .35f, 6.0f, .002f,  9, .20f, 0,   .30f, 4.5f, .80f, .40f },
            { "Tubular Haunt",    B, .45f, .6f, 2.0f, .30f, 8.0f, .002f,  3, .30f, 0,   0,    0,    .85f, .15f },
            { "Crystal Celesta",  B, .20f, .3f, .5f,  .80f, 2.8f, .001f,  3, .08f, 0,   0,    0,    .45f, .25f },
            { "Wind Chime Dream", B, .85f, 1.0f,2.5f, .70f, 6.5f, .002f,  6, .05f, 0,   .20f, 3.0f, .70f, .50f },
            { "Ice Carillon",     B, .50f, .7f, 1.5f, .90f, 4.5f, .001f,  2, .10f, 0,   0,    0,    .60f, .20f },
            { "Sleigh Lullaby",   B, .65f, .4f, 1.0f, .80f, 3.0f, .001f,  7, .10f, 0,   .15f, 6.0f, .45f, .35f },
            { "Star Glockenspiel",B, .15f, .2f, .5f,  .95f, 2.5f, .001f,  2, .12f, 0,   0,    0,    .40f, .15f },
            { "Drowned Bell",     B, .50f, 1.8f,4.0f, .20f, 9.0f, .004f, 12, .35f, .05f,.20f, 1.5f, .90f, .30f },
            { "Fae Bellflower",   B, .75f, .9f, 2.0f, .75f, 4.0f, .002f,  5, .08f, 0,   .10f, 5.0f, .65f, .45f },
        };

        // inharm = FM modulator ratio for e-pianos; fmIndex climbs from soft to aggressive
        const Patch epiano[12] = {
            { "Velvet Tine",      E, 1, 1.2f, 1.2f, .40f, 3.5f, .004f,  3, .08f, 0,   .20f, 4.5f, .35f, .40f },
            { "Dream Rhodes",     E, 1, 1.0f, 1.5f, .35f, 4.5f, .006f,  4, .05f, 0,   .30f, 3.0f, .65f, .60f },
            { "Lullaby Tine",     E, 1, .6f,  1.0f, .25f, 4.0f, .010f,  3, .03f, 0,   .15f, 2.5f, .70f, .50f },
            { "Warm Wurly",       E, 1, 1.8f, .8f,  .45f, 2.8f, .004f,  4, .10f, .15f,.35f, 5.5f, .30f, .25f },
            { "Suitcase Chorus",  E, 1, 1.4f, 1.2f, .50f, 3.5f, .004f,  6, .08f, .05f,.60f, 5.0f, .30f, .55f },
            { "Glass FM",         E, 1, 2.5f, .9f,  .80f, 3.0f, .002f,  3, .05f, 0,   0,    0,    .40f, .30f },
            { "Bark Wurly",       E, 1, 3.0f, .6f,  .60f, 2.0f, .002f,  5, .15f, .45f,.20f, 5.5f, .20f, .10f },
            { "Crunch Rhodes",    E, 1, 2.4f, .7f,  .65f, 2.5f, .002f,  4, .12f, .55f,.10f, 4.5f, .20f, .10f },
            { "Growl Tine",       E, 1, 4.0f, .5f,  .70f, 2.0f, .002f,  5, .18f, .70f,0,    0,    .15f, .10f },
            { "Rusted Clav Keys", E, 2, 3.5f, .3f,  .85f, 1.2f, .001f,  3, .25f, .50f,0,    0,    .10f, .05f },
            { "Industrial Tine",  E, 1, 5.0f, .4f,  .90f, 2.0f, .001f,  8, .30f, .85f,0,    0,    .20f, 0     },
            { "Furnace Keys",     E, 3, 6.0f, .5f,  .95f, 1.8f, .001f, 10, .35f, 1.0f,.10f, 6.0f, .15f, .05f },
        };

        const Coat coats[10] = {
            { "Plain",   1.00f, 1.00f, 0.00f, 1.0f, 0.00f,  0.00f,  0.00f, 1.0f, 1.00f },
            { "Veiled",  0.60f, 1.10f, 0.00f, 1.0f, 0.00f,  0.20f,  0.05f, 1.0f, 0.80f },
            { "Gilded",  1.25f, 1.20f, 0.00f, 0.8f, 0.00f,  0.05f,  0.05f, 1.0f, 1.10f },
            { "Dusty",   0.80f, 0.90f, 0.00f, 1.6f, 0.00f, -0.05f,  0.00f, 1.6f, 0.90f },
            { "Velvet",  0.75f, 0.90f, 0.012f,1.2f, 0.00f,  0.05f,  0.05f, 0.8f, 0.80f },
            { "Glass",   1.40f, 1.30f, 0.00f, 0.6f, 0.00f,  0.10f,  0.10f, 0.8f, 1.20f },
            { "Wide",    1.00f, 1.00f, 0.00f, 1.0f, 0.00f,  0.10f,  0.35f, 1.2f, 1.00f },
            { "Short",   1.00f, 0.45f, 0.00f, 1.0f, 0.00f, -0.10f,  0.00f, 1.0f, 1.00f },
            { "Bloom",   0.90f, 1.50f, 0.12f, 0.7f, 0.00f,  0.25f,  0.15f, 1.0f, 0.90f },
            { "Grit",    1.15f, 0.90f, 0.00f, 1.3f, 0.25f, -0.05f,  0.00f, 1.3f, 1.30f },
        };

        std::vector<Patch> out;
        out.reserve (total);

        auto emit = [&] (const Patch* bases)
        {
            for (int b = 0; b < 12; ++b)
                for (int c = 0; c < 10; ++c)
                {
                    Patch p = bases[b];
                    const Coat& k = coats[c];
                    p.name    = juce::String (bases[b].name) + " - " + k.name;
                    p.bright  = juce::jlimit (0.02f, 1.0f, p.bright * k.bright);
                    p.decay   = juce::jlimit (0.15f, 14.0f, p.decay * k.decay);
                    p.attack += k.attack;
                    p.noise   = juce::jlimit (0.0f, 1.0f, p.noise * k.noise);
                    p.drive   = juce::jlimit (0.0f, 1.0f, p.drive + k.drive);
                    p.reverb  = juce::jlimit (0.0f, 1.0f, p.reverb + k.rev);
                    p.chorus  = juce::jlimit (0.0f, 1.0f, p.chorus + k.chorus);
                    p.detune *= k.detune;
                    p.fmIndex *= k.fm;
                    out.push_back (p);
                }
        };

        emit (piano);
        emit (bells);
        emit (epiano);
        return out;
    }

    inline const std::vector<Patch>& all()
    {
        static const std::vector<Patch> list = build();
        return list;
    }
}
