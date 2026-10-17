# Hide Infinite Counts

A [Dusklight](https://github.com/TwilitRealm/dusklight) mod that hides resource counts and meters
while the corresponding infinite-resource cheats are enabled. Item icons remain visible.

Supported cheats:

- **Infinite Arrows, Seeds, and Bombs:** hides the matching equipped-item and item-ring ammo counts.
- **Infinite Oil:** hides lantern oil gauges, including the item-ring gauge.
- **Infinite Oxygen:** hides the oxygen meter.
- **Infinite Rupees:** hides rupee digits, preserving the rupee icon and key display.
- **Infinite Hearts:** hides the heart meter.

Bomb-arrow ammo remains visible unless **both** Infinite Arrows and Infinite Bombs are enabled,
since either finite resource can still run out. Unrelated counts, such as bottled bait, are unchanged.
The mod reads the effective cheat settings at draw time; toggling a cheat restores its display
without reloading the mod. It does not enable cheats or change resource quantities.

## Quick start

1. Build locally:
   ```sh
   cmake -B build
   cmake --build build
   ```

2. Copy `build/mods/hide_infinite_counts.dusk` into the game's mods folder:

- Windows: `%APPDATA%\TwilitRealm\Dusklight\mods`
- Linux: `~/.local/share/TwilitRealm/Dusklight/mods`
- macOS: `~/Library/Application Support/TwilitRealm/Dusklight/mods`

3. Enable **Hide Infinite Counts** in the in-game mod manager, then enable the desired cheats.

During development, rebuild, copy and click **Reload** in the mod manager to pick up changes.

### Manual verification

With cheats disabled, confirm normal counts and meters are visible. Toggle each supported cheat
individually and verify only its count/meter disappears, with item icons and unrelated counts intact.
Check both equipped-item buttons and selected/unselected items in the item ring. Test bomb arrows
with each cheat separately and with both enabled. Toggle cheats off and disable/reload the mod to
confirm normal rendering returns, including previously hidden leading digits and HUD fades.

This template has no automated test suite; build with the commands above and verify rendering in-game.

> [!IMPORTANT]
> A mod built locally will only be valid for your own platform, and shouldn't be distributed.
> The repository will build a [cross-platform bundle](#github-actions) for distribution. See below.

## Updating to a new Dusklight version

Change the `DUSKLIGHT_VERSION` line in `CMakeLists.txt` to the new release tag (or commit hash) and reconfigure. The
pinned version is fetched into `dusklight/` automatically. Use the `dusklight/` checkout to browse game code, headers
and mod services.

> [!IMPORTANT]
> The Dusklight checkout is for **reference only**. Mods use
> [services](https://github.com/TwilitRealm/dusklight/blob/main/docs/modding.md#built-in-services) and
> [hooks](https://github.com/TwilitRealm/dusklight/blob/main/docs/modding.md#hooking-game-functions) to interact with
> game code.

## GitHub Actions

The included GitHub Actions workflow builds the mod for the following platforms:
- Windows (AMD64 & ARM64)
- macOS (Apple Silicon & Intel)
- iOS (Apple Silicon)
- Linux (x86_64 & aarch64)
- Android (aarch64)

It then merges the per-platform builds into a single `.dusk` supporting all platforms. (Artifact `mod-combined`) 

Pushing a tag to the repository creates a GitHub release with the combined bundle.

## For Dusklight developers

Point the build at an existing checkout instead of fetching one:

```sh
cmake -B build -DDUSKLIGHT_DIR=~/path/to/dusklight
```
