# Video notes, part 3 (1:12:13 to 1:48:22)

Source: 1993 tutorial video of the original (version 3.16 on screen). Transcript lines 166-243, frames f0362-f0507.
Everything here is **observed in the video** (treat as [obs]-quality for UI, [verify] where the screen is hard to read).

Topics in this range: lyrics/text/title entry, print layout, score tips (transposing instruments, process note attributes, MIDI meaning, stems, enharmonics, mouse note entry), saving, polyphonic voice separation, time-signature changes, tempo changes via pseudo events, program-change events, and the start of the arrange tutorial (pattern hierarchy, arrange list, arrange mode, recording in arrange).

---

## 1. Main page layout (seen throughout)

![Main page](../video-frames/1-20-46-main-page-track-params.png)
![Main page, demo song](../video-frames/1-38-23-main-page-demo-song.png)

Readable layout of the main page [1:20:46, 1:38:23]:

- **Top bar** (left to right): FREE (memory), ARRANGE (shows `ON` or `--`), a SysEx/del field, SYNC (`intern`), TEMPO (`110.0000`), FORMAT (`1/16`), SIGNATURE (`4/ 4`), a time clock (h/min/sec/frames). Right side: bar counter `1 1 1 1`.
- **Left column**: arrange list with header `ARRANGE a b c d` and a MUTE strip; under it small fields (CATCH, two arrows with values `0`, a highlighted button), then `UP BEAT` / `CUT` (`1 1 1 1`), `TRANSPOSE`, `Pattern LENGTH` (`no limit`), `Pattern POSITION` (`1 1 1 1`). When arrange is empty the program logo fills the area.
- **Centre**: pattern header (`Pattern: 1`, or the pattern name e.g. `Verse 1`) with a STATUS label left and a switchable column label right. 16 track rows: number, name, then a per-track info column. Below the list: `MIDI THRU`, `OVERFLOW`, `SMPTE`.
- **Track parameter box** (`TRACK: n`): CHANNEL, QUANTIZE, GROOVE, TRANSPOS, VELOCITY, COMPRESS, LOOP, DELAY, LOWEST, HIGHEST, GHOST OF. Buttons below: (RNG?), EDIT, DO QUANTIZE, SOLO, HIDE, DEFAULT.
- **Transport column**: left/right locator, AUTODROP, CYCLE, DROP, UNDO, PUNCH, RECORD, `<<` `>>`, `<<<` `>>>`, STOP, START, CONT.
- **Track-list right column is switchable**: the header label over it changes and the column shows that parameter for every track. Seen as `CHANNEL` (values like `A 9`, `F 2`, `B 13`) and as `VELOCITY`/transpose-like values (`127`, `- 25`, `- 20`, `- 30`) [1:38:07, 1:44:29, 1:48:11]. Unused tracks show `A 9` / a default. Some tracks show a short tag like `RUNIT.F` / `ExPort` after the name (instrument/port name per output, [verify]).
- Track names appear in the list; the selected track is inverted.

## 2. Track parameters used in this section

- **Transpose** [1:20:52]: per-track, semitone steps, changeable while playing. It is a playback parameter only; the score display is unchanged [1:21:22].
- **Channel on an event-only track** [1:39:54-1:40:29]: a program-change event goes out on the track's channel, so set the track's CHANNEL to the target device (e.g. B13 for a Proteus) before inserting. Lesson: track channel overrides/stamps the output channel of all its events.
- Tip: select the track (click once) before opening editors/pages, so the page knows which track you mean [1:21:22].

## 3. Recording & quantize (brief)

![Record message](../video-frames/1-30-27-record-deletes-old-track.png)

- Press RECORD, plays a count-in ("one, two, three"), then records onto the selected track [1:30:09]. The top line shows `RECORD: deletes whole old track` (record-replace mode) [1:30:27].
- After recording, a name prompt pops up ("it gives me an OK straight away") — track is named immediately (`Piano`) [1:30:09].
- Quantize then set to `8` (eighths) so the score is clean [1:30:09]; values like `16` shown in QUANTIZE.
- A sustain pedal recorded with the notes shows up as pedal marks in the score; notes are physically short so rests appear [1:31:13, 1:35:05].

## 4. Event editor (event list) and event types

![Event list above score](../video-frames/1-33-01-event-list-above-score.png)
![Empty track, event palette](../video-frames/1-39-00-empty-track-event-palette.png)

- In the score editor (E), an **event list strip** sits above the staves showing the selected notes in "word form" [1:32:30]. Columns: position (`bar beat 1/16 tick`), STATUS, CHANNEL, `-1-`, `-2-`, `Length/Info`. Example rows: `1 1 1 1 NOTE 1 F1 55 1 1 12`, `2 1 1 1 NOTE 3 B1 62 1 0 36` [1:33:01-1:35:50].
- The **dividing line** between event list and score can be dragged down to enlarge the list and reveal more of the event-type palette [1:38:49].
- **Event palette** (left of list): `ON/OFF`, `PROGRAM`, `CONTROL`, `PITCH W`, `C-PRES`, `P-PRES`, `SysEx`, `PSEUDO`, `COMMENT`. Insert by **dragging an event type from the palette into the list**; it lands at bar 1 beat 1 by default, then edit its fields [1:38:49, 1:40:29].
- Right-side buttons seen: CATCH, scroll arrows, INSMODE, CHORD, MIDIOUT, MIDI IN [1:41:24].
- Opening E on an **empty track** shows a small dialog first; press Return and you get a blank stave + blank list [1:38:49].
- With **MIDIOUT** active, clicking/changing a program-change value sends it immediately, so you can audition sounds on the synth while scrolling the number [1:41:10].

## 5. Tempo changes [1:38:24-1:39:54]

- Global tempo shown in top bar (110). To change tempo mid-song: pick an **empty track** (track 16), E, Return, drag a **PSEUDO** event into the list.
- First pseudo event: bar 1 beat 1, tempo 110 (the starting tempo). Second: bar 8 beat 1, tempo 120. Tempo jumps instantly at that point.
- So tempo changes are **events living on a track** in the pattern (and thus subject to the arrange), not a separate global map.

## 6. Program changes [1:39:54-1:42:40]

![Program change event](../video-frames/1-40-28-program-change-event.png)
![Program at bar 8](../video-frames/1-41-24-program-change-bar8.png)

- Same recipe: empty track "out of the way", set its CHANNEL to the target, E, Return, drag `PROGRAM` into the list. Row reads `1 1 1 1 Program <n>`; edit the bar (→ `8 1 1 1`) and the last number (program number).
- Numbering caveat: the program's number may be off by one vs the synth display (Roland "normally one out") [1:40:58].
- Name the track (double-click name) e.g. `PRG` to mark it [1:41:40]. Many program changes per track allowed [1:42:16].

## 7. Time-signature changes [1:36:49-1:38:24]

- Menu Edit → enable "time signature changes" (gets a tick).
- Move the main bar counter with the **`[` `]` keys** to bar 3, set SIGNATURE to 3/4; move to bar 4, set back to 4/4. Changes play back in order.
- Delete all: Menu Edit → click signature changes again → "delete all changes?" → yes.
- Signature changes are positional (per bar), entered from the main page counter.

## 8. Patterns, songs, arrange (the important bit) [1:42:40-1:48:22]

![Arrange default entry](../video-frames/1-44-29-arrange-default-entry.png)
![Arrange on, two entries](../video-frames/1-46-09-arrange-on-two-entries.png)
![Recording in arrange](../video-frames/1-47-14-arrange-record-countin.png)
![Three entries](../video-frames/1-48-11-arrange-three-entries.png)

**Hierarchy** [1:42:57]: song → up to **99 patterns** → **16 tracks** each. Select a pattern by clicking the pattern number at the top of the track list (scrolls to 99). A pattern can be a whole piece or a section.

**Recommended workflow** [1:43:26-1:44:00]: split the piece into sections (intro/verse/chorus), record each into its own pattern. **Every pattern starts at bar 1 beat 1**; record each section from its own start. Click a pattern to audition it alone.

**Pattern naming** [1:45:25]: double-click the pattern name above the tracks (default text `Pattern`), Esc clears it, type name, Return. The name is carried into arrange entries using that pattern.

**Arrange list** (left area) [1:44:29]:
- A new song always has **one entry**: `1 Pattern: 1` (here `1 Verse 1`). Entry columns: start bar, name, pattern number.
- **Add entries**: click-and-hold an existing entry (hand cursor), drag down, release — duplicates it as a new entry. Repeat as needed [1:44:29].
- New entry got bar 5 automatically (`5 Verse 1`) — i.e. placed after the previous one using its pattern length [verify: 4-bar pattern].
- **Delete**: select entry, press Delete [1:44:57].
- Edit the pattern number of an entry to change what plays: `1 Intro 2`, `5 Verse 1` [1:45:51].
- Edit the start bar directly (`9` → `13` for the chorus) [1:46:50].
- Entry selected shows `Pattern LENGTH` (e.g. `4 0 0 0` for the intro vs `no limit`) and `Pattern POSITION` (`13 1 1 1`) underneath [1:46:09, 1:47:21].

**Arrange mode** [1:45:51-1:46:10]: click the ARRANGE box (top bar, shows `ON`) or press **A**. When on, the arrange list takes precedence over the pattern shown; the track list follows the current entry (header changes to `Intro 2`, `Chorus 3`). Patterns play back seamlessly.

**Recording in arrange mode** [1:46:50]: set an entry (bar 13, new empty pattern 3 `Chorus`), select a track/sound, press RECORD: the count-in **plays the previous four beats of the arrangement** (the tail of the preceding entry) instead of a click-only count-in. Screen inverts during recording.

**Play from an entry** [1:47:26]: normally Play starts at bar 1; clicking an entry in the arrange list makes playback start from that entry.

**`[` / `]` during playback** fast-forward/rewind the song (bar steps) [1:47:26].

**Arrange transpose** introduced at the end [1:47:51]: transpose whole sections in semitone/tone steps (continues in part 4).

## 9. Score-only topics (brief)

![Process note attributes](../video-frames/1-24-33-process-note-attributes.png)
![X page polyphony](../video-frames/1-33-34-x-page-polyphony.png)
![SATB separated](../video-frames/1-35-50-polyphonic-satb.png)
![Page preview](../video-frames/1-18-29-page-preview.png)

- **Lyrics/text** [1:12:13-1:16:18]: drag LYRICS symbol in, type word, Tab to next note (music respaces to fit), Return to finish. Font style number (1 large, 2 small; up to 16 with loaded fonts) set above the symbol. Click a word to edit. Delete anything: rubber-band select, Backspace. Text: drag TEXT in, dialog with bold/underline/italic/font/position. Title needs a header area: drag the bracket top-left down (e.g. 35); Print Header must be enabled on the printer page; position left/right/centre.
- **Print layout** [1:16:52-1:20:30]: drag staves by click-hold (hand). Bottom bracket sets print limit/system spacing. Page preview: page number, bars/line, l./r. margin, min. dist, local format (left-click in last bar adds a bar to that line, right-click removes), fix format, flip page, print page. "Print single pattern" prints the whole piece.
- **Transposing instruments** [1:20:30-1:22:54]: X page (select track first, press X) has "transpose upper/lower stave" — display only, playback stays concert pitch. Contrast with track Transpose (playback only). U toggles full score, P page preview.
- **Process note attributes** [1:22:54-1:24:43] (Functions menu): limit positions (bars 1-7), filter by internal channel = 1, then set Staccato ON (also tie direction, stem direction, note head, accent). Bulk score-symbol apply.
- **MIDI meaning** [1:24:43-1:25:40] (Options → set MIDI meaning): score symbols alter playback, e.g. down-bow +50 velocity, staccato divides length by 2/4. Needs MIDI meaning mode switched on.
- **Stems / enharmonics** [1:25:40-1:26:34]: D stems down, Shift+D up; H / Shift+H enharmonic shift.
- **Mouse entry** [1:26:34-1:28:01]: drag note values from bottom-left palette (3 variants: plain, triplet, dotted); takes velocity/length of the last note entered. Hold Alt while picking up → rest. I toggles interpretation.
- **Polyphony** [1:28:59-1:36:49]: uses *internal* (score) channels, separate from the MIDI out channel. S splits stave (split point `C3`), Alt+S assigns upper stave notes to channel 1 and lower to 3. X page: POLYPHONIC on, voice table (voice 1-4 with rests on/off, stems UP/DOWN). Click-hold at stave start to draw a voice separation line; right-click while holding left cycles which voices the line separates (1/2, 2/3, 3/4).

## 10. File handling [1:28:01-1:29:59]

![File menu](../video-frames/1-29-31-file-menu.png)

- File menu: Load/Save SONG, Load System, Load/Save ARRANGE, Load/Save PATTERN, Load/Save SEQUENCE, Load/Save MIDI File, Delete File, Format Disk, New Song, New Pattern, New Arrange, Quit.
- Save over existing → "File exists" → choose **Rename** (keeps backup) or overwrite; tip: Rename always.
- "Re-initialise": reload the AUTOLOAD song (holds named MIDI channels, printer setup, click) rather than New Song, which loses those.

---

## Relevant to our build

- **Arrange entry creation by drag-duplicate** of an existing entry; new entry auto-placed after the previous one. Default song = one entry at bar 1 with pattern 1. Worth replicating as the default state and the "add entry" gesture in our arrange list. [obs]
- **Entry fields**: start bar, name (inherited from pattern name), pattern number; selection shows pattern length / position plus upbeat/cut/transpose fields. Matches SPEC §5; LENGTH shown as `no limit` for the last entry agrees with SPEC "last entry plays with no limit".
- **Arrange on/off toggle (key A)** that overrides the pattern selector, and track list follows the current entry. SPEC covers "arrange/pattern couple"; add the A shortcut and the explicit on/off state if not already there.
- **Count-in in arrange mode plays the previous four beats of the song** — confirms SPEC §7 "count-in plays the end of previous entry" (move to [obs]-ish confidence).
- **Clicking an arrange entry sets the play start**; `[`/`]` step bars (also during playback). Add to transport spec.
- **Switchable per-track info column** in the track list (channel / velocity / transpose…). Nice compact UI; consider for our track list.
- **Tempo changes are pseudo events on a track**, not a song-level tempo map. SPEC currently defers tempo to Reaper's tempo map [ours]; note this as a deliberate divergence.
- **Program changes as events on an "empty" track with its channel set** — confirms track channel stamps output channel. Our event editor should support inserting program changes; track channel must apply to non-note events.
- **Event list columns** (position bar/beat/16th/tick, status, channel, data1, data2, length) and **drag-from-palette insertion** at 1 1 1 1; palette includes Pseudo and Comment. Useful model for our Event editor.
- **Time signature changes positional, set from main counter** — consistent with SPEC §2 (signature belongs to song position).
- **Record-replace message** "RECORD: deletes whole old track" and immediate track-name prompt after first recording; SPEC §7 covers replace/merge.
- Score/polyphony/print/MIDI-meaning: out of scope; MIDI meaning (symbols altering playback) is a possible future idea only.
