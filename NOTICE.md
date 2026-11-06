
Truely!NO-LOD:Enchanted 1.0.0 is licensed under GPL-3.0-or-later.
The full license and warranty disclaimer are in LICENSE and licenses/GPL-3.0.txt.

Spore ModAPI SDK: Copyright Eric Mor and contributors; GPL-3.0-or-later.
The DLL links SporeModAPI.lib from the v2.5.559 release, which cmake/setup.sh fetches into deps/modapi.
src/core/Allocator.cpp: Copyright (C) 2018 Eric Mor; GPL-3.0-or-later. It is adapted from the SDK
SourceCode/Allocator.cpp by way of Civ Challenge, with include separator and noexcept compatibility changes.
SDK source: https://github.com/Spore-Community/Spore-ModAPI

EASTL and EABase: Copyright (C) 2015 Electronic Arts Inc.; BSD 3-Clause License.
Their headers come with the Spore ModAPI SDK and are compiled into the DLL.
The full license is in licenses/EASTL-BSD.txt.
Source: https://github.com/electronicarts/EASTL

Microsoft Detours: Copyright Microsoft Corporation; MIT License.
The compiled DLL links Detours v4.0.1 (commit e4bfd6b) built from its sources, which cmake/setup.sh
fetches into deps/Detours; the Source.zip inside the release .sporemod includes them.
The license is in licenses/Detours-MIT.md.
Source: https://github.com/microsoft/Detours

src/core/HookChain.hpp and src/core/SdkCompat.hpp come from Civ Challenge by the same author, GPL-3.0-or-later.
The build scripts in cmake/ are shared with Civ Challenge.
Source: https://github.com/codestlover/Spore-CivChallenge

The water shaders in src/shaders are derived from the planet water shader fragments of Spore and Spore Galactic
Adventures by Maxis/Electronic Arts, with the reflection added. The game's code and assets remain the property of
their rights holders; the license above covers the changes and the rest of this project.

Spore and Galactic Adventures are owned by Electronic Arts/Maxis. No game executable, save data or decompiled game
code are included in the mod.

