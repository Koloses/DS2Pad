# DS2Pad

Xbox (XInput) controller support for **Dungeon Siege II: Broken World** (`DungeonSiege2.exe` version 2.30), aimed at playing it like a twin-stick action RPG: you steer one hero directly with the left stick, the rest of the party follows and fights alongside.

It is a single DLL. No game files are modified on disk.

> Single-player only. The mod places the hero itself every frame, which a multiplayer session would not accept.

## Features

- **Direct 360° movement** of the controlled hero with the left stick, checked against the game's own walk mesh. Area triggers (saves, region loads, scripted events) still fire.
- **Party follow**: companions keep up with the controlled hero and, in mirror orders, attack what it attacks.
- **Targeting**: the nearest enemy in the stick direction is highlighted and attacked; enemies take priority over friendly characters and interactables. Target cycling on R3.
- **Party auto potions**: companions drink their own health or mana potions below a threshold (default 30%). The controlled hero never does.
- **Potion sharing**: a companion with no potion left is handed one by a party member that has more than a set minimum.
- **Auto pickup** of gold and of potions near the controlled hero, each with its own switch. Potions you dropped yourself are left alone.
- **Auto resurrect**: a companion carrying a resurrect scroll uses it on a dead party member once no enemy is near. The controlled hero never casts on its own.
- **Ground item stats**: a stats box for the targeted item on the ground (equippable items only).
- **Compare with equipped**: item stats boxes (ground, inventory, shops) also list what the hero wears in the same slot.
- **Menu navigation**: d-pad / left stick snaps between buttons and inventory slots, right stick moves the cursor freely.
- **UI scaling** (default 1.5×) for high resolutions; the 3D scene is untouched.
- **Camera lock** on the controlled hero.
- **Mouse pointer hidden** during play while the pad is in use; it comes back in menus or when the mouse is moved.
- **Frame rate fix**: removes the slow processor-identification calls the copy protection makes thousands of times a frame.
- **Frame pacing**: frames are released on an even schedule matched to the display, which removes judder at a steady 60 fps.
- **No battery prompt**: the game is told it runs on mains power.

Every feature has a switch in `ds2pad.ini`.

## Controls

### In game

| Input | Action |
|---|---|
| Left stick | Move the controlled hero |
| Right stick | Rotate / tilt camera |
| D-pad up / down | Zoom in / out |
| D-pad left / right | Previous / next party member |
| A or RT (hold) | Attack the highlighted target |
| X (tap) | Pick up / use / talk to the nearest thing |
| X (hold) | Whole party collects loot |
| B | Stop, drop the target lock |
| Y | Cycle the active weapon / spell |
| LB / RB | Drink health / mana potion |
| R3 | Cycle target |
| L3 | Map |
| Back | Inventory |
| Start | Game menu |
| LT + A, B, X, Y | Powers 1–4 |
| LT + d-pad left / right | Party orders: mirror / rampage |
| LT + LB | Skills |
| LT + RB | Spell book |
| LT + Back | Journal |
| LT + R3 | Pause |
| L3 + R3 together | Force gameplay mode on / off (way out if a panel is mistaken for a menu) |

### In menus and the inventory

| Input | Action |
|---|---|
| D-pad / left stick | Snap to the next button or slot |
| Right stick | Move the cursor freely |
| A / X | Left / right mouse button |
| LT / RT (hold) | Shift / Ctrl |
| B or Start | Close (Esc) |
| Y | Spell book |
| Back | Inventory |
| LB / RB | Drink health / mana potion |
| L3 / R3 | Map / pause |

During cutscenes A, B or Start sends Esc.

Keyboard-driven actions are sent as key presses, so the `[Keys]` section of `ds2pad.ini` has to match your in-game key bindings (it ships with the game defaults).

## Requirements

- Dungeon Siege II: Broken World, `DungeonSiege2.exe` **version 2.30**. The mod uses fixed addresses inside that executable; other builds are not supported.
- An XInput controller.
- Developed and tested on Windows. Other setups have not been tried.

## Install

The game loads its sound library `mss32.dll` from its `system` folder. DS2Pad takes that name and passes every sound call on to the original, which has to be renamed.

1. Download the release zip and put the `DS2Pad` folder from it inside the game folder (next to `DungeonSiege2.exe`).
2. Run `DS2Pad\install.bat`.

Or by hand:

1. In the game's `system` folder rename `mss32.dll` to `mss32o.dll`.
2. Copy the mod's `mss32.dll` into `system`, and `ds2pad.ini` into the game folder.

To uninstall, run `DS2Pad\uninstall.bat`, or delete the mod's `system\mss32.dll` and rename `mss32o.dll` back to `mss32.dll`.

## Settings

`ds2pad.ini` in the game folder; every key has a comment. The main ones:

| Key | Default | Meaning |
|---|---|---|
| `MoveMode` | 2 | 2 = direct movement, 0 = plain move orders |
| `EnemySpacing` | 1.2 | How close (metres) the hero may walk to enemies; 0 = walk through |
| `PartyFollow` / `FollowDistance` | 1 / 3.5 | Companions follow once further away than this many metres |
| `PartyAutoPotion` / `PartyAutoPotionPercent` | 1 / 30 | Companions drink their own potions below this percentage |
| `PotionShare` / `PotionShareKeep` | 1 / 3 | Potion sharing; a member only gives while it has more than this many |
| `AutoPickupGold` / `AutoPickupPotions` / `AutoPickupRadius` | 1 / 1 / 2.5 | Auto pickup, each on its own, and its reach in metres |
| `AutoResurrect` | 1 | Companions use resurrect scrolls on dead party members |
| `GroundItemInfo` / `CompareWithEquipped` | 1 / 1 | Ground item stats box; equipped item listed in stats boxes |
| `UIScale` / `UIScaleMinWidth` | 1.5 / 1280 | Interface scale; 1.0 = off |
| `CameraLockToLeader` | 1 | Camera stays on the controlled hero |
| `HideCursorWithPad` / `PointerWakePixels` | 1 / 40 | Pointer hiding and how far the mouse must move to bring it back |
| `FrameRateFix` | 1 | Copy-protection slowdown removed |
| `FramePacing` / `FramePacingHz` | 1 / 0 | Even frame schedule; 0 = the display's refresh rate |
| `IgnoreBattery` | 1 | No "running on battery" prompt |
| `StickXSign` | 0 | Set to 1 or -1 if left and right are mirrored |
| `Log` | 1 | Write `ds2pad.log` in the game folder; 0 turns it off |

## Building

One C file plus two generated headers, built with 32-bit MinGW-w64:

```
./build.sh
```

or directly:

```
i686-w64-mingw32-gcc -O2 -fno-omit-frame-pointer -Wall -Wextra -shared -static -static-libgcc \
    -o mss32.dll src/ds2pad.c src/mss32.def -lwinmm
```

`src/mss32.def` and `src/proxy_gen.h` pass the Miles Sound System exports on to `mss32o.dll`; `src/cpuid_sites.h` lists the call sites patched by `FrameRateFix`.

## How it works

- `system\mss32.dll` is a proxy for the game's sound library, which gets the mod loaded into the game process.
- A hook on `PeekMessageA` in the game's import table gives a callback on the main thread; hooks on the Direct3D 9 device (`Present`, `ShowCursor`, `Clear`, `SetViewport`) drive frame pacing, pointer hiding and UI scaling.
- The game is driven through its own exported script-binding (FuBi) functions, such as `GoMind::RSMove`, plus a small number of hooked functions, patched call sites and direct reads of engine structures at fixed addresses. That is why only the 2.30 executable works.
- Direct movement tests each step against the walk mesh of the terrain pieces and places the hero exactly.
- Menu actions are synthesized mouse and keyboard input.

`tools/` holds the small Python helpers used while reverse engineering (`pefile` and `capstone` needed). `dz.py` and `xr.py` expect `DungeonSiege2.exe` in the current directory; run `exports.py` first to produce `exports.txt`. `tank.py` reads the game's `.ds2res` archives.

## Troubleshooting

- `ds2pad.log` in the game folder records what the mod found and did. The first line names the build.
- Left and right swapped: set `StickXSign`.
- A button does nothing: check that `[Keys]` matches your in-game bindings.
- The pad acts as if a menu were open during play: click both sticks together.

## AI disclosure

The code in this repository, the reverse engineering behind it and this README were written by an AI assistant (Anthropic's Claude), directed by the repository owner, who set the requirements and tested builds by playing the game. The code has not been reviewed line by line by a human, so treat it accordingly.

## Disclaimer

Unofficial fan project, not affiliated with or endorsed by Gas Powered Games or Microsoft. No game code or assets are included; you need your own copy of the game.
