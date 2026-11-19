# Video notes, part 1 (0:00:00 to 0:36:15)

Source: 1993 video manual of the original, version 3.16. Atari 4MB STE, SM144 monitor. Gear: Korg M1 (A1), piano module (A9), Alesis SR16 (A16), Proteus/1 via Export (B13), Wavestation (F1, F2).

Everything here is what the video shows or says; treat it as [obs] from video, not hardware we tested.

## 1. Main page layout [0:06:12, 0:16:50 to 0:18:34]

![main page](../video-frames/0-07-46-main-page.png)

The "sequencing page" (main page). Left to right:

- **Menu bar**: Desk, File, Functions, Quantize, MIDI, Flags, Options, Edit, Copy.
- **Title line**: song name (`AUTOLOAD.SON`), "<PROGRAM> 3.16 (c) 1992 …". In record mode this line shows a help text for the button under the mouse, e.g. "RECORD: deletes whole old track" [0:25:49].
- **Top row of fields** [0:17:08]: FREE (bytes free, e.g. 438646), ARRANGE (shows `--` when arrange is off), SyncDEL, SYNC (`intern`), TEMPO (`120.0000`, four decimals), FORMAT (`1/16`), SIGNATURE (`4/ 4`), stopwatch (h min sec ms), main bar counter (BAR / 4 / 16 / 768).

  ![free/arrange/sync](../video-frames/0-17-12-free-arrange-sync.png)
  ![tempo/format/signature](../video-frames/0-17-53-tempo-format-signature.png)

- **Arrange area** (left column): header `ARRANGE a b c d`, MUTE column, list starting with "1 Pattern: 1". Below: fields for cut, transpose, "no limit" (pattern LENGTH), and pattern POSITION `1 1 1 1`. The presenter says to ignore it for now [0:16:50]; covered later in the video.
- **Track list** (centre): header `STATUS | Pattern: 1 | CHANNEL`. 16 rows: track number, name, channel name, port letter, channel number. Below the list: three indicator fields MIDI THRU, OVERFLOW, SMPTE [0:08:38].
- **Track parameter box**: `TRACK: 1`, then CHANNEL, QUANTIZE, GROOVE, TRANSPOS, VELOCITY, COMPRESS, LOOP, DELAY, LOWEST, HIGHEST, GHOST OF. Below: RMG, EDIT, DO QUANTIZE, SOLO, HIDE, DEFAULT buttons.
- **Right column**: LEFT LOCATOR (`1 1 1 1`), RIGHT LOCATOR (`5 1 1 1`), AUTODROP and CYCLE toggles (`--` = off), then transport buttons.

  ![bar counter and locators](../video-frames/0-18-24-bar-counter-locators.png)
  ![track params](../video-frames/0-03-27-track-params-closeup.png)

## 2. Mouse conventions [0:06:59 to 0:07:46]

- Left button = "doing" button: select, click buttons, double-click (load program, open naming boxes).
- Right button = "adjust" with left: on a number field, **left click increments, right click decrements** (tempo 120 to 110 example) [0:07:18, 0:17:34].
- Menus drop down when the pointer touches a heading (no click needed), then left-click an item [0:07:46].
- Time signature: click the left or right half of the field to change numerator or denominator; range up to 99/32 [0:17:51].
- Right-click a field to blank it (Auto-Off Channel in MIDI Thru page set to blank with right click) [0:13:46].

## 3. Bar counter / position format [0:18:08 to 0:19:04]

Four figures everywhere in the program: **bar, beat, sixteenth, 1/768 pulse**. Beat counts per the signature (1-4 in 4/4, 1-3 in 3/4). Counter fields shown with labels `BAR / 4 16 768`. Stopwatch next to it shows h:min:sec:ms of running time.

## 4. Transport and keyboard shortcuts [0:19:04 to 0:20:48]

![transport buttons](../video-frames/0-19-14-transport-buttons.png)

Buttons carry their key on a small label:

| Function | Key | Note |
|---|---|---|
| Record | `*` (keypad) | starts with 4-beat count-in |
| Stop | Enter (keypad) | |
| Start | `0` (keypad) | **always starts from the beginning** (bar 1), so no rewinding needed [0:20:20] |
| Continue / pause (CONT) | `.` (keypad) | toggles: stops at current position, press again to resume from there [0:19:51] |
| Drop | Space | not explained yet |
| Punch | `/` | not explained yet |
| Rewind / fast forward `<<` `>>`, `<<<` `>>>` | `(` `)` labels on keypad | not explained yet |
| Edit (score) | `E` | toggles main page and score page [0:33:37] |
| Delete track | Backspace | confirm box "DELETE TRACK?" Cancel/OK [0:31:40] |
| MIDI channel naming | Esc | [0:09:17] |

The keypad shortcuts matter because the score page has no transport buttons [0:19:51].

## 5. MIDI ports, channels and channel names [0:02:56 to 0:03:54, 0:09:17 to 0:11:01]

- Atari's own MIDI out = port A, channels A1-16. Export interface adds B, C, D; the publisher's sync box adds E and F. A track's channel is chosen as port letter + 1-16 [0:03:06, 0:10:11].
- **Channel names**: press Esc, a naming box opens for the current channel; up to 8 characters; Return closes. From then on every track set to that channel shows the name (e.g. A1 = "Korg M1", A16 = "SR16", B13 = "Proteus") [0:09:17].

  ![channel names in track list](../video-frames/0-10-33-channel-names.png)

- MIDI-in check: play a key and the MIDI THRU indicator under the track list lights [0:08:12].

  ![MIDI in indicator](../video-frames/0-08-38-midi-in-indicator.png)

- Picking a sound = set the track's channel to the module's channel; with MIDI thru on, playing the keyboard immediately sounds through the selected track's channel [0:21:52].

## 6. MIDI Thru page and click [0:12:51 to 0:14:17]

Menu MIDI > MIDI Thru. Box title "MIDI THRU etc", subtitle "Remember: parameters accessed by any empty track".

![MIDI thru page](../video-frames/0-13-25-midi-thru-click.png)

Fields seen: MIDI THRU: ON; Auto-Off Channel; MIDI CLICK: Channel (A 1), Note (C5), Velocity (64); REMOTE: Basic Note (C1), Drop: Control No (65); STOP ends Record (`--`); CLOCK PORT (A); EXIT.

- Click is sent as a MIDI note in record mode, used as a metronome on a drum machine (set to A16, G#2 tom, velocity ~100). The monitor also beeps [0:12:51].
- He blanks Auto-Off Channel with a right click [0:13:46] (no explanation of what it does).

## 7. Autoload song [0:08:46 to 0:16:20]

A song named `AUTOLOAD.SON` on the program disk loads at startup and carries setup: channel names, printer adaptation, MIDI click, etc.

Steps: File > New Song (OK) to initialise; name channels (Esc); load printer adaptation; delete the empty track the score page created (Backspace); set MIDI click; File > Save Song as AUTOLOAD.SON. Disk full: File > Delete File first, then save [0:15:20].

Desktop auto-boot setup (Options > Install Application > Auto, Save Desktop) [0:04:58]: Atari-only, skip.

Takeaway: a "template song" that stores global defaults (channel names, click, thru) is how users of the original set up their studio.

## 8. Tracks and patterns [0:21:09 to 0:22:53]

- 16 tracks in the middle, compared to 16 empty staves. Select with left click on the track number; record on any track in any order [0:21:09].
- Pattern number shown as `Pattern: 1` above the list (one frame at 0:03:27 shows `Pattern: 2`).
- Track rows show: number, name (blank until named), channel name, port, channel.
- **Empty tracks share settings**: after setting track 1's channel to A9, all empty tracks 2-16 also show "Piano A 9" [0:25:49]; after setting track 2 (still empty) to A16 all empty tracks below show SR16 [0:29:30]; after selecting B13 on track 3 all empties show Proteus [0:32:11]. Matches SPEC section 3 (empty tracks share one parameter set) and the MIDI Thru box note "parameters accessed by any empty track".

  ![channel A9](../video-frames/0-25-13-channel-a9.png)

- **Recorded track default name is "OK"** [0:26:38]. Double-click the name to open the 8-character naming box; Return confirms. The track name is the name shown on the score [0:26:38].

## 9. Recording [0:24:27 to 0:32:59]

- Select track, set channel, press `*`. Screen inverts (black background) to show record mode; MIDI click runs [0:24:48].

  ![record mode](../video-frames/0-25-49-record-mode-inverted.png)

- **Count-in of 4 beats** (one bar in 4/4): bar counter runs through the count-in, then bar 1 beat 1 [0:25:17]. Every song starts at 1 1 1 1; anacrusis possible later.
- No hurry to stop: nothing is recorded when you don't play [0:26:05].
- After Stop the recording shows as track named "OK" and plays back immediately with Start.
- Re-recording on a track replaces its content ("RECORD: deletes whole old track") [0:31:40, help line at 0:25:49]. Alternatively Backspace deletes the track with a confirm box.

  ![delete track](../video-frames/0-32-02-delete-track.png)
  ![recording track 3](../video-frames/0-32-11-recording-track3.png)

- Workflow shown: record piano (track 1), drums (track 2), quantize both to 16 so the backing is tight, then record the melody (track 3) on top.

## 10. Quantize as a track parameter [0:28:11 to 0:32:59]

- QUANTIZE is the second line of the track parameter box; the box shows the currently selected track's values (select track first) [0:28:49].
- Default **768** = effectively unquantized (nearest 1/768 of a 4/4 bar, i.e. real time) [0:29:15].
- Set to 16: notes move to the nearest sixteenth [0:29:43]. Also used 8 [0:31:18]. Values are changed with left/right click on the number.
- **Quantize can also be edited in the track list**: the right-hand column of the track list can show quantize values instead of channel ("change the figure either there or in the track window as well at the end of the track") [0:29:58]. Frame shows the column header changed and values `768` / `16` / `8` per track.

  ![quantize column](../video-frames/0-29-30-quantize-column.png)

- Applies to the recorded take without re-recording, and can be changed freely afterwards (he goes 16, then 8 on the piano). Badly played notes snap to the wrong grid line, needing manual editing [0:30:21].
- Also matters for score display: the score can show down to 1/96, so unquantized playing looks messy [0:28:11].
- Quantize set on a track before recording: he sets track 3 to 16 *after* recording [0:32:59]; nothing shown about pre-set values in this part.

## 11. Score editor (skim) [0:33:37 to 0:36:15]

- `E` toggles into/out of the score page for the selected track. There is also an EDIT button [0:33:37].

  ![score page](../video-frames/0-34-05-score-page.png)

- Score page top strip: ON/OFF, PROGRAM, CONTROL, partbox; position fields `1 1 1 1 NOTE 1 F1 55` (an event line: position, type, channel, pitch, velocity).
- `S` = split stave (piano grand staff), default split C3. `X` opens the score parameter box: OVER, REST, INTER, MINI, QUA (def), VOCAL, EMPTY, SPLIT (C3 tick), POLYPHONIC, MAPPED DRUM, SINGLE LINE, transpose/key for upper and lower stave, AUTOUPDATE, UPDATE, EXIT [0:34:36].

  ![score parameter box](../video-frames/0-35-06-score-param-box.png)
  ![split at F#2](../video-frames/0-35-29-split-stave-f2.png)

- Interpretation mode introduced at the end (display recorded 16ths as crotchets) [0:35:45]; continues in part 2.
- Printer setup: Edit > Printer from the score page; load printer adaptation (Epson LQ 24-pin) [0:11:01].

  ![printer page](../video-frames/0-11-48-printer-page.png)

## 12. Hardware setup (skip)

Dongle, MIDI ports, Export interface, printers, desktop install [0:02:26 to 0:05:50]. Not relevant beyond the port letters in section 5.

## Relevant to our build

- **Empty-track parameter sharing** is visible on screen: setting an empty track's channel changes the shown channel of all empty tracks. Confirms SPEC section 3 (auto-copy). We leave thru to Reaper, but our track list should still show what an empty track would get.
- **Quantize editable from the track list**: the list's right column toggles between channel and quantize. Worth copying: a switchable per-track column in our pattern view.
- **Recorded tracks default to name "OK"**; name max 8 chars. We can use a better default but it's a cheap detail.
- **Start always plays from the beginning** of the song/pattern; CONT (`.`) is the resume-from-position toggle. Reaper's transport differs; if we offer actions in the original's style, "play from pattern start" and "pause/continue" are the two to map.
- **Record replaces the track** by default (help line "RECORD: deletes whole old track"), with a one-bar count-in and inverted screen. SPEC section 8 matches; Reaper record modes do the job.
- **Channel names** (global per port+channel, 8 chars, Esc) show in the track list instead of raw numbers. Our Destination already points at a named Reaper track, which covers this.
- **Quantize values in the box are 768/16/8**: the field shows the grid divisor (768 = off), not "1/16". SPEC 4a lists 1/4 ... 1/96 and "neutral"; the UI uses 768 as the neutral value. Our UI can show "off" but the divisor form is what users know.
- Main page shows pattern, track list, track params, arrange list, locators and transport on one screen. Our ReaImGui main window should keep arrange list + track list + parameter box side by side.
- Template/autoload song for defaults (channel names, click): Reaper project templates cover it.
- Position format bar/beat/16th/768th is used everywhere; our event editor should show positions in that 4-field form (with Reaper PPQ in the last field).
