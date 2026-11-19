# Video notes, part 4 (1:48:22 to 2:22:40)

Source: 1993 video tutorial of the original, version 3.16 transcript lines 244-316, frames f0508-f0702. Everything here is [obs from video], not checked against the manual unless noted. Key letters are as spoken in the video.

Topics: arrange entry transpose and Disable Arrange Transpose; Graphic Arrange (GAM) and the four chains ("lanes"); RMG (Realtime MIDI Generator) faders, snapshots, drop record; Hyper Edit for drum/timpani rolls; event list reading; fonts (brief); the publisher's sync box SMPTE sync (brief).

## 1. Arrange entry transpose [1:48:22-1:51:09]

- Deleting an entry: click it in the arrange list, press Delete. [1:48:22]
- Adding an entry: "make another command" at bar 14 that plays the Verse pattern again. The arrange list then reads `1 Intro 2`, `5 Verse 1`, `14 Verse 1` (start bar, pattern name, pattern number). [1:48:22]
- The selected entry's parameters are edited in the box below the list: `UP BEAT-` / `CUT+` (shown as `1 1 1 1`), `TRANSPOSE`, `Pattern LENGTH` (`no limit`), `Pattern POSITION` (`14 1 1 1`). Above those are buttons `CATCH`, `#1 up`, `#2 down` and a highlighted third button. The arrange header reads `ARRANGE a b c d MUTE`. [1:48:52]
  - ![arrange transpose box](../video-frames/1-48-52-arrange-transpose-box.png)
  - ![arrange list with transpose 1](../video-frames/1-49-00-arrange-list-entry-transpose.png)
- He sets transpose to +1 on the bar 14 entry, plays from the second verse (clicks that entry, then Play) and fast-forwards with the `[` `]` keys. From bar 14 every part sounds a semitone up. [1:48:53]
- Problem: drum tracks played on keyboards/drum machines get transposed onto different drum notes. Soloing the KickSnre track in arrange mode shows the wrong drums; with arrange off it plays correctly. [1:49:20]
- Fix: **Options > Disable Arrange Transpose**. This is a grid of 6 rows (Port A-F) by 16 MIDI channels. Clicking a cell toggles it (black = disabled). He clicks A16 (the SR-16 drum machine), then OK. The drums now play correctly with the arrange transpose active. The setting is per output port and channel, not per track. [1:50:02-1:51:09]
  - ![disable arrange transpose grid](../video-frames/1-50-05-disable-arrange-transpose.png)

## 2. Graphic Arrange (GAM) and the four chains [1:51:09-1:55:14]

- **K** toggles the graphic arrange ("GAM"). It shows the same arrange in a different view. [1:51:09]
- Layout: continuous bar numbers down the left, then one column per chain (a, b, c, d). Each entry is a block labelled with its name and pattern number, e.g. a 4-bar block for Intro (pattern 2), then Verse (1), then Verse (1) again. During playback a horizontal song-position line moves down and jumps into the next block at each changeover. [1:51:09-1:51:39]
- He calls the chains "lanes" A-D and compares them to motorway lanes. Everything so far was in lane A. [1:51:59]
- **Right-click** in an empty lane next to the song start creates a new entry and asks for a pattern number. Pattern 4 in lane B now plays alongside patterns 2, 1, 1 in lane A. [1:52:24]
  - Use: an improvised "busk" part that runs through the whole song without being split at section changes. He picks a track in pattern 4 with arrange on, chooses a piano sound and records. There is a 4-beat count-in, and the status line shows `RECORD: deletes whole old track` (record-delete mode). The result is one continuous recording, not split into chunks. [1:52:53]
  - Soloing that track (click track 4, Solo, Play) plays it alone. [1:53:46]
- A third and fourth lane are added the same way (pattern 5 in lane C, another pattern in lane D). [1:53:46-1:54:12]
- Typical uses for the extra lanes: volume-change data from the RMG page, tempo changes and other "separate" information. [1:54:12]
- **64 tracks:** 4 lanes × 16 tracks. On screen, the track number column next to the track list shows **1-16 for lane a, 17-32 for lane b, 33-48 for lane c, 49-64 for lane d** when an entry in that lane is selected. Each pattern's own track list still shows 1-16. [1:54:12-1:54:38]
  - ![lane b selected, tracks 17-32, record-delete message](../video-frames/1-53-40-gam-lane-b-tracks-17-32.png)
  - ![lane c, tracks 33-48](../video-frames/1-54-27-gam-lane-c-tracks-33-48.png)
  - ![lane d, tracks 49-64](../video-frames/1-54-31-gam-lane-d-tracks-49-64.png)
- Tip: with several lanes the text arrange list is confusing. Always use the GAM view for multi-lane arrangements. [1:54:38]

## 3. Main page layout (seen throughout this section)

Read from frames f0509, f0517, f0608:
- Top bar: free memory, `ARRANGE` (ON/off), `SyncDEL`, `SYNC` (`intern` / `SMPTE`), `TEMPO` (`110.0000`), `FORMAT` (`1/16`), `SIGNATURE` (`4/ 4`), h:min:sec:frames clock, bar position.
- Track list: number, name, instrument name (e.g. `SR16`, `Korg M1`, `Wave SR2`), port letter, channel. A track can have channel `original`. His "16 Vol" track, holding RMG data for several channels, shows `original`, so recorded channels are kept. [2:05:35]
  - ![track list with Vol track on original](../video-frames/2-06-48-track-list-vol-track-original.png)
- Track parameter column on the right: `CHANNEL, QUANTIZE, GROOVE, TRANSPOS, VELOCITY, COMPRESS, LOOP, DELAY, LOWEST, HIGHEST, GHOST OF`. Buttons: `RMG`, `EDIT`, `DO QUANTIZE`, `SOLO`, `HIDE`, `DEFAULT`. Below the track list: `MIDI THRU`, `OVERFLOW`, `SMPTE`.
- Transport panel (far right): `LEFT LOCATOR`, `RIGHT LOCATOR`, `AUTODROP`, `CYCLE`, `DROP`, `UNDO`, `PUNCH`, `RECORD`, `<<` `>>` `<<<` `>>>`, `STOP`, `START`, `CONT`.
  - ![locators, autodrop, cycle](../video-frames/2-07-38-locators-autodrop-cycle.png)
- Keys used in this part: `[` `]` fast rewind/forward; **E** score/edit page; **K** GAM; **M** RMG; **J** Hyper Edit (inside the edit page); **S** SMPTE sync reference; **Y** toggles sync between SMPTE and internal; **Space** drop record (paused record); **Enter**/Return stop or close a dialog; `(` `)` step bars in the edit page.

## 4. RMG: Realtime MIDI Generator [2:01:37-2:08:56]

- Purpose: mainly recording volume changes as controller events. RMG can do much more. [2:01:37]
- Workflow: first select an empty track "out of the way" (e.g. 16). The RMG output is recorded as ordinary events on the selected track, and he keeps it separate from note tracks. Open with the `RMG` button or **M**. [2:02:06]
- Layout: 16 vertical faders, **one per MIDI channel of the selected port** (Port A shown), not per track. Each fader shows its value (127) and its channel number 1-16. Above the faders are instrument/channel names (Korg over ch 1, Piano over 9, SR16 over 16). Header fields: `STATUS: Control`, `NUMBER: 7 Volume`, `LISTEN TO MIDI`, `PORT A`, `CHANNELNAME Piano`, `DELETE MOVEMENTS`, `COPY TO ALL`, `EXIT`. A snapshot row runs along the bottom, labelled `SNAPSHOTS CLICK LEFT:RECALL RIGHT:SAVE`. [2:02:35-2:03:00]
  - ![RMG faders](../video-frames/2-03-45-rmg-faders.png)
- Moving a fader with the mouse during playback sends the controller live (fading the piano on A9). [2:03:00]
- **Recording:** right-click `RECORD` in the transport panel on the right of the RMG page. After the count-in, fader moves are recorded. [2:03:36]
  - ![RMG with transport panel](../video-frames/2-03-58-rmg-transport-panel.png)
- **Linking faders:** click the number under a fader to join it to others. Linked faders move together (he links 10 and 1). With all faders linked, you can record a whole-song fade-out. Click again to unlink. [2:04:05-2:05:04]
- **Copy to all:** sets every fader to the selected fader's value (used to reset all faders to the top). [2:04:28]
- Playback moves the faders on screen to follow the recorded data. [2:04:28]
- **Delete movements:** select the bad fader (it flashes), click `DELETE MOVEMENTS` and confirm. This deletes all of that channel's controller events from the track; other channels are untouched. [2:05:35-2:06:05]
  - ![delete movements confirm](../video-frames/2-06-04-rmg-delete-movements.png)
- **Snapshots** (fader-setting memories): right-click a snapshot box to save, left-click to recall instantly. A box shows a tiny graphic of the fader positions (solid black = all at max). [2:06:34-2:07:22]
  - ![snapshots](../video-frames/2-07-41-rmg-snapshots.png)
- **Drop record an initial-volume template:** use a separate track (15). Set the bar counter to 1, press **Space** to enter drop record ("paused record"), click the all-faders-up snapshot, then stop with Enter. The result is one CC7 = 127 event per channel at 1 1 1 1, so playback always starts at a known level. [2:06:34-2:08:56]
  - ![event list of the snapshot](../video-frames/2-08-12-event-list-volume-snapshot.png)

## 5. Event list (as seen in the edit page) [2:07:52, 2:09:50]

- He drags the dividing line down to make the event box larger above the score. [2:07:52]
- Column header: `BAR / 4 16 768  STATUS  CHANNEL  -1-  -2-  Length/Info`. Position is bar, beat, sixteenth, tick. The timpani roll entries `4 1 1 1`, `4 1 1 13`, `4 1 1 25`, `4 1 1 37`, `4 1 2 1` … show 64th notes as 12-tick steps, so a sixteenth is 48 ticks (768 per 4/4 bar, i.e. 192 per quarter). [2:09:50]
  - ![timpani roll in event list](../video-frames/2-09-50-event-list-timpani-roll.png)
- Controller rows read e.g. `1 1 1 1 Control 9  7 127 Volume`: channel in the CHANNEL column, controller number in -1-, value in -2-, controller name as info. [2:08:12]
- Filter/function buttons on the left: `ON/OFF` (notes), `PROGRAM`, `CONTROL`, `PITCH W`, `C PRES`/`P PRES`, `SysEx`, `PSEUDO`, `COMMENT`, then `QUANTIZE`, `DEQUANT`, `DELETE`, then `MATRIX`, `SNAP`, `VELOC`. [2:08:12]

## 6. Hyper Edit: drawing a timpani roll [2:08:56-2:13:20]

- Context: the intro pattern has a timpani roll made with Hyper Edit. He hides the original track and remakes it: empty track 8, channel A6 (timpani), **E** for the edit page (OK to dismiss a message), then **J** to show Hyper Edit. [2:09:26-2:09:51]
- Layout: rows ("lines") are instrument lanes, time runs left to right, and the ruler shows bars split into four beats. `(` `)` step through bars. Set row above: `SET 1`, zoom+/zoom-, `PEN`, `COLOR`, `AutoDel`(?), `Flags`, `LockScore`. Lane columns: `NAME`, `QUA`, `DLY`, a fourth column, `STATUS`. The loaded set shows `BD 64`, `RIMSHOT 16`, `SNARE 16`, `CLAP 16`, `CL.HIHAT 16`, all `NOTE`. [2:10:10]
  - ![Hyper Edit drum set](../video-frames/2-09-59-hyper-edit-drum-set.png)
- Ready-made sets come from the autoload song. If you replaced the autoload song, reload "system hyper edit sets" from the backup disk. [2:10:35]
- **Define a lane:** right-click the white name box at the start of the lane to open its definition. Change the note name (to C2), then click **Convert events**. As he puts it, everything in this track now becomes C. (The manual describes Convert as acting per lane; see spec-hyper-edit.md §7.) [2:10:35-2:11:36]
- **Draw:** hold the left button and drag across. Notes are entered at the lane's quantize step (QUA). The height of each beam is velocity, set by the mouse's vertical position: low is quiet, high is loud. Dragging diagonally upward draws a crescendo. [2:11:03, 2:11:50]
- **Erase:** hold the right button and drag. [2:11:03]
- Per-lane quantize: 16 means sixteenth steps. For rolls he sets 64. Each beam is one note. [2:11:50-2:12:16]
- You can "drop in the middle and chop out little chunks" to fix parts of a roll. The same method works for any drum sound: set the note, name the lane, click in the beats. [2:12:53-2:13:20]
- Later he also uses Hyper Edit to enter tempo pseudo events into a dedicated tempo track. [2:18:43]

## 7. Fonts (score printing, brief) [1:55:14-2:01:37]

- Menu **Edit > Fonts**. **E** goes to the score page. You need about 2 MB of RAM for serious font use. [1:55:31-1:55:58]
- Left side: font slots (A-C are system fonts; you load into D onwards) via a file selector. File names: 4 letters for family/style (`ATSS` Swiss, `ATTR` Dutch), then point size (10, 12, 18, 24), then a suffix: none = screen, `LS` = laser, `SP` = 24/9-pin. Load both a screen and a printer font. [1:56:28-1:57:45]
- Middle grid: 16 styles (the 1-16 style numbers used on the score page). Each style has a screen-font slot, a 24-pin slot, a laser slot and a size ratio. Right side: which style is used for track names, bar numbers, lyrics, chord symbols, page numbers and so on. Fonts load when you click OK. [1:58:11-2:01:10]
  - ![font configuration](../video-frames/1-57-55-font-configuration.png)

## 8. the publisher's sync box and SMPTE sync (brief) [2:13:20-2:22:17]

- The the publisher's sync box adds MIDI In 2/3, Outs E and F (E1-16, F1-16), SMPTE in/out and the dongle port. [2:13:20-2:14:09]
- **Sync reference page (S):** `SMPTE OFFSET`, `END`, `TEMPO`, `SONG START`, `SONG END`, `FRAMES/sec 25 (EBU)`, `DISPLAY OFFSET`, `NAME .REF` + `LOAD`, `CREATE (fixed tempo)`, `CREATE TRACK n USING TEMPO OF PATTERN n`, a generator section (`START TIME`, `START`, `Quartz clocked`), `VITC LINE`, `TIMEOUT`, `EXIT`. [2:14:46]
  - ![sync reference](../video-frames/2-14-46-smpte-sync-reference.png)
- Striping: set the start time (he uses 00:59:55:00), click START, and a dialog says "Start Record on tape / Press any key to stop". Stripe the whole tape on track 8. [2:14:39-2:16:01]
  - ![generate SMPTE dialog](../video-frames/2-15-53-generate-smpte-dialog.png)
- Sync reference: set the offset to 01:00:10 (song start on tape) and the tempo, then Create (fixed tempo). Set main-page SYNC to SMPTE; **Y** toggles SMPTE/intern. The tape is master, and starting mid-song chases the position. [2:16:01-2:18:11]
- Variable tempo: put the tempo changes in a track of a pattern that runs through the whole song (pattern 7, track 10, in its own lane), then "Create using tempo of pattern 7 track 10". `DELETE` removes a reference, and references can be saved as files. [2:18:43-2:19:48]
  - ![create from tempo track](../video-frames/2-20-58-smpte-tempo-from-pattern-track.png)
- The same procedure works with timecode-striped video (offset 10:00:00:00). [2:20:15-2:21:46]

## Relevant to our build

1. **Disable arrange transpose is keyed by port+channel**, a 6×16 grid under Options. SPEC already says "per output channel" [doc 20-24]. In our model the destination is a Reaper track plus optional channel, so we need an equivalent: a per-destination (track+channel) "ignore arrange transpose" flag. Also consider a per-pattern-track flag. [ours]
2. **Track numbering 1-64 across chains** (a = 1-16, b = 17-32, c = 33-48, d = 49-64) is shown in the UI. This confirms SPEC's "up to 64 simultaneous tracks". If we show global track numbers, use this scheme.
3. **GAM entry creation by right-click** in an empty lane, followed by a pattern-number prompt. SPEC §5 GAM says "create by clicking at a bar" without naming the button; tag right-click as [obs-video].
4. **Long through-composed parts in a parallel chain** (busk tracks, tempo track, volume automation) is the intended use of chains b-d. Our renderer must handle a single entry spanning many entries of chain a, with no splitting.
5. **Record mode message `RECORD: deletes whole old track`** confirms record-delete as the default record mode (SPEC §8).
6. **Event position format** bar / beat / 16th / tick with 48 ticks per sixteenth (192 ppq). Useful for our Event Editor display; spec-event-editor.md should agree.
7. **Track channel `original`** keeps per-event channels on a multi-channel track. Check that our track Channel parameter has an "original/none" option. The CC data tracks here depend on it.
8. **Hyper Edit:** right-click the lane name to edit the definition; left-drag draws at QUA steps with beam height = velocity (pointer Y); right-drag erases; per-lane QUA up to 64 for rolls. This matches spec-hyper-edit.md. The video's "Convert events" makes a timpani track one pitch. Worth checking whether it converts only the lane or the whole track (manual says lane).
9. RMG, snapshots, fonts and SMPTE are out of scope per SCOPE.md. The drop-recorded "initial volume" template shows that users expect a known CC state at song start. Reaper's CC chase/reset may cover it; no action needed.
10. Shortcut letters (K = GAM, J = Hyper Edit, E = edit, M = RMG, `[` `]` wind) are candidates for default action bindings in our window. [ours]
