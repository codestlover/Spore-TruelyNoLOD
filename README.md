
# Truely!NO-LOD

A *Spore* mod that turns off the game's level of detail (LOD): models, their materials and the plants on planets are
drawn in full detail at any distance instead of switching to simpler versions or disappearing as the camera moves
away.

Current version: **1.1.0**. Download `TruelyNoLOD.sporemod` from the
[Truely!NO-LOD 1.1.0](../../releases/tag/v1.1.0) release.

This is the `main` branch: the level of detail only, with no options of its own. Truely!NO-LOD:Enchanted, the same mod
with real reflections in the water of planets and a *Water reflections* row in the graphics options, is on the
[`enchanted`](../../tree/enchanted) branch. Each branch has releases of its own.

## What it changes

Spore lowers detail with distance in several separate places. Truely!NO-LOD changes each of them:

- **Model meshes.** Models come with up to four meshes, from the full model (level 0) down to rough low-poly
  versions, and a list of camera distances; every frame the game picks a mesh by the distance, relative to the
  model's size, and past the last distance a model can run out of meshes and is not drawn at all. With Truely!NO-LOD every
  model keeps level 0 at any distance. When the game asks for a fixed reduced level (1 to 3) for a model, it gets
  level 0 instead; a request for level 4 or higher selects no mesh at all and is left alone, so a model the game
  hides stays hidden.
- **High-resolution meshes.** Some models, such as buildings and vehicles, also have a more detailed
  high-resolution mesh, which the game draws only for the few nearest of them each frame. With Truely!NO-LOD every model
  that has one draws it.
- **Materials.** Past four camera distances the game switches models to simpler material and shader variants. With
  Truely!NO-LOD the shaders always get the most detailed variant.
- **Plants, rocks, grass and other scattered objects.** The plants, trees, rocks and grass on a planet are
  scattered over the surface by the game's effects (Swarm). Such an effect has levels by camera distance: near the
  camera it places the plant's model, farther away a rougher model, and far away flat sprites or nothing at all;
  grass thins out and disappears; whole regions swap their plant and rock effects for simpler ones. Truely!NO-LOD keeps
  every effect whose farther levels switch off something its nearest level shows on that nearest level, so it stays
  everywhere: effects that scatter models or place single models, and effects made only of scattering, whether they
  scatter models, sprites or other effects. The models those effects draw are reduced meshes of each plant (level 1-3); Truely!NO-LOD loads the plant's
  level 0 mesh instead, taken from the model's own property list.
- **A model's own effects.** Smoke, glow and other effects attached to a model are switched off past the model's
  effect range. With Truely!NO-LOD they run at any distance.
- **Rocks and grass seen from high up.** On planets, terrainDistribute effects place rocks, grass and other ground
  cover in cells around the camera, finer cells nearer, up to distances that count the camera's height; from high up
  the fine cells with rocks are never reached and the rocks disappear. Truely!NO-LOD sets those distances each frame to
  reach as far across the ground from the camera's height as they do on the ground plus that height, at most twice
  as far, so the ground under the view keeps its rocks and the extra work stays bounded.
- **Planet terrain.** With the High Planet Detail option, before a planet's terrain is set up, Truely!NO-LOD raises
  its quality settings to what the planet resolution component of BruteLOD sets: a terrain chunk resolution of 32
  instead of 16, the quality range from 1200 to 120000, and the seabed drawn as land. Settings already higher are
  kept. This is costly on the video card; Medium and Low keep the game's terrain.
- **Shadows.** With the High shadow option, which turns on the game's two nested shadow cascades, the game still
  lowers their detail with distance: close up it narrows the area they cover, so shadows are cut off at its edges;
  far away it drops the detail of the inner cascade and, below a threshold, the outer cascade altogether; and plants
  and rocks placed by effects cast into the outer cascade only when they are marked for it. Truely!NO-LOD gives both
  cascades the widest area and the most detail the game uses at any camera distance, makes the shadow maps
  2048x2048, lets every effect cast into both, and keeps the depth the shadow camera covers on the point the camera
  looks at and as deep as the area is wide, so large objects keep their whole shadows. Larger maps are kept. Medium
  keeps the game's own shadows, for a lighter load, and Low turns them off as before.

Models are still culled when they are outside the view.

## What it does not change

- **Planets seen from space** are still drawn as simplified planet images, and the terrain still subdivides by
  distance within its raised settings.
- **Textures** follow the in-game *Texture Detail* option.
- **Animation, simulation and the game's quality options** are not touched.
- **Other effects** keep their own distance levels. In Spore these levels also decide what an effect shows at each
  distance: the galaxy's dust and glow, screen flashes and camera shakes near explosions and events. Holding every
  effect at its near level fills the galaxy view with dust and flashes the screen for events anywhere, so Truely!NO-LOD
  only holds effects that place or scatter models, or that are made only of scattering. When such an effect also
  shakes the camera, flashes the screen, plays sound or sends game events, it keeps its levels, but its decals and
  models that its near level shows stay on at every distance, such as the craters of spice geysers.
- **Cell stage:** the cell stage has its own limit on how many objects get high detail in the foreground and the
  background (`numHighLOD_FG` and `numHighLOD_BG`). Truely!NO-LOD does not change it.

## Performance

Detailed meshes, detailed materials and the models and effects that used to vanish in the distance are now drawn
as well, so the video card has more to do; the larger shadow maps take about 70 MB of video memory, and every
frame draws the shadow casters twice, once per cascade, plants and rocks included. Plants and rocks are drawn as full models and grass at full density
over the whole planet instead of sprites or nothing in the distance, so planets with a lot of plants are the
heaviest: the frame rate can drop noticeably
there, in big cities and in the space stage, and entering a planet can take longer. Plant particles load the
plant's full mesh instead of the reduced one, once per plant type.

## Installation

1. Install the [Spore ModAPI Launcher Kit](https://davoonline.com/sporemodder/rob55rod/ModAPI/Public/index.html)
   if you do not have it yet.
2. Open `TruelyNoLOD.sporemod` with **Spore ModAPI Easy Installer**.
3. Start the game with **Spore ModAPI Launcher**. If the game was already running, restart it completely.

To remove the mod, use **Spore ModAPI Easy Uninstaller → Truely!NO-LOD**. The mod stores nothing in saves, so removing it
simply brings the normal level of detail back. Truely!NO-LOD 1.1.0 adds no options and writes no files; the
`%APPDATA%\TruelyNoLOD` folder that 1.0.0 kept its water reflections setting in can be deleted.

Truely!NO-LOD supports *Spore Galactic Adventures* with the March 2017 patch (`SporeApp_ModAPIFix.exe`, the version the
ModAPI Launcher Kit runs). On any other game build it does not change anything and shows a message instead; the
game then runs with its normal level of detail.

## Other mods

Truely!NO-LOD works together with mods that hook the same game functions. When another mod's jump into its own module is
already at the start of a function Truely!NO-LOD hooks, Truely!NO-LOD puts its hook in front of it and calls through, so
both run in any load order. The other changes are made in place inside game functions; Truely!NO-LOD
checks that code byte for byte first and leaves out only a change whose code something else has changed. It then
shows a message.

## How it works

The mod is a 32-bit DLL loaded by the ModAPI Launcher. After the game has started it checks the game executable
(image size and time stamp) and the exact bytes of the code it changes, then:

- replaces the distance walk of the per-frame model world update with `mLodIndex = 0`, right after the update frame
  is recorded and before the game picks the mesh for the current level;
- detours `IModelWorld::SetLOD` so that a fixed level from 1 to 3 becomes 0;
- removes the HighRes flag check from the two places that pick a model's mesh, the update and the draw pass,
  together, so the update animates the mesh that is drawn;
- makes the draw pass pass material level 0 to the shaders;
- detours the effect renderer's model batch creation and its single model creation: when the mesh either is asked
  to load is level 1, 2 or 3 of a model, as that model's property list names it (`modelMeshLOD1-3`, or level 0
  named there too), it loads the list's `modelMeshLOD0` instead;
- detours the effects world's level switch: the first time it meets an effect whose farther levels switch off a
  model, scattered models or, in an effect made only of scattering, any scattering its nearest level shows, it sets every level distance after the first in the effect's description to the largest
  float, so the game's own distance walk keeps that effect on level 1 with no fade; in an effect that also shakes the
  camera, flashes the screen, plays sound or sends game events, it instead makes the decal and model blocks its
  nearest level shows last to the farthest level;
- makes the update keep a model's own effects on at any distance;
- detours the terrain distribute grid's update to keep its level distances reaching across the ground from height;
- detours the terrain sphere's setup to raise the planet quality settings in the game's application properties
  first;
- raises the shadow map sizes in the application properties and has the shadow world make its maps again, as it
  does when its settings change; and
- detours the curve evaluator so that the shadow world's update reads the widest shadow view, the largest cascade
  zoom and the strength the game has close up at any camera distance, and keeps casters at least as deep as the
  widest view's half width; makes the shadow world's effects registration flag effects for both cascades alike; and
  sets the factor that keeps the shadow camera's depth range on the point the camera looks
  at, which the game leaves at 0.

The addresses and original bytes are in [`src/core/Hooks.hpp`](src/core/Hooks.hpp).

## Building from source (Linux)

> **A note from the author:** I build and test this mod only on Linux, and I cannot help with a Windows build.
> To play the mod you don't need to build anything: just use the `.sporemod` from Releases.

The build is the same as for [Civ Challenge](https://github.com/codestlover/Spore-CivChallenge), and the scripts in
`cmake/` are shared with it, which is why their settings start with `CIV_`. You need LLVM (`clang-cl`, `lld-link`,
`llvm-lib`, `llvm-mt`, `llvm-rc`), CMake 3.20 or newer, `make`, GNU `bash` 4.4 or newer, coreutils, findutils, grep
with `-P`, `git`, `curl`, `unzip`, `tar` and `jq`.

```sh
git clone https://github.com/codestlover/Spore-TruelyNoLOD.git && cd Spore-TruelyNoLOD
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clangcl-i686.cmake \
    -DCIV_ACCEPT_MSVC_LICENSE=ON
cmake --build build
```

This produces `build/bin/TruelyNoLOD.dll`. The first `cmake` run downloads the dependencies into `deps/` with
`cmake/setup.sh`: the Spore ModAPI SDK and `SporeModAPI.lib` (v2.5.559), Microsoft Detours v4.0.1 and, with xwin,
the MSVC CRT and Windows SDK, which come under the Microsoft license and are not redistributed.
`-DCIV_ACCEPT_MSVC_LICENSE=ON` means you accept that license. If you already prepared these dependencies for Civ
Challenge, set `CIV_DEPS` to that `deps` folder (an absolute path) before the first `cmake` run to reuse them. With
CMake 3.27 or newer the build is reproducible and gives the same DLL as the one in the release.

Every text file starts with exactly one empty line and ends with exactly two; `cmake/layout.sh` checks this on each
build, and `bash cmake/layout.sh --fix` fixes it.

## Testing

Releases are checked with tests that are not part of this repository:

- an emulator suite that runs each piece of the game's own level-of-detail code from `SporeApp_ModAPIFix.exe`
  before and after the change, together with the compiled hooks and the patch table from the released DLL; and
- a Wine test that installs the hooks and the code changes with the real Windows APIs and chains a hook with
  another mod's.

These checks do not replace playing. The mod has been tried in-game, but not played through every stage.

## Credits and license

Truely!NO-LOD is licensed under **GPL-3.0-or-later** (see [`LICENSE`](LICENSE)).

- [Microsoft Detours](https://github.com/microsoft/Detours), MIT, fetched by `cmake/setup.sh`.
- [Spore ModAPI SDK](https://github.com/Spore-Community/Spore-ModAPI) by Eric Mor and contributors,
  GPL-3.0-or-later: the mod links `SporeModAPI.lib`.

Full notices are in [`NOTICE.md`](NOTICE.md) and [`licenses/`](licenses). Spore and Galactic Adventures belong to
Electronic Arts/Maxis. This repository and the release contain no game executables, game assets, save data or
decompiled game code.

