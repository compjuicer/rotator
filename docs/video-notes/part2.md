# Video notes, part 2 (0:36:15 to 1:12:13)

Source: 1993 video tutorial of the original, version 3.16. Transcript lines 84-165, frames f0198-f0361.
Everything here is **observed in the video**, not confirmed on our hardware. When it goes into SPEC.md, tag it [verify] (or [doc] once it's matched to the manual).

The first half is mostly score editing and printing (skimmed below). The sequencer material is at 0:57:38 to 1:03:38: recording another track, track loop, solo/hide, copying a track to split a keyboard part with Lowest/Highest, and velocity mixing.

---

## 1. Main page layout (seen repeatedly)

![main page](../video-frames/0-37-09-main-page.png)
![main page, perc track](../video-frames/1-00-19-main-page-perc-track.png)

The screen, top to bottom and left to right:
- Menu bar: Desk, File, Functions, Quantize, MIDI, Flags, Options, Edit, Copy.
- Title line: song name (e.g. `TIM1.SON`, `AUTOLOAD.SON`), "<PROGRAM> 3.16".
- Status row: free memory, `intern` SYNC, TEMPO `118.0000`, FORMAT `1/16`, SIGNATURE `4/ 4`, h:min:sec:ms clock, bar position counter `b b b b`.
- Left: ARRANGE panel with chain tabs `a b c d` and the entry `1 Pattern: 1`. Below it are the transpose field, PATTERN LENGTH (`no limit`) and PATTERN POSITION (`1 1 1 1`).
- Centre: a MUTE column with track numbers 1-16, a STATUS column, then the track list headed `Pattern: 1`. Each row shows the number, the name (large font), the instrument/channel name in small text (Proteus, SR16, Piano, Wave SR2), and a **parameter value column**.
- Right of the list: the TRACK box (`TRACK: n`) with the parameter rows CHANNEL, QUANTIZE, GROOVE, TRANSPOS, VELOCITY, COMPRESS, LOOP, DELAY, LOWEST, HIGHEST, GHOST OF.
- Right column: LEFT/RIGHT LOCATOR, AUTODROP and CYCLE, then DROP, UNDO, PUNCH, RECORD, then transport `<<` `>>` `<<<` `>>>` STOP START CONT.
- Under the track box: EDIT, DO QUANTIZE, SOLO, HIDE, DEFAULT. The track list footer has three small buttons (they look like MIDI THRU / … / … but are illegible).

### The parameter value column follows the selected parameter [0:47:49, 0:58:03, 1:02:07]
The header to the right of `Pattern: 1` names a parameter, and the column shows that parameter's value for **every** track in the pattern at once:
- `QUANTIZE`: recorded tracks show `16` or `8`; empty tracks show `768`, i.e. no quantize at 768 ticks per bar. [0:47:49]
- `CHANNEL`: `A 9`, `B 13`, `A 16`… (port letter + channel). [0:58:03]
- `HIGHEST`: Piano `G8`, Bass `F#2`, and blank where the value is the default. [1:02:07]

In the frames the column header matches the parameter highlighted in the TRACK box (HIGHEST is highlighted in both at 1:02:07), so selecting a parameter in the track box seems to switch the column.

![quantize column](../video-frames/0-47-49-track-list-quantize-column.png)
![channel column](../video-frames/0-58-03-track-list-channel-column.png)
![highest column](../video-frames/1-02-07-highest-column-split.png)

---

## 2. Tracks: naming, recording, reordering, copying

- **Naming:** double-click the track name, type, then Return. [0:58:28, 1:10:32]
- **Selecting:** click a track to select it (shown as an inverted row). Its parameters appear in the TRACK box.
- **Recording onto a fresh track:** select the next empty track, set its channel (e.g. `A16` for the SR16 drum machine), and press RECORD. Recording starts after a one-bar count-in ("count four"). While recording, the title bar reads **`RECORD: deletes whole old track`**, so plain record replaces the track. [0:57:38-0:58:28]
  ![record message](../video-frames/0-58-28-record-deletes-old-track.png)
- **Reordering tracks (exchange):** left-click and hold on a track until a hand cursor appears, drag it onto another track, and let go. A box asks **"Exchange or merge these tracks?"**. Exchange swaps them; merge combines them. The tutorial uses this to put Voice, Piano, Drums in score order. [0:47:22-0:48:19]
  ![after exchange](../video-frames/0-48-05-track-exchange-result.png)
- **Copying a track:** the same click-hold-drag, but dropped on an **empty** slot, copies the track. The result is "two piano parts exactly the same", with its parameters. [1:00:17]

## 3. Track parameters used in this section

![track params zoom](../video-frames/1-01-16-track-params-zoom.png)

- **CHANNEL:** shown as port letter + channel, e.g. `F 2` (Wavestation bass) or `B 13`. Changing it on the copied track sends that copy to another synth. [1:00:43]
- **QUANTIZE:** the perc track is quantised to 16 after recording, simply by setting the parameter, and the tutor listens to check. [0:58:48]
- **LOOP:** value in **beats**. The tutor plays 4 beats of percussion, stops, and sets `LOOP 4` on the track (the box reads "Track 4 loop"). The 4 beats repeat for the whole song, so one played bar becomes a whole percussion track. [0:58:28-0:59:26]
- **SOLO:** select a track and click SOLO; only that track plays. Click again to turn it off. Recommended for finding mistakes in a part. [0:58:48-0:59:26]
- **HIDE:** "the opposite of solo", and the button sits next to SOLO. To hide a track, point at the area **in front of** it (the STATUS column left of the number) and left-click. A `*` appears and the track stops playing. Several tracks can be hidden. Unhide by clicking again. [0:59:52-1:00:17, 1:02:17-1:02:21]
  ![hide stars](../video-frames/1-02-18-hide-stars.png)
  ![hide button lit](../video-frames/1-02-21-hide-button.png)
- **LOWEST / HIGHEST (keyboard split from one recording)** [1:00:55-1:02:21]
  1. The piano track holds right-hand chords and the left-hand bass line.
  2. Copy it to empty track 5, name it "Bass", and set CHANNEL `F 2` (the bass sound).
  3. On Bass, set LOWEST to the bottom of the range (screen shows `C#-2`, likely the default minimum) and HIGHEST to `F#2`.
  4. On Piano, set LOWEST `F#2` and HIGHEST to the top (`G8`).
  5. Result: one performance played by two instruments split at F#2. The tutor speaks of the split as "the breakpoint". Whether a note exactly at F#2 sounds on both tracks isn't shown. The bounds look inclusive, so F#2 would play on both.
  ![bass lowest/highest](../video-frames/1-01-50-bass-lowest-highest.png)
- **VELOCITY:** an offset, set by clicking VELOCITY on the track. The tutor uses `-50` to pull the voice down and says the range goes "all the way up to +127". It works as "instant velocity mixing" but applies to the whole track ("a global mix across the whole track"). Real volume mixing is left for the Realtime MIDI Generator later. [1:02:46-1:03:38]

## 4. Transport and navigation keys

These keys work on the score page, and most probably work everywhere:
- Numpad **0** = start, numpad **Enter** = stop. The score page has no on-screen start/stop buttons, so you have to remember these. [0:36:30]
- **E** toggles between the main page and the edit (score) page for the selected track. [0:37:02, 0:41:24]
- **( )**, the round brackets on the numpad next to record, step the display back or forward **one bar**. [0:44:31]
- **[ ]**, the square brackets next to Return, scroll the **main bar counter**. Hold them down to scroll, then press **L** ("leap") to jump the display to that bar. [0:45:02-0:45:28]
- **Clr Home** = go to bar 1. **Shift+Clr Home** = go to the end of the song. [0:45:53-0:46:19]
- In any dialog, the button with a thick black border is the default and Return activates it. [0:41:00]
- Menus are context-sensitive: on the score page, everything in the File menu is greyed out and you must leave with E to save or load. [0:54:02]

## 5. Event editor strip on the score page [0:41:09, 1:04:06-1:04:20]

![score + event strip](../video-frames/1-04-06-score-event-editor-split.png)
![event editor enlarged](../video-frames/1-04-20-event-editor-enlarged.png)

- The score page has an **event list above the score**, described as "exactly the same information as in the score but in word form". The divider between the list and the score can be dragged up or down to give space to either one. [1:04:18]
- Column header: `1 2 3 4 5 6 BAR / 4 16 768 STATUS CHANNEL -1- -2- Length/Info`. A row reads, e.g., `1 1 1 1 NOTE 1 C2 95 | 1 0 5`. That is position as bar / beat / 16th / tick (768 per bar), status, channel, data 1 (pitch), data 2 (velocity), then length.
- The current event is shown inverted. Clicking a note in the score selects it, plays it, and highlights its event line. [0:42:04]
- Down the left side is a column of event-type buttons: `ON/OFF`, NOTE (implied), `PROGRAM`, `CONTROL`, `PITCH W`, `C-PRES`, `P-PRES`, `SysEx`, `PSEUDO`, `COMMENT`. The right side has `INS MODE`, `CHORD`, `MIDI OUT`, `MIDI IN`, `CYCLE`. The tutorial doesn't explain any of them yet ("which we'll come to later").
- The white scroll bar at the left of the score scrolls through the staves. [1:04:18]

## 6. Score editing (skimmed; score is out of scope)

- **I** = interpret, which cleans up display rests. It is a toggle and applies to the whole track. **U** = full score, also a toggle (also in Menu Edit > Full Score). **M** = miniature for the selected track; **Shift+M** = miniature for all tracks. Interpret, split and full score are all toggle keys. [0:36:15, 0:44:43, 0:46:52, 1:04:46-1:05:14]
- Clef: click the clef with the left or right button to cycle through the clefs forwards or backwards. [0:37:31]
- **Drum map** (Options > Set Drum Map) [0:38:18-0:41:24]: set an octave base note (C2), then per note choose a display pitch and a notehead symbol. "Force Horizontal Beams" and "Display Drum Clef" are toggles that show light grey when off. Then enable **Mapped Drum** in the X box. **X** = Menu Edit > Edit Parameters. "Single line" rhythm notation is also available.
  ![drum map](../video-frames/0-40-34-drum-map-dialog.png)
  ![drum notation](../video-frames/0-41-09-drum-notation-event-strip.png)
- **Moving notes in the score:** click a note to select it (it plays). Click and hold to get a hand cursor, then drag up/down to change pitch (it sounds as it moves) or left/right to change time. **Group select:** click outside the notes and drag a rubber-band box around them (the selection flashes), then drag one of them and they all move. Used to fix four drum notes that were a 16th late. [0:42:04-0:43:35]
  ![note drag](../video-frames/0-42-16-score-note-drag.png)
- **Full score** shows the tracks in track-list order, which is why tracks were reordered with exchange. [0:47:22] Up to 32 staves on screen. [1:05:14]
  ![full score](../video-frames/0-47-00-full-score.png)
- **Display Parameters** (Menu Edit) [1:05:44-1:06:59]: the X box settings for all tracks in one grid. Clicking a track's number in the left column greys it out, which hides it from the score but **it still sounds**.
  ![display params](../video-frames/1-06-41-display-parameters.png)
- Part box: move the mouse to the bottom of the screen to open it. To place a symbol, left-click it, then hold and drag it into the score. **Right-click** stamps another copy of the last symbol. Slurs and crescendos are different: select with the left button, right-click in the score to set the start, drag out, then left-click to confirm. Slur thickness is in Global Score Parameters. [0:48:47-0:50:06, 1:07:57-1:10:32]
- Key signature: left- or right-click between the clef and the time signature. **Shift** applies it to all staves. [1:06:59-1:07:57]
- Lyrics: do them in single-score mode. Drag the lyric cursor under the first note, type, and Tab to the next note. [1:10:32-1:12:13]
- **Printer page** (Menu Edit > Printer): left/right limits (set the right limit one beat past the end), bar-number step, Justify Last Stave, margins, bars per line, Page Preview, Print Page. Any key aborts printing. [0:50:06-0:53:40]
  ![printer page](../video-frames/0-50-36-printer-page.png)
  ![page preview](../video-frames/0-52-01-page-preview.png)

## 7. Files [0:53:40-0:57:38]

- File > Format Disk, then choose double-sided (726 KB free afterwards). File > Save Song / Load Song use an 8-character name, and the `.SON` extension is added automatically.
- **Loading a song replaces everything in memory.** Check that the title of the file selector says Save Song or Load Song before clicking OK.
- `AUTOLOAD.SON` appears as the song name at start-up (frame f0245).

---

## Relevant to our build

- **Parameter-overview column:** the track list shows one selected parameter for all tracks in a column, and the column follows the parameter selected in the track box. This is cheap and very useful in an ImGui list, so replicate it. SPEC doesn't mention it.
- **Drag a track onto a track:** dropped on an empty slot it copies the track with its parameters; dropped on a recorded track it asks "Exchange or merge?". SPEC 18-1 lists copy, merge and exchange as separate operations. The video shows one gesture with a prompt, so model the UI on that.
- **Hide** = a `*` in the STATUS column left of the track number, toggled by clicking it. SOLO is a button that acts on the selected track. Both match SPEC 6-7. Also note that hiding on the score display page is separate: hidden from the score but still sounding.
- **Loop in beats**, with "play 4 beats, set LOOP 4" as the headline workflow. This matches SPEC.
- **Lowest/Highest:** the copy-and-split workflow confirms the filter use case. The displayed low bound reads `C#-2` and the high bound `G8`. Check the actual defaults and whether the bounds are inclusive (SPEC only says "note range filter").
- **Velocity** offset up to +127 (and presumably down to -127). Matches SPEC 6-16.
- **Quantize display `768`** = off. The resolution is 768 ticks per bar (192 per quarter), and the event list position column is bar/beat/16th/tick. Our tick model should map to this, or the UI could display it this way.
- **Plain RECORD replaces the whole track** ("deletes whole old track") and has a one-bar count-in. Matches SPEC 7-4.
- **Channel display:** port letter A-F + channel 1-16. We replace it with Destination, per SPEC.
- **Key shortcuts worth mirroring as actions:** E (toggle editor), ( ) one bar back/forward, [ ] + L (set bar, then leap), Clr Home / Shift+Clr Home (start/end), numpad 0 / Enter (start/stop), double-click to name, Return for the default button.
- **Event list columns:** `position (bar beat 16th tick) | STATUS | CHANNEL | -1- | -2- | Length/Info`, with an event-type button column (ON/OFF, PROGRAM, CONTROL, PITCH W, C-PRES, P-PRES, SysEx, PSEUDO, COMMENT). Compare with docs/spec-event-editor.md.
- Out of scope but noted: score, drum map, printing, lyrics.
