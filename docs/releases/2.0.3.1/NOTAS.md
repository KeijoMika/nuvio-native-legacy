# Nuvio Legacy 2.0.3.1

## Fixed

- **TV guide: search no longer crashes** when you press "done" on the keyboard and a channel has more than 24 programmes in the next 6 hours (#344). Searching now also looks past the first 24 programmes of such a channel.
- **Season tabs match the episode list** (#372, #328). When a metadata add-on supplied a longer episode list than the Nuvio catalog (anime such as The Apothecary Diaries), its episodes were loaded but the page kept one season tab, so only season 1 was visible. The tabs now come from the list that is actually shown, and reopening the title while its cast and artwork are still loading no longer brings the old tabs back.
- 2017 LG TVs (webOS 3): the app no longer closes when opening a video for the second time in a session (the DTS fallback check loaded and unloaded LG's media player library on every video; it now loads once and stays).
