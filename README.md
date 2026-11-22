
# Truely!NO-LOD:Enchanted

An edition of Truely!NO-LOD with real reflections in the water of planets. It is the same mod with the same ID, so
installing it replaces plain Truely!NO-LOD, and the other way round.

This is the `enchanted` branch. Plain Truely!NO-LOD, without the reflections and without an options row, is on the
[`main`](../../tree/main) branch. Each branch has releases of its own.

A *Spore* mod that turns off the game's level of detail (LOD): models, their materials and the plants on planets are
drawn in full detail at any distance instead of switching to simpler versions or disappearing as the camera moves
away.

Current version: **1.0.2**. Download `TruelyNoLOD-Enchanted.sporemod` from the
[Truely!NO-LOD:Enchanted 1.0.2](../../releases/tag/enchanted-v1.0.2) release.

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
  cascades the widest area and the most detail the game uses at any camera distance, makes the shadow map
  2048x2048 (the basis map keeps the game's size: with any other size the editors' Paint mode shows the creation
  black), lets every effect cast into both, and keeps the depth the shadow camera covers on the point the camera
  looks at and as deep as the area is wide, so large objects keep their whole shadows. Larger maps are kept. Medium
  keeps the game's own shadows, for a lighter load, and Low turns them off as before.
- **Water reflections.** Spore's water only shines with the sky from the planet's atmosphere map, and the reflection
  pass the game has never reaches the water. With *High* in the *Water reflections* row of the graphics options, in
  every language Spore ships in, Truely!NO-LOD:Enchanted draws the terrain, plants, rocks, buildings, creatures,
  vehicles and effects once more each frame through a camera mirrored in the water, into a picture and a depth map of
  its own. For each point of the water the shader then follows the real reflected ray from the curved surface of the
  planet through that depth map until it meets something, so reflections start where an object meets the water and
  keep their place all over the visible water, however small the planet. Waves sway the picture; where a ray meets
  nothing the water shows the sky from the atmosphere map. The mirror image comes in before the fog, under the sun's
  glitter, and gets stronger toward a grazing view. Nothing more is drawn while no water is in view, high above the
  planet, or on lava and ice planets. *Low* leaves the water to the game. The setting takes effect at once, is on by
  default and is kept in `%APPDATA%\TruelyNoLOD\settings.ini`.

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
as well, so the video card has more to do; the larger shadow map takes extra video memory, and every
frame draws the shadow casters twice, once per cascade, plants and rocks included. Plants and rocks are drawn as full models and grass at full density
over the whole planet instead of sprites or nothing in the distance, so planets with a lot of plants are the
heaviest: the frame rate can drop noticeably
there, in big cities and in the space stage, and entering a planet can take longer. Plant particles load the
plant's full mesh instead of the reduced one, once per plant type. With *Water reflections* on *High*, every frame
with water in view draws the scene a second time, plants included, so that view costs noticeably more; *Low* takes all
of it away at once.

## Installation

1. Install the [Spore ModAPI Launcher Kit](https://davoonline.com/sporemodder/rob55rod/ModAPI/Public/index.html)
   if you do not have it yet.
2. Open `TruelyNoLOD-Enchanted.sporemod` with **Spore ModAPI Easy Installer**.
3. Start the game with **Spore ModAPI Launcher**. If the game was already running, restart it completely.

To remove the mod, use **Spore ModAPI Easy Uninstaller → Truely!NO-LOD:Enchanted**. The mod stores nothing in saves, so removing it
simply brings the normal level of detail back; its water reflections setting stays in `%APPDATA%\TruelyNoLOD`.

Truely!NO-LOD supports *Spore Galactic Adventures* with the March 2017 patch (`SporeApp_ModAPIFix.exe`, the version the
ModAPI Launcher Kit runs, and the `SporeApp.exe` of the Steam copy, the same game build). It works on Windows as well
as under Wine: Windows usually loads the game at another address than the one it was built for (ASLR), on Steam, EA
app and GOG copies alike, and since Truely!NO-LOD:Enchanted 1.0.2 the mod follows it there. On any other game build
it does not change anything and shows a message instead; the game then runs with its normal level of detail.

## Other mods

Truely!NO-LOD works together with mods that hook the same game functions. When another mod's jump into its own module is
already at the start of a function Truely!NO-LOD hooks, Truely!NO-LOD puts its hook in front of it and calls through, so
both run in any load order. The other changes are made in place inside game functions; Truely!NO-LOD
checks that code byte for byte first and leaves out only a change whose code something else has changed. It then
shows a message.

[Seamless Water](https://github.com/codestlover/Spore-SeamlessWater) 1.1.0 or newer is linked to it directly. Seamless
Water builds each of its water shaders a second time with the reflection in it, from the same source, and hands them
over through the exported function `SeamlessWater_Link`, so a change to its water can never leave the reflections
behind, and the two mods load in any order. Both sides check a contract number; with an older Seamless Water the water
keeps its seamless look without reflections. Water shaders the mod does not know are left alone in the same way.

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
- adds the water reflections row to the graphics options when they open, and keeps the game's own reflection cube
  switched off, which nothing the water draws ever reads;
- detours the terrain's pass for water reflection and refraction: after the game's own work there, when water is in
  view, it draws layer 8 (the terrain, without its water or seabed), the model worlds of layers 10, 12, 13 and 20,
  the creatures of layers 14 and 21 and the effects of layer 18 through the game's render manager with a camera of
  its own, mirrored in the tangent plane of the water under the view, with its depth in an INTZ texture, and puts the
  terrain's camera state back. Layer 13 draws the planet's own model world, with its cities, vehicles, ships and
  rocks. During that pass the model worlds see no planet horizon and no occluders, which would hide nearly every
  model from a camera inside the planet; the terrain is clipped at the tangent plane, so the skirts between its
  chunks, which hang under the water, cannot cover objects in the mirror, and everything else is clipped a little
  lower, so objects far out on the water keep their hulls;
- while the terrain draws its water, a hook on Direct3D's indexed draw swaps the water pixel shader for the version
  with `src/shaders/WaterReflection.hlsli` in it, binds the mirror picture and depth to samplers 6 and 9 and the mirror
  camera to constants c50 to c54, and puts the device state back after the draw. The shader walks the reflected ray
  across the mirror picture at most one and a half texels at a time, jumps ahead to the depth it sees when that is
  closer, and takes a hit behind a surface only where the ray has just crossed it, so thin parts such as masts stay
  whole and no ghost copies appear beside objects;
- raises the shadow map size (not the basis map's) in the application properties and has the shadow world make
  its maps again, as it does when its settings change; and
- detours the curve evaluator so that the shadow world's update reads the widest shadow view, the largest cascade
  zoom and the strength the game has close up at any camera distance, and keeps casters at least as deep as the
  widest view's half width; makes the shadow world's effects registration flag effects for both cascades alike; and
  sets the factor that keeps the shadow camera's depth range on the point the camera looks
  at, which the game leaves at 0.

The addresses and original bytes are in [`src/core/Hooks.hpp`](src/core/Hooks.hpp). The addresses are the game's at
the base it was built for, 0x400000; where Windows loads the game at another base (ASLR), the mod moves every address
by the same amount. The loader then has also moved the absolute addresses inside the game's code, so the mod reads the
code it checks as the executable file has it, those addresses moved back through the executable's relocation table
([`src/core/Relocation.hpp`](src/core/Relocation.hpp)).

## Building from source (Linux)

> **A note from the author:** I build and test this mod only on Linux, and I cannot help with a Windows build.
> To play the mod you don't need to build anything: just use the `.sporemod` from Releases.

The build is the same as for [Civ Challenge](https://github.com/codestlover/Spore-CivChallenge), and the scripts in
`cmake/` are shared with it, which is why their settings start with `CIV_`. You need LLVM (`clang-cl`, `lld-link`,
`llvm-lib`, `llvm-mt`, `llvm-rc`), CMake 3.20 or newer, `make`, GNU `bash` 4.4 or newer, coreutils, findutils, grep
with `-P`, `git`, `curl`, `unzip`, `tar` and `jq`.

```sh
git clone -b enchanted https://github.com/codestlover/Spore-TruelyNoLOD.git && cd Spore-TruelyNoLOD
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clangcl-i686.cmake \
    -DCIV_ACCEPT_MSVC_LICENSE=ON
cmake --build build
```

This produces `build/bin/TruelyNoLOD.dll`. The first `cmake` run downloads the dependencies into `deps/` with
`cmake/setup.sh`: the Spore ModAPI SDK and `SporeModAPI.lib` (v2.5.559), Microsoft Detours v4.0.1 and, with xwin,
the MSVC CRT and Windows SDK, which come under the Microsoft license and are not redistributed.
`-DCIV_ACCEPT_MSVC_LICENSE=ON` means you accept that license. If you already prepared these dependencies for Civ
Challenge, set `CIV_DEPS` to that `deps` folder (an absolute path) before the first `cmake` run to reuse them.

The water shaders in `src/shaders` are compiled during the build by `src/shaders/ShaderCompiler.cpp`, a small
Windows program that runs under Wine and uses Microsoft's 32-bit `d3dcompiler_47.dll`. That DLL is not
redistributed: pass its path with `-DTRUELY_D3DCOMPILER=/path/to/x86/d3dcompiler_47.dll` on the first `cmake` run,
and have `wine` in `PATH`. The release was built with version 10.0.22621.3233. With CMake 3.27 or newer and that
compiler the build is reproducible and gives the same DLL as the one in the release.

Every text file starts with exactly one empty line and ends with exactly two; `cmake/layout.sh` checks this on each
build, and `bash cmake/layout.sh --fix` fixes it.

## Testing

Releases are checked with tests that are not part of this repository:

- an emulator suite that runs each piece of the game's own level-of-detail code from `SporeApp_ModAPIFix.exe`
  before and after the change, together with the compiled hooks and the patch table from the released DLL;
- a Wine test that installs the hooks and the code changes with the real Windows APIs and chains a hook with
  another mod's;
- two Direct3D 9 tests under Wine and DXVK: one draws a scene on a sphere through the mirror camera and compares every
  pixel of the water with a ray traced reference, and one draws water with all the compiled water shaders and checks
  that the reflection reaches them and that at strength 0 the picture is the original one; and
- a relocation check that maps `SporeApp_ModAPIFix.exe` and Steam's `SporeApp.exe` (its code decrypted) at 0x400000
  and at moved bases, with the executable's relocations applied as Windows applies them, and reads every piece of
  code the mod checks, the water reflections' and the options row's included, with the DLL's own reading code.

The emulator suite also checks that every game function the mirror pass calls pops the arguments the mod passes, and
runs the pass and the water draw against a recording Direct3D device.

These checks do not replace playing. The mod has been tried in-game, but not played through every stage, and the
reflections have been seen on a few planets only.

## Credits and license

Truely!NO-LOD is licensed under **GPL-3.0-or-later** (see [`LICENSE`](LICENSE)).

- [Microsoft Detours](https://github.com/microsoft/Detours), MIT, fetched by `cmake/setup.sh`.
- [Spore ModAPI SDK](https://github.com/Spore-Community/Spore-ModAPI) by Eric Mor and contributors,
  GPL-3.0-or-later: the mod links `SporeModAPI.lib`.

Full notices are in [`NOTICE.md`](NOTICE.md) and [`licenses/`](licenses). Spore and Galactic Adventures belong to
Electronic Arts/Maxis. This repository and the release contain no game executables, save data or decompiled game
code. The water shaders in `src/shaders` are the exception among game assets: they are the game's own water pixel
shaders, from the shader fragments Spore ships, with the reflection added.

