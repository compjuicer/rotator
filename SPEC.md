# Pattern sequencer: behavioral spec

Behavioural spec for a pattern/arrange MIDI sequencer in the spirit of a classic 1990 Atari ST sequencer ("the original"), as a Reaper extension.

Tags:
- **[doc X-Y]** from the original's manual, chapter X page Y (manual's own footer numbering)
- **[obs]** confirmed on the real machine
- **[verify]** believed true, not yet confirmed
- **[ours]** our own design decision, not a claim about the original

---

## 1. Concepts

| Term | Meaning |
|---|---|
| Event | Smallest unit of data: a MIDI message with a time position. A note is a Note On plus a Note Off. [doc 4-1] |
| Track | Holds any number of events, any length, up to 16 MIDI channels at once. Processed by its own track parameters. [doc 4-2] |
| Pattern | A handle on 1–16 tracks that belong together. 99 patterns per song. [doc 4-2] |
| Arrange | A "sequencer for patterns": entries that tell patterns when to play. Entries are instructions, not recorded material. [doc 20-2] |
| Chain | One of four parallel lanes (a–d) in the arrange. One pattern per chain at a time, so up to 64 simultaneous tracks. [doc 4-3, 20-2] |

## 2. Pattern

- Holds 1–16 tracks. [doc 4-2] Optional 32-track "double pattern" mode for the whole song: two neighbouring patterns tied together, using chains a and c only. [doc 20-13] We make the track count configurable and skip double patterns. [ours]
- Natural length = length of its longest track. The arrange overrides it. [doc 4-2]
- Pattern 0 is special: as an arrange entry it means "stop" [doc 20-12]; its 16 tracks also store per-channel default track parameters [doc 6-10]. We drop both uses (Reaper has end markers and its own defaults). [ours]
- Naming a pattern names every arrange entry using it, unless that entry was named separately. [doc 6-33, 20-6]
- Time signature belongs to a bar position in the song, not to a pattern or track. Changing it changes all four chains at that bar. [doc 5-4, 5-5] Changing signatures after building an arrange shifts entry lengths in bars and needs manual correction. [doc 5-6] We follow Reaper's tempo/signature map. [ours]
- Resolution: 1/768 or 1/1536 notes, switchable (192 or 384 PPQ). [doc 6-19] We use Reaper's PPQ. [ours]

## 3. Track

- Belongs to one pattern. Events are positioned relative to the pattern start. [doc 4-2, 20-7]
- States: empty (no events, no name), recorded (has events), initialized (no events but has a name and parameters, usable as a template). [doc 6-3]
- Empty tracks share one parameter set (auto-copy) and that set applies to live MIDI thru. Recording or initializing a track removes it from that group. [doc 6-3] We leave thru to Reaper's track input monitoring. [ours]
- Track parameters are per track: changing one track's channel does not touch other tracks. Modifiers apply a change to all tracks in the pattern, to all tracks on the same channel in the pattern, or to all tracks on the same channel in the whole song. [doc 6-1, 6-8]
- Hide: track not transmitted until un-hidden; not automatable. Mute: automatable (screen recording, arrange mutes). [doc 6-7]
- Solo follows the same track number in each pattern of the same chain as the arrange advances. [doc 6-7]
- Per-track length is the LOOP parameter (section 4); there is no separate track length. [doc 6-18]

## 4. Track parameters (non-destructive layer)

Track parameters act only on data as it plays; stored events are untouched. [doc 6-1] Exception in display only: QUANTIZE and GROOVE results show in the event editor, but the original positions are remembered and restored when set back to neutral. [doc 6-1, 6-31]

| Parameter | Behavior | Affects thru |
|---|---|---|
| Channel | Output port + channel, overriding each event's channel. "Original" leaves event channels as recorded. 6 ports × 16. [doc 6-11, 6-13, 6-14] Ours: replaced by Destination = a Reaper track (stored by GUID) + optional MIDI channel 1–16 or "original". The Reaper track does the routing (hardware out, VSTi, FX); port × channel maps to a track's hardware out. No destination = silent. [ours] | yes |
| Quantize | Snap note starts; details in ch 17. [doc 6-15] | no |
| Groove | Swing/feel; details in ch 17. [doc 6-15] | no |
| Transpose | ±96 semitones. [doc 6-15] | yes |
| Velocity | Add/subtract; result clamped to 1–127, giving a limiter effect at the top. [doc 6-16] | yes |
| Compress | Steps 1:2, 1:4, 1:8, then "fix" (all at 64); expand −1:2, −1:4. Velocity is added after compress. [doc 6-17] | yes |
| Loop | Length in quarter-note beats. Track repeats from the pattern downbeat; can't loop from elsewhere. Events past the loop length aren't heard but are kept. Creates a "Track Loop" event whose position can be edited off the beat for polyrhythms. [doc 6-18] | no |
| Delay | ± in 1/768 (or 1/1536) units, optionally shown in ms. Notes advanced before the pattern start aren't heard unless the entry has an upbeat. [doc 6-19, 6-20] | no |
| Lowest / Highest | Note range filter. [doc 6-21] | yes |
| Ghost of | Plays another track's (pattern + track) events through its own parameters, without copying them. Source edits pass straight through. A source can have several ghosts. Ghost's Quantize, Groove and Loop are disabled (source's apply); negative delay not allowed. Editing a ghost offers to make it a real copy. [doc 6-21, 6-22] Ghosts of an empty track process live input (realtime ghosts); Quantize, Groove and negative delay unavailable live, delay only while running. [doc 23-4] Realtime ghosts: out of scope, Reaper input FX do this. [ours] | yes |
| Output filter | Block event types (notes, program, control, pitch, pressure…). [doc 6-24] | ? |
| Channel filter | Only pass events whose own channel equals the Channel parameter. Combined with ghosts, splits a multichannel track. [doc 6-24, 6-25] | ? |

Order of application: compress before velocity [doc 6-17]. Full order not documented; ours: quantize → groove → delay → transpose → lowest/highest → compress → velocity. [ours]

### Committing parameters (event-altering)

- **Normalize** (key N): bakes all set parameters of the track into its events, resets them to neutral; what you hear stays the same. Whole track only. Exceptions: Channel display is kept; Loop can't be normalized; Quantize/Groove aren't normalized; Lowest/Highest deletes notes outside the range. [doc 6-27, 6-28]
- **Process Data**: same, but parameters switch on individually and can be limited to the locator range; adds a start/end velocity fade. Quantize/Groove applied this way can give different feels in different sections, but changing the track's Quantize/Groove later overwrites them. [doc 6-29, 6-30]
- **Fix Quantize/Groove** (key F): makes the current quantized positions the new "original" positions. [doc 6-31]
- **Merging tracks** with differing parameter values bakes each track's own values in; parameters that match stay as parameters. [doc 6-32, 6-33]
- Only the last event-altering operation is undoable; multi-track operations aren't. [doc 6-27] We use Reaper undo. [ours]

## 4a. Quantize and groove

Both are track parameters, reversible, and always computed from the notes' original positions; switching between values never accumulates. [doc 17-2, 17-10] Settable before, during or after recording; a value on an empty track auto-applies after recording, and after each cycle pass. [doc 17-3, 17-4]

**Quantize values:** 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/48, 1/64, 1/96; neutral = unquantized. [doc 17-3]

**Modes** (global, affect future quantizing; re-apply to change existing tracks) [doc 17-4]:
- Quantize all: every event type, not just notes. [doc 17-4]
- Note On and Off: both ends snapped independently. [doc 17-4]
- Note On: starts snapped, ends stay (length changes). [doc 17-5]
- Musical I: starts snapped, length kept. [doc 17-5]
- Musical II (default): like I, but context-aware, chords treated as a whole. [doc 17-5] Its heuristics aren't documented; ours: treat as Musical I. [ours]
- Capture: range (pulses either side of a grid step; outside it notes don't move) and strength (% of distance moved). [doc 17-6, 17-7]

**Preset grooves:** 16A–16F and 8A–8F are 1/16 and 1/8 swing at 50, 54.1, 58.3, 62.5, 66.6, 70.8 %; "8-12" and "1216" combine straight and triplet grids; "five", "sept", "nine" tuplet grids; "Free" leaves offbeats between the straight and triplet positions untouched. [doc 17-11]
- Note lengths are kept; if grooving makes two same-pitch, same-channel notes overlap, the first is shortened. [doc 17-10]

**User grooves (Us1–Us16):** a groove is any track used as a template: each template note is a grid point. Per slot: source pattern + track, range, strength. A looped template repeats, so a 1-beat template grooves any length. [doc 17-12, 17-13, 17-15] Templates can themselves be grooved tracks. [doc 17-18]

**Dynamic groove:** copies template velocities onto the track, amount 0–15; event-altering, applies cumulatively. [doc 17-19]

- Adaptive groove (automatic per-segment choice among up to 8 templates) [doc 17-20 to 17-29]: out of scope. [ours]
- Note-length operations (length quantize, min/max/fixed length, add/subtract, legato, overlap correction, delete short notes, check duplicates, humanize) [doc 19-2 to 19-5]: Reaper's MIDI editor covers these. [ours]

## 4b. Pattern and track operations [doc 18-1 to 18-11]

| Operation | Behavior |
|---|---|
| Track copy | Duplicate to an empty track, same or other pattern, with parameters. Onto a recorded track = merge. [doc 18-1, 18-3] |
| Track merge | Matching parameters stay; differing ones are baked into each side's events (section 4). [doc 18-1] |
| Track exchange / move / replace / delete | Swap two tracks; move to empty slot; overwrite one with another; delete (undoable). [doc 18-1, 18-2] |
| Segment copy | Source track, left (inclusive) to right (exclusive), to a destination track and position; merge or replace; repeat N times end-to-end ("fill"). Source can equal destination. [doc 18-4] |
| Multicopy | Segment copy across selected tracks of a pattern at once, to the same track numbers in another pattern. [doc 18-6, 18-7] |
| Pattern copy | Whole pattern to another pattern number. [doc 18-7] |
| Mixdown | Merge all tracks of a pattern onto the top track; channels baked in, track set to "original". [doc 18-7] |
| Extract one channel / demix all channels | Split a multichannel track by channel into other tracks. [doc 18-8] |
| Extract lines / lines to channels | Split chords into voices (by position in chord, not pitch). [doc 18-9, 18-10] Out of scope. [ours] |
| Cut / reverse inside locators, cut outside | Track-level edits. [doc 18-10] Reaper has these. [ours] |
| Cut and move pattern | Delete the locator range from all tracks of the pattern and close the gap. [doc 18-11] |
| Insert and move pattern | Open a gap of the locator range in all tracks of the pattern. [doc 18-11] |
| New pattern | Clear all tracks of the pattern. [doc 18-12] |

All ranges are left-inclusive, right-exclusive everywhere. [doc 30-7]

## 5. Arrange

- Four chains a–d play simultaneously; their entries are independent and may overlap. [doc 20-2]
- An entry = start bar (+ fine position), name, pattern number, chain. One pattern per entry. List order is irrelevant; the start bar decides timing. [doc 20-3]
- Entries reference patterns, not copies; the same pattern can appear many times. [doc 20-4]
- **Entry length** = distance to the next entry in the same chain. The last entry in a chain plays with no limit. [doc 20-4, 20-25] Two entries at the same start bar in one chain give length zero. [doc 20-25]
- Entry length is a playback setting: a 20-bar pattern in a 4-bar entry plays 4 bars; a 4-bar pattern in a 20-bar entry plays 4 bars then 16 bars of silence (or loops continue, section 6). [doc 20-25, 20-22]
- Changing an entry's length moves every later entry in the same chain. Inserting an entry pushes later ones in that chain back; deleting telescopes them. Other chains don't move. [doc 20-9, 20-11, 20-24]
- Start bar can be as early as −9. [doc 20-8]

### Per-entry pattern parameters (non-destructive) [doc 20-3, 20-4, 20-18]

| Parameter | Behavior |
|---|---|
| Position | Exact start time, for entries not starting on a bar. [doc 20-18] |
| Upbeat | Moves the changeover earlier than the pattern downbeat, so the pattern's pickup notes play; the previous entry in the chain stops that much earlier. Pattern timing doesn't move. Moving the entry to another chain lets both play during the upbeat. [doc 20-20, 20-21] |
| Cut | Moves the changeover later: the entry starts partway into its pattern; the previous entry is lengthened. [doc 20-23] |
| Transpose | Added to each track's Transpose. [doc 20-23] Can be disabled per output channel (for drums). [doc 20-24] |
| Length | See above. [doc 20-24] |
| Mute | Per-track mutes (16 per entry) for the duration of the entry. Shares state with recorded mute events, so a recorded demute can open a track muted by the entry. Mute all / demute all / solo / invert helpers; 10 stored mute groups. [doc 20-27, 20-28, 20-29] |

- Copy transpose / upbeat / mutes to the next entry, or to all following entries. [doc 20-30]
- A whole chain can be switched off (chain hide). [doc 20-29]
- Arrange copy: copy a group of entries elsewhere, limited by positions and selected tracks. [doc 20-31, 20-32]
- Arrange-to-pattern: render one chain's entries into one long pattern. Destination tracks must be empty. Only entry length is honoured; entry transpose, mutes and track loops must be baked in first. Same track number must carry the same channel in every pattern, or the user is warned. [doc 20-31, 20-32, 20-33] Ours: honour all entry parameters when rendering. [ours]

### Recording and editing with the arrange running

- Recording stays on the current track while the arrange moves on; recorded positions are relative to the current pattern's start. A pattern used in several entries plays the new recording in all of them. [doc 20-34]
- What plays back is limited by the entry length, even if more was recorded. [doc 20-35]
- Count-in plays the end of the previous entry. [doc 20-36]
- Global positions: with the arrange on, event positions (and the limits of copy/process functions) can be shown relative to the song instead of the pattern. [doc 20-36, 20-37]
- Arrange/pattern couple: the pattern window follows the current entry; paused automatically while recording; can be switched off to work on one pattern while the song plays. [doc 20-37, 20-38]
- Pattern overview: all 99 patterns with name, whether recorded, and which chain(s) use them; delete from there. [doc 20-39, 20-40]

### Graphic arrange (GAM) [doc 20-40 to 20-49]

A second view of the same arrange: one vertical beam per entry, up to four columns (chains a–d), bar ruler down the side, song position line. Upbeat/cut shown as grey areas. Create by clicking at a bar; drag vertically to change start (end stays put, so length changes); drag horizontally to move chain; drag the corner to change length (later entries follow); Shift-drag copies; Alt-drag moves without changing length. Scrub by dragging in the bar ruler. In Reaper this is the timeline itself; chains map to lanes. [ours]

- The arrange can be saved and loaded separately from the patterns. [doc 20-49]

## 6. Timing and fill rules

- A looped track repeats from the pattern downbeat until stop or until the next entry in the chain takes over. [doc 6-18] So loop phase restarts at every entry. [doc 6-18]
- Tracks with different loop lengths loop independently (polymeter). [verify: implied by per-track LOOP, confirm by ear]
- If an entry is lengthened, looped tracks keep looping through the extra length. [doc 20-22]
- At a pattern changeover, notes still sounding get Note Offs. [doc 20-22] We send note-offs at the entry end. [ours]
- Events past the entry length aren't played. [doc 20-25]
- Notes moved before the pattern start (negative delay, pickups) only play if the entry has an upbeat. [doc 6-19, 20-21]

## 7. Transport

Use Reaper's transport, loop and markers. [ours] Features of the original worth mirroring as actions:
- Jump to the previous/next entry while playing, loading the locators with the entry's start and end. [doc 20-16]
- Up to 10 stored arrange positions, recallable during playback. [doc 20-18]

## 8. Recording

- Record-delete replaces the track; record-merge keeps existing data. Both have a count-in. Drop/punch variants exist. [doc 7-4, 7-5]
- In arrange mode, the count-in plays the end of the previous entry. [doc 7-4, 20-36]
- Cycle record default is overdub: each pass merges. Up to 16 passes are numbered as layers and can be undone or deleted per layer. A replace mode exists. [doc 7-6, 7-7, 7-9]
- Incoming events are stored raw; quantize stays a reversible playback setting. [ours; consistent with doc 7-6, "all quantizing is reversible"]
- Recording in Reaper uses Reaper's record modes. [ours]

## 9. Sync

Out of scope: Reaper handles MIDI clock and MTC. [ours] Notes for the record: chasing MTC from an XRI XR300 works on the STE setup. [obs]

## 10. Verification tests on the real STE

Most earlier open questions are settled by the manual (see doc tags above). What's left needs ears:

| # | Question | Test |
|---|---|---|
| 1 | Polymeter | 4-bar pattern; track A loop 16 beats, track B loop 3 beats. Do they drift as expected and both restart at the next entry? |
| 2 | Track Loop event | Move the Track Loop event off the beat in the event list. What exactly happens? |
| 3 | Parameter order | Quantize + delay + groove on one track: does delay shift before or after quantizing? |
| 4 | Changeover note-offs | Sustained note across an entry boundary, no upbeat: exactly when is the note-off sent? |
| 5 | Ghost across patterns | Ghost of a track in another pattern: does it play when only the ghost's pattern is in the arrange? |

## 12. Pattern editors

Built in house; Reaper's MIDI editor is not used for pattern data. [ours] Specs:
- Event Editor (ch 9): [docs/spec-event-editor.md](docs/spec-event-editor.md)
- Matrix Editor (ch 16): [docs/spec-matrix-editor.md](docs/spec-matrix-editor.md)
- Hyper Edit (ch 15): [docs/spec-hyper-edit.md](docs/spec-hyper-edit.md)

Events cover all channel messages: note, control change, program, pitch bend, channel and poly pressure. SysEx deferred. [ours]

## 11. Out of scope

- Score editor, printing, lyrics (ch 10–14).
- Hardware (the publisher's sync box, Export, Human Touch, Steady Eye), Soft Link (ch 27, 29).
- RMG faders (ch 22).
- Transform and universal maps (ch 24): conditional event rewriting, offline and realtime. Large, not part of the pattern/arrange idea; Reaper's MIDI editor, scripts and JSFX cover the common cases.
- Adaptive groove, Musical Quantize II heuristics, chord-voice splitting.
- Realtime MIDI routing and thru (ch 21, 23), sync, tempo, disk handling, event-altering length functions (ch 19): Reaper does these.
