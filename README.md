<img align="left" width="100" height="100" src="doc/kstorm_256.png" alt="Logo of KStorm viewer"/>

# KStorm - A research viewer

KStorm is a personal research viewer for Second Life by Kissmy Spicoli: an unsupported experiment. There are no public builds and no support from KStorm, Firestorm or AyaneStorm.

## Lineage and credit

- KStorm is built on **[AyaneStorm](doc/ayanestorm-README.md)** by Chanayane, a Firestorm-based viewer for photographers. Most of KStorm's rendering and photography features come from AyaneStorm; its README is kept in [doc/ayanestorm-README.md](doc/ayanestorm-README.md).
- AyaneStorm is built on **[Firestorm](https://www.firestormviewer.org)** by The Phoenix Firestorm Project, Inc.
- Firestorm is built on the **[Second Life viewer](https://github.com/secondlife/viewer)** source code released by Linden Lab.

KStorm is distributed under the GNU LGPL 2.1, like its upstream projects. All upstream copyright notices are retained.

## What KStorm adds

See [doc/kstorm-changes-vs-ayanestorm.md](doc/kstorm-changes-vs-ayanestorm.md) for every KStorm feature and change compared with AyaneStorm.

## Build (Windows)

Prerequisites: see [doc/building_windows.md](doc/building_windows.md). KStorm builds the open (no FMOD, no Havok) configuration with OpenSim support off:

```
autobuild configure -A 64 -c ReleaseFS_open -- --chan Research --no-opensim -DLL_TESTS:BOOL=FALSE
autobuild build -A 64 -c ReleaseFS_open --no-configure
```

The channel is `KStorm-Research`.

## Community respect

KStorm follows the [Third Party Viewer Policy](https://secondlife.com/corporate/third-party-viewers) and the [Second Life Code of Conduct](https://github.com/secondlife/viewer?tab=coc-ov-file).
