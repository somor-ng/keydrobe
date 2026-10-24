# Keydrobe — a wardrobe for keys

360 synthesized presets in one plugin (VST3 + Standalone):

| Wardrobe | Presets | Range |
|---|---|---|
| Piano | 120 | haunted nursery, crypt grand, cathedral felt → toy piano, candy keys, sunny parlor, fairy cottage |
| Bells | 120 | cathedral tower, drowned bell, ghost handbell → music box, star glockenspiel, fae bellflower |
| E-Piano | 120 | velvet tine, dream rhodes → bark wurly, crunch rhodes, growl tine, industrial tine, furnace keys |

Each category = 12 base characters × 10 coats (Plain, Veiled, Gilded, Dusty, Velvet, Glass, Wide, Short, Bloom, Grit).

Knobs (every one is a rose): Brightness, Decay, Attack, Release, Space, Chorus, Tremolo, Volume.

## Build

Requires CMake 3.22+, a C++17 compiler, and git (JUCE 8 is downloaded automatically).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The VST3 is copied to your system VST3 folder after the build
(`COPY_PLUGIN_AFTER_BUILD`). The Standalone app is in `build/Keydrobe_artefacts/`.

- Windows: Visual Studio 2022 ("Desktop development with C++")
- macOS: Xcode command line tools
- Linux: JUCE's Linux dependencies (libasound2-dev, libfreetype-dev, libx11-dev, etc.)

Notes
- Output format is VST3 (VST2 is discontinued by Steinberg).
- The sound is fully synthesized (no samples): inharmonic additive strings for piano,
  inharmonic partials + FM shimmer for bells, FM tine/body + saturation for e-piano.
- Adding presets: edit the base tables or `coats` in `Source/Presets.h`.

## Automated builds (GitHub Actions)

`.github/workflows/build.yml` builds Windows, macOS (universal arm64 + x86_64) and Linux automatically.

1. Create a GitHub repo and push this folder (`git init && git add . && git commit -m "Keydrobe" && git remote add origin <url> && git push -u origin main`).
2. Every push / pull request builds the plugin. Download the result from the run's **Artifacts** section (Actions tab).
3. To publish a release, tag a version: `git tag v1.0.0 && git push --tags`.
   The workflow attaches `Keydrobe-Windows.zip`, `Keydrobe-macOS.zip` and `Keydrobe-Linux.zip` to a GitHub Release.
4. You can also run it by hand: Actions tab → Build Keydrobe → Run workflow.

Notes
- macOS builds are unsigned: after unzipping, run `xattr -dr com.apple.quarantine Keydrobe.vst3` or right-click → Open. Signing/notarization needs an Apple Developer account and secrets added to the repo.
- Local builds still copy the plugin to your plugin folder; CI turns that off via `-DKEYDROBE_COPY_AFTER_BUILD=OFF`.
