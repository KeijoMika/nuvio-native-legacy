# Nuvio Legacy 2.0.4

## Fixed

- **TV guide: search no longer crashes** when you press "done" on the keyboard and a channel has more than 24 programmes in the next 6 hours (#344). Searching now also looks past the first 24 programmes of such a channel.
- **Season tabs match the episode list** (#372, #328). When a metadata add-on supplied a longer episode list than the Nuvio catalog (anime such as The Apothecary Diaries), its episodes were loaded but the page kept one season tab, so only season 1 was visible. The tabs now come from the list that is actually shown, and reopening the title while its cast and artwork are still loading no longer brings the old tabs back.
- 2017 LG TVs (webOS 3): the app no longer closes when opening a video for the second time in a session (the DTS fallback check loaded and unloaded LG's media player library on every video; it now loads once and stays).
- **Series no longer show the same episode on every card.** When a stream add-on answered the episode request with its list of torrent files (thousands of entries repeating the same few episodes), that list replaced the real one: every card read "Episode 1" with "1200 of 1200 watched" (2.0.2), or the series was left with only a handful of episodes (2.0.3). Add-on lists are now compared by distinct episodes, and a file list is only used when it really knows more episodes than the catalog (the catalog's episode names are kept where both have them). This also covers series opened from that add-on's own catalog.
- 2017 LG TVs (webOS 3.9) are no longer treated as webOS 4: the version now comes from the TV's own `webos_release` (the same one the log's `[tv]` line shows), so Dolby Vision in MKV, which needs webOS 4, stays off on them.
- Switching profile and coming back no longer rearranges the Home rows (collections, catalogs, Continue Watching): the other profile's addon catalogs are no longer registered into the profile you return to (#392).
- **Custom poster URL template is saved again** (#390). Long templates (up to 400 characters) and API keys with uppercase letters are kept as typed; before, anything past 299 characters was cut and the key was lowercased, so the template was rejected without saving. A template whose finished address could not fit is refused when you save it, with a message.

## Notes

| Platform | File |
| --- | --- |
| LG webOS 3+ | `space.nuvio.native.legacy_2.0.4_arm.ipk` |
| LG with more RAM | `space.nuvio.native.legacy_2.0.4_arm-highcache.ipk` |
| Samsung Tizen 4 / 5 / 5.5 | `Nuvio-2.0.4-NuvioTpk40.tpk` |
| Samsung Tizen 6 | `Nuvio-2.0.4-NuvioTpk60.tpk` |
| Samsung Tizen 6.5 / 7 | `Nuvio-2.0.4-NuvioTpk65.tpk` |
| Samsung Tizen 8 / 9 | `Nuvio-2.0.4-NuvioTpk.tpk` |
| Samsung web app, Tizen 5.5+ | `NuvioTV-2.0.4-tizen.wgt` |
| Android TV / Google TV, Android 7+ | `Nuvio-2.0.4-android.apk` |

If something breaks, send the log code from Settings › About and help.
