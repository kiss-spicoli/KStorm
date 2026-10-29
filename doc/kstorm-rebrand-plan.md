# KStorm Rebrand Plan

Status: DONE 2026-10-29. Executed on branch `feat-kstorm-rebrand`, built and runtime-tested (bokt), merged into `kstorm`.

Goal: the viewer presents itself as **KStorm** wherever a user, the OS, the installer or the grid sees the application's name. Features Ayane created keep their AyaneStorm names. Credit stays visible: KStorm is built on AyaneStorm (Chanayane), which is built on Firestorm (The Phoenix Firestorm Project), which is built on the Second Life viewer (Linden Lab).

## 1. Scope rule (the one decision everything else follows)

Survey: 877 case-insensitive `AyaneStorm` hits in 253 tracked files (`git grep -i ayanestorm`), plus 182 tracked files whose names contain `ayanestorm` (139 outside `doc/`, mostly icons).

| Tier | What | Action |
|---|---|---|
| A. Product identity | App name, window/menu titles, exe version resource, installer, channel, AppData folder, icons, About, README | Change to KStorm |
| B. User-visible labels | Labels using AyaneStorm as the app name (About credits tab, Search title) change; labels naming Ayane's features (menu `AyaneStorm`, Preferences tab, DoF, stars, etc.) keep the name | Per-item list in §3.4 |
| C. Credit and history | `<AS:Chanayane>` tags, `chanayane@firestorm` author lines, Firestorm/LL credits and copyright, `doc/ayanestorm-*.md` design docs, AYAstorm algorithm name | Keep (this is the credit) |
| D. Internal identifiers | `as*` files/classes, `AS*` settings, `as_*` XUI names, `panel_preferences_ayanestorm.xml`, `aspanelprefsayanestorm.*`, `AYANESTORM_PREFIXES`, `ayanestorm_flavor` login param | Keep (recommended) |

Why keep D: renaming breaks every future merge of AyaneStorm `upstream/master` (`master` mirrors it), and renaming `AS*` settings silently resets the user's saved values. Users never see these names. This follows the existing convention: AyaneStorm-owned code keeps `as*`, KStorm-owned code uses `ks*`.

Decided (2026-10-29): change only what users see. Tier A and the app-name labels in Tier B change to KStorm; Ayane's feature names, Tiers C and D stay. No settings migration needed.

## 2. Edit conventions for the rebrand

- One feature branch off `kstorm`: `feat-kstorm-rebrand`; merge into `kstorm` when built and tested.
- Edits in `ll*`/`fs*`/AyaneStorm-owned files: wrap in `// <KS:KS> rebrand` ... `// </KS:KS>` (XML: `<!-- <KS:KS> -->`), keep the original line commented. Exception: plain string swaps inside XML attributes where a comment cannot sit inside the tag; put the tag comment on the line above.
- Keep each changed brand string on its own line, so upstream merges conflict on that line only.
- LF line endings. No shader version bumps.
- Brand name spelling: `KStorm` (display), `KSTORM` (capitalized), `kstorm` (file names), `KStorm_x64` (data folder).

## 3. Inventory by area

### 3.1 Core identity (Tier A, needs rebuild)

| File | Change |
|---|---|
| `indra/llcommon/indra_constants.h:35` | `APP_NAME = "AyaneStorm"` -> `"KStorm"` (drives `%APPDATA%\<name>_x64`, logs, cache, crash dirs) |
| `indra/llcrashlogger/llcrashlogger.cpp:689-700` | Not changed: used by the Linux crash logger only (Decision 4: no macOS/Linux code) |
| `indra/llfilesystem/lldir_mac.cpp:67` | Not changed: macOS code (Decision 4) |
| `indra/lib/python/indra/util/llmanifest.py:97` | `CHANNEL_VENDOR_BASE = 'AyaneStorm'` -> `'KStorm'` |
| `indra/newview/fs_viewer_manifest.py:60` | `.replace("AyaneStorm", "")` -> `.replace("KStorm", "")` |
| `scripts/configure_firestorm.sh:71,397` | `CHANNEL="AyaneStorm-$CHANNEL"` -> `"KStorm-$CHANNEL"`; help text |
| `indra/newview/res/viewerRes.rc:57-59,162-167` | Icons -> `kstorm_icon.ico`; FileDescription/InternalName/ProductName -> KStorm; OriginalFilename -> `KStorm.exe` (OpenSim off, §3.3.1). Keep CompanyName and LegalCopyright (Phoenix Firestorm Project) as credit |
| `indra/newview/skins/default/xui/en/strings.xml:10-14` | `APP_NAME` -> KStorm, `CAPITALIZED_APP_NAME` -> KSTORM |
| `xui/es,ru,tr,zh/strings.xml` | Same two strings |
| `indra/newview/fspanellogin.cpp:929` | Keep `ayanestorm_flavor` (Tier D; no visible effect) |

Channel: today `VIEWER_CHANNEL` = `AyaneStorm-KStorm` (from `--chan KStorm`). After the change, `--chan KStorm` would give `KStorm-KStorm`. Reconfigure with a new suffix:

`autobuild configure -A 64 -c ReleaseFS_open -- --chan Research --no-opensim -DLL_TESTS:BOOL=FALSE` -> `KStorm-Research`, OpenSim off (§3.3.1).

`ReleaseFS_open` is the autobuild build type (optimized build), not branding; it stays.

Decided (2026-10-29): channel suffix `Research`. KStorm starts as an unsupported experiment. `research` matches none of the `release`/`beta`/`project` tests in `indra/newview/CMakeLists.txt:2177-2183`, so CMake uses the default `private` set; the current build has `OPENSIM:BOOL=ON` (`build-vc170-64/CMakeCache.txt`), which appends `-os` (`icons/private-os`). With OpenSim off (§3.3.1) the folder is `icons/private`. No CMake change needed.

The channel string is sent to the grid at login and shows in About and in the Help > About copy text. Linden Lab's Third Party Viewer policy expects a viewer to identify itself under its own name; the change aligns with that.

### 3.2 Icons and art (Tier A)

Placeholder art for now (decided below); the user supplies real KStorm artwork later.

- 126 icon files in `indra/newview/icons/<channel>[-os]/ayanestorm_{16,32,48,128,256,512}.png`, `_256.bmp`, `_icon.ico`, `_icon.icns` (14 channel folders), plus `installers/windows/ayanestorm_icon*.{ico,BMP}`; `res/`, `res-sdl/`, `skins/default/textures/ayanestorm_icon.png`; `win_crash_logger/ayanestorm_icon.ico`; `ayanestorm_icon.icns`, `ayanestormos_icon.icns`; `doc/ayanestorm_256.png`.
- Minimum for Windows: `icons/private/` (OpenSim off) or `icons/private-os/` (OpenSim on) + `installers/windows/` + `skins/default/textures/` + `win_crash_logger/`.
- Plan: add `kstorm_*` files alongside, repoint Windows references, leave the `ayanestorm_*` files in place (avoids delete/modify conflicts on upstream merges; macOS/Linux still use them). Windows references to repoint:
  - `indra/newview/CMakeLists.txt:2250-2261,2297-2299` (Windows icon copy and resource list)
  - `indra/newview/res/viewerRes.rc:57-59`
  - `.gitignore:67` (generated `res/` copies; add a `kstorm_icon.*` line)
  - `indra/newview/growlnotifierwin.cpp:88`
  - `installers/windows/installer_template.nsi:90-93`
  - Untracked generated copies in `indra/newview/res/` are rebuilt by CMake.
- Not changed (macOS/Linux code, Decision 4): `CMakeLists.txt:2123,2192-2193,3039`, `.gitignore:66`, `llwindowsdl.cpp:729`, `llwindowsdl2.cpp:814`, `desktopnotifierlinux.cpp:216-217`, `viewer_manifest.py:1606,2158,2190`. Those platforms keep the `ayanestorm_*` icon files, which hold the same placeholder art.

Decided (2026-10-29): placeholder art. Copy each referenced `ayanestorm_*` icon to a `kstorm_*` file of identical content and repoint references; the user supplies real KStorm artwork later, which then replaces the `kstorm_*` files in place (no reference changes). Copy only the files the Windows build uses: `icons/private/` and `icons/private-os/` (either OpenSim setting works), `installers/windows/`, `res/ayanestorm_icon.png`, `skins/default/textures/`, `win_crash_logger/`. Other channel folders, `res-sdl/` and the `.icns` files keep only their `ayanestorm_*` files.

Decided (2026-10-29): real art in `icons/private/` (Firebird, head C; source `kstorm_icon.svg`, generator `kstorm_make_icons.py`, both kept outside the repository). PNG/ICO/ICNS are transparent; the 24-bit BMPs are on black. Files with names the build still references (`ayanestorm_256.{bmp,png}`, `ayanestorm_icon.icns`, `firestorm_icon.{ico,icns}`) hold the same art under their old names. The unreferenced `ayanestorm_*`/`firestorm_*` files in `icons/private/` were deleted (accepts modify/delete conflicts if upstream changes them). Other placeholder copies listed above still hold the old art.

### 3.3 Installer, packaging, platforms (Tier A)

- `installers/windows/installer_template.nsi:1018-1039`: uninstall cleanup paths `AppData\Roaming\AyaneStorm*` / `Local\AyaneStorm*` -> `KStorm*`. Installer title/shortcut names come from `APP_NAME`/channel via the manifest; verify after the change.
Decided (2026-10-29): macOS and Linux get branding text only; no code, build-script or file-name changes for those platforms in this rebrand.

- macOS, change (display text): `English.lproj/InfoPlist.strings` `CFBundleName` / version strings -> KStorm (keep the Phoenix Firestorm copyright); `Firestorm.xib` menu titles (`AyaneStorm`, About/Hide/Quit AyaneStorm) and window title -> KStorm.
- macOS, not changed (code/build): `CMakeLists.txt:3036-3042` bundle name/executable, `viewer_manifest.py:1471,1483,1757` (`AyaneStorm.app` path; must match the CMake bundle name), `lldir_mac.cpp:67`, `.icns` icon references.
- Linux, change (display text): `installers/linux/firestorm.desktop` `Name=` and `refresh_desktop_app_entry.sh` `Name=` line -> KStorm. Keep both `Icon=` lines pointing at `ayanestorm_icon` (the file the Linux manifest installs).
- Linux, not changed (code/build): `llcrashlogger.cpp`, `llwindowsdl*.cpp`, `desktopnotifierlinux.cpp`, Linux sections of `viewer_manifest.py` and `CMakeLists.txt`.
- Shared code changed in §3.1 (`APP_NAME`, `CHANNEL_VENDOR_BASE`, `strings.xml`) also brands macOS/Linux builds; that is intended.
- `fsutils/download_list.py:331` (`AyaneStormOS-` package prefix): release tooling only; not changed.

These macOS/Linux text edits cannot be build-tested here.

### 3.3.1 OpenSim support (evaluation)

The current build is configured with OpenSim support on: `autobuild.xml` passes `--opensim` for `ReleaseFS_open`, and `build-vc170-64/CMakeCache.txt` has `OPENSIM:BOOL=ON`. The user does not plan to use OpenSim grids.

What `OPENSIM` switches (271 `#if`/`#ifdef` blocks in 97 files, plus CMake and manifest):

| Area | OpenSim on (today) | OpenSim off |
|---|---|---|
| Second Life behaviour | Same: almost every OpenSim block also checks `isInOpenSim()` / `isInSecondLife()` at runtime | Same |
| Login screen | Grid selector, grid-builder link; login waits for the grid list (`llstartup.cpp:1043-1073`) | Second Life grids only, no grid-list wait |
| Startup network | Downloads grid list from `phoenixviewer.com/app/fsdata/grids.xml`; OSSL/AA script libraries (`fsdata.cpp:332`) | Not fetched |
| Preferences | Extra "OpenSim" tab | Tab removed (`llfloaterpreference.cpp:783`) |
| Voice | WebRTC (Second Life) plus Vivox; `SLVoice.exe` and Vivox libraries shipped (`Copy3rdPartyLibs.cmake`, `viewer_manifest.py:717-758`) | WebRTC only (what Second Life uses); Vivox not built or shipped |
| Cache folder | `%LOCALAPPDATA%\<APP_NAME>OS_x64` (`lldir.cpp:427`); today `AyaneStormOS_x64`, 31 GB | `%LOCALAPPDATA%\<APP_NAME>_x64` |
| Settings folder | `%APPDATA%\<APP_NAME>_x64` | Same (unaffected) |
| Icons | `icons/<channel>-os`, installer `_os` icon | `icons/<channel>`, plain icon |
| Package/installer name | `OS` added after the vendor name (`viewer_manifest.py:327-362`) | No `OS` |
| Cross-grid SLURLs | Allowed (Hypergrid); `hop://` protocol registered | Blocked with "can't teleport to grid" (`llurldispatcher.cpp:309`) |
| Octree | OpenSim "4096 bug" workaround (`lloctree.h:427`) | Linden Lab's original re-insert path |
| Havok | Off (open build) | Still off: Havok needs a Linden Lab licence; `--no-opensim` does not enable it |

Assessment for a Second-Life-only viewer:
- Nothing used in Second Life is lost. Gains: simpler login, no OpenSim tab, no grid-list or OSSL downloads from `phoenixviewer.com` at startup, no Vivox/`SLVoice.exe` in the build, cache folder named `KStorm_x64` instead of `KStormOS_x64`.
- Cost: none extra if done in the same reconfigure as the channel change (both force a full rebuild).
- Risk: AyaneStorm publishes OpenSim builds (`AyaneStormOS-` packages), so the OpenSim-off path gets less upstream compile testing. No `as*` or `ks*` file references `OPENSIM`, so the risk of compile errors is low; any errors appear at build time.
- The cache moves to a new, empty folder whichever setting is chosen, because `APP_NAME` changes. The viewer rebuilds it on first runs. The old 31 GB `%LOCALAPPDATA%\AyaneStormOS_x64` can be deleted by hand once KStorm runs well.

Configure line with OpenSim off (`--no-opensim` overrides the `--opensim` in `autobuild.xml`; `scripts/configure_firestorm.sh:81,124`):

`autobuild configure -A 64 -c ReleaseFS_open -- --chan Research --no-opensim -DLL_TESTS:BOOL=FALSE`

Decided (2026-10-29): OpenSim support off; `--no-opensim` added to the rebrand reconfigure.

### 3.4 User-visible labels (Tier B, XUI only, no rebuild)

Decided (2026-10-29): features created by Ayane and named AyaneStorm keep the name (credit). Only labels that use AyaneStorm as the name of the application change. KStorm's own features already sit under the separate `KStorm` menu (`menu_viewer.xml`, `<KS:KS> KStorm Menu`).

Change:

| File | Current | Proposed |
|---|---|---|
| `xui/en/floater_about.xml:96` | Tab `AyaneStorm Credits` (holds Firestorm credits) | `Firestorm Credits`; add a new `KStorm Credits` tab (§4) |
| `xui/en/floater_about.xml:36-37` | Firestorm info line | KStorm lineage line (§4); keep Firestorm URL as credit |
| `xui/en/floater_fs_search.xml:15` | `AyaneStorm Search` (Firestorm search, titled with the app name) | `[APP_NAME] Search` (shows `KStorm Search`) |
| Translations (es, ru, tr, zh) | `APP_NAME` / `CAPITALIZED_APP_NAME` (§3.1) | These, plus support-URL sentences; see Translations below |

Keep (Ayane's features, named AyaneStorm):

| File | Label |
|---|---|
| `xui/en/menu_viewer.xml:6523-6528` | Top menu `AyaneStorm` (Poser, My Lights, Advanced Shape Deformer, Animation, Favorite Streams, effects floaters) |
| `xui/en/floater_preferences.xml:210`, `panel_preferences_ayanestorm.xml:10,15` | Preferences tab `AyaneStorm`, `AyaneStorm Prefs` (AyaneStorm's preferences) |
| `xui/en/notifications.xml:7092` | `Preferences > AyaneStorm` (matches the kept tab) |
| `xui/en/floater_as_depth_of_field.xml:4,31,384` | `AyaneStorm Depth of Field`, `Live / AyaneStorm (new)` |
| `floater_phototools.xml:2047,2050`, `panel_preferences_graphics1.xml:1776,1779` | `Advanced / AyaneStorm DoF`, `Live / AyaneStorm ...` (also copied at runtime into KStorm Lighting's `ks_dof_mode`) |
| `panel_as_stars_settings.xml:6` | `Enable AyaneStorm stars` |
| `floater_as_bone_deformer.xml:8` | `Show AyaneStorm-only scales` |
| `xui/en/notifications.xml:10086` | "AyaneStorm extension ... compatible AyaneStorm viewers" (mesh format) |
| `floater_ks_lighting_preset_edit.xml:21`, `kslightingpresets.cpp:135` | Preset group `AyaneStorm floaters` (KStorm feature naming Ayane's floaters) |
| `app_settings/settings.xml` (~22 Comment strings) | "AyaneStorm Live DoF ..." (Debug Settings) |

Panel/tab `name=` attributes, control names and help topics stay (Tier D).

#### Translations (i18n)

Mechanism: `xui/en` is the complete base. Each of the 12 other language folders (az da de es fr it ja pl pt ru tr zh) holds partial overlay files; a localized file overrides attributes by matching `name`, and anything missing falls back to English (`LLUICtrlFactory::getLayeredXMLNode`, `llui/lluictrlfactory.cpp:176`; `strings.xml` and `notifications.xml` layer the same way). No written i18n policy in `AGENTS.md` or `/doc`; the upstream practice (Firestorm, AyaneStorm) is English only, translators add other languages later. AyaneStorm's 27 `*_as_*` XUI files have no translations in any language. KStorm has none either and stays English only for now (decided 2026-10-29); hard-coded English in `ks*.cpp` stays.

Rebrand impact per language:
- `APP_NAME` / `CAPITALIZED_APP_NAME`: defined only in es, ru, tr, zh (`AyaneStorm`); change those four. The other eight languages inherit the English value.
- About credits tab: every translation already says Firestorm (e.g. de `Firestorm`, es `Créditos de Firestorm`, fr `Contributeurs Firestorm`); only en (`AyaneStorm Credits`) changes. The new `KStorm Credits` tab is English in all languages.
- Search title: translations already say Firestorm (de `Firestorm-Suche`, ...). Change en only, to `[APP_NAME] Search` (floater titles substitute `[APP_NAME]`, as `floater_about.xml` does); leave the translated titles.
- "Unsupported research viewer" wording: the Firestorm support URL appears in en (8 times in `strings.xml`, once in `floater_about.xml`) and in de, it, ja, pl, pt, ru, zh `strings.xml`. Change en; for the translated files, replace only the URL sentence with the English wording or remove it (no new translations written). Decide at execution by checking each string.

### 3.5 Docs and repo text

- `README.md`: rewrite as the KStorm README: name, one-paragraph lineage with credit, link to `doc/kstorm-changes-vs-ayanestorm.md`, build pointer. Move the AyaneStorm feature list into a "Inherited from AyaneStorm" section, or keep the original text verbatim as `doc/ayanestorm-README.md`. Remove AyaneStorm download badge and `git clone` URLs (they point at AyaneStorm's GitHub). Logo -> `doc/kstorm_256.png`.
- `AGENTS.md` / `CLAUDE.md`: title "KStorm Agent Instructions"; author `Kissmy Spicoli`; tag `<KS:KS>`; note that `<AS:Chanayane>` tags in existing code are kept as credit. (Today the memory notes override these lines; the rebrand makes the file match.)
- `doc/ayanestorm-*.md` (41 files) and comments in `as*` code: keep (Tier C). `doc/kstorm-changes-vs-ayanestorm.md`: keep its name; add a "Rebrand" section when merged.
- Code comments in `ks*` files that say "AyaneStorm floater" (ksfloaterlighting*, ksmotionmode, kslightingpresets): keep; they describe origin accurately.
- SVG/XUI header comments `AyaneStorm Viewer; author: chanayane@firestorm`: keep (credit).

### 3.6 Local tooling outside the repo

- `run-kstorm.cmd`: `%APPDATA%\AyaneStorm_x64\...` -> `%APPDATA%\KStorm_x64\...`.
- `settings-history.py`: check for a hard-coded data path; old `settings-history\*.xml` copies stay valid.
- `kstorm-env.cmd`, `KStorm-install-steps.md`: update the configure line to `--chan Research --no-opensim`.
- Claude memory notes (`kstorm-settings-history`, `kstorm-build-commands`, `kstorm-agent-builds`): update paths and configure command.

## 4. Credits text (draft)

New About tab `KStorm Credits` (first credits tab), above the existing Firestorm and Linden Lab tabs:

> KStorm is a personal research viewer by Kissmy Spicoli: an unsupported experiment.
>
> KStorm is built on **AyaneStorm** by Chanayane, a Firestorm-based viewer for photographers; most of KStorm's rendering and photography features come from AyaneStorm.
>
> AyaneStorm is built on **Firestorm** by The Phoenix Firestorm Project, Inc. (https://www.firestormviewer.org).
>
> Firestorm is built on the **Second Life viewer** source code released by Linden Lab.
>
> KStorm is distributed under the GNU LGPL 2.1, like its upstream projects. All upstream copyright notices are retained.

Keep unchanged: the `Linden Lab Credits` tab, the Firestorm team/contributor lists, every license header, `LegalCopyright`/`CompanyName` in `viewerRes.rc`, the copyright in `InfoPlist.strings`.

Support wording: Firestorm does not support forks. Strings that send users to Firestorm support (`strings.xml:1538` frozen-viewer message, About info line) should say KStorm is an unsupported research viewer: no support from KStorm, Firestorm or AyaneStorm.

Decided (2026-10-29): credits text and "unsupported research viewer" wording approved as drafted.

## 5. User data migration

`APP_NAME` change moves user data from `%APPDATA%\AyaneStorm_x64` and `%LOCALAPPDATA%\AyaneStorm_x64` to `...\KStorm_x64`. On first KStorm launch the viewer would start with default settings, no saved logins, empty cache.

Plan (manual, one time, viewer closed): copy `%APPDATA%\AyaneStorm_x64` -> `%APPDATA%\KStorm_x64` (settings, per-account folders, chat logs, presets, KStorm lighting presets). Do not copy the cache `%LOCALAPPDATA%\AyaneStormOS_x64` (31 GB; rebuilds; new folder per §3.3.1). Keep the AyaneStorm folders as backup; delete later by hand.

Side effect: an installed AyaneStorm and KStorm then keep separate settings and can run side by side.

Optional: a small code migration on startup (copy if `KStorm_x64` is missing). Not recommended; one manual copy is enough for a single user.

## 6. Out of scope (noted, not changed)

- Executable name `firestorm-bin.exe` (Firestorm build-wide; renaming touches CMake, manifest, installer, run script). Optional later.
- Firestorm service URLs (`phoenixviewer.com` fsdata, grids, client tags, wiki help, JIRA): functional; leave. The help/JIRA links point to Firestorm, which does not support KStorm; consider removing the "file a bug" link.
- Login/splash page: corrected after execution. OpenSim builds loaded AyaneStorm's page (`storm.ayane.li`, `fsgridhandler.cpp`); OpenSim off switched it to Firestorm's (`phoenixviewer.com/app/loginV3`, `llviewernetwork.cpp`). Replaced by a local KStorm page on `feat-ks-login-page` (`85d2fbcfbae`).
- Viewer version numbers: keep upstream numbering.
- `AYAstorm` transparency mode name: an algorithm name, keep.

## 7. Execution order (when approved)

1. All decisions made (2026-10-29); user says go.
2. Branch `feat-kstorm-rebrand` off `kstorm`.
3. Tier A code/build files (§3.1, §3.3) and icons (§3.2).
4. Tier B XUI labels (§3.4), translations, About credits (§4).
5. README, AGENTS.md (§3.5).
6. Grep check: `git grep -n -i ayanestorm -- ':!doc'` and review every remaining hit is Tier C/D.
7. User copies AppData folder (§5); update `run-kstorm.cmd` and tooling (§3.6).
8. Reconfigure with `--chan Research --no-opensim` (full rebuild), then build.
9. Runtime checks: window title, taskbar/exe icon, exe Properties > Details, About (all three credit tabs, channel line reads `KStorm-Research`), Search floater title, Ayane's labels unchanged (AyaneStorm menu, Preferences tab, DoF combos incl. KStorm Lighting DoF pane, stars checkbox), settings and logins carried over, settings-history copy runs from the new path.
10. Update `doc/kstorm-changes-vs-ayanestorm.md`, merge into `kstorm`, update memory notes.

## 8. Risks

- Upstream AyaneStorm merges: each changed line in shared files can conflict. Mitigation: Tier D untouched, one brand string per line, `<KS:KS>` tags. Expect conflicts mainly in `strings.xml`, `menu_viewer.xml`, `floater_preferences.xml`, `viewerRes.rc`, `CMakeLists.txt`, README.
- Reconfigure + full rebuild required (channel and `APP_NAME` are compile-time).
- Forgetting the AppData copy looks like lost settings; copy before first launch.
- macOS/Linux edits cannot be build-tested here.
