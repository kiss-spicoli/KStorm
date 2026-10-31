# KStorm @VERSION@, pre-release

**KStorm.** This software is not provided or supported by Linden Lab, the makers of Second Life.

KStorm is a research viewer for Second Life, for people who build scenes, looks and photos. It is
built on AyaneStorm and Firestorm, and neither project provides or supports it.

## Install

Download the `Setup.exe` below and run it. To remove KStorm, use Installed apps in Windows Settings.

The installer is unsigned, so Windows SmartScreen will warn you before it runs.

KStorm needs 64-bit Windows, a processor with AVX2, a graphics card with an up-to-date driver, and a
display of at least 1K, with 4K preferred. It won't run on a processor without AVX2.

## Support

**KStorm provides no customer support.** Linden Lab, Firestorm and AyaneStorm don't support it either.
There is a [Troubleshooting section](https://github.com/kiss-spicoli/KStorm#troubleshooting) in the
README. Questions about your Second Life account go to
[Second Life support](https://support.secondlife.com/).

## What may surprise you

Compared with Linden Lab's viewer:

- **No automatic updates.** KStorm doesn't update itself. New versions appear on this Releases page.
- **RLVa is disabled and can't be turned on.** Items that depend on RestrainedLove won't respond.
- **LSL bridge.** A Firestorm feature kept in KStorm: the viewer makes and wears a small scripted
  attachment for you. It is on by default and can be turned off in Preferences.
- **Open-source libraries only.** The build leaves out FMOD, Kakadu and Havok. Sound uses OpenAL, parcel
  music uses VLC, and textures use an open-source decoder. Mesh physics tools that rely on Havok aren't
  available.
- **Its own login screen.** The viewer draws it itself, with no web page.
- **Help goes to KStorm's README**, not to a help site.

## Content

KStorm adds no adult content of its own. As in any viewer, what you see depends on where you go in
Second Life and on your maturity settings.

## Privacy

KStorm collects no data about you. The project runs no servers, and the viewer has no KStorm analytics
and sends no crash reports.

- **Linden Lab.** To use Second Life, the viewer sends your login details and activity to Linden Lab's
  servers, including the viewer statistics Linden Lab requires. Linden Lab's
  [privacy policy](https://lindenlab.com/privacy) covers this.
- **On your computer.** Settings, saved logins and chat logs stay in `%APPDATA%\KStorm_x64`, and the
  cache in `%LOCALAPPDATA%\KStorm_x64`. KStorm doesn't send them anywhere.
- **Other services.** The built-in browser, media and music streams contact the sites they load. Optional
  services contact their own servers only if you connect them. Those sites' own policies apply.
- **This download.** GitHub hosts this page and has its own privacy policy. KStorm has no website of
  its own, no advertising and no tracking cookies. The built-in browser stores cookies from the sites
  it loads on your computer.

Because KStorm holds no personal data about you, it has nothing to show, correct, delete or hand over.
Requests about your Second Life data go to Linden Lab.

Saved logins stay on your computer, so keep it secure. No software can promise perfect security.

KStorm isn't intended for anyone under 18. Installing or removing it doesn't ask for personal
information. Any change to this statement will be published with the release that makes it.

## Source

KStorm is distributed under the GNU LGPL 2.1. The source for this release is at
https://github.com/kiss-spicoli/KStorm/tree/@TAG@.
