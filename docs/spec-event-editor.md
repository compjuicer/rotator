# Event editor: behavioral spec (draft, to merge into SPEC.md)

Source: the original's manual, chapter 9 (pages 9-1 to 9-39). Same tags as SPEC.md. Page numbers follow the manual's own footers.

Screens: `manual-screens/event-editor-9-8.png` (event line anatomy), `event-editor-9-22.png` (comment line), `event-editor-9-23.png` (filter/source icon box), `event-editor-9-26.png` (graphic display beside the list).

---

## 1. Overview

- The event editor is the second of the program's two pages; it shows the events of one track, the one under the track cursor on the main page. [doc 9-1]
- It hosts four editors, all live and linked: the event list (with its graphic display), the Matrix editor, Hyper Edit, and the score editor. [doc 9-1, 9-2] Matrix and Hyper Edit are chapters 16 and 15; score is out of scope. This spec covers the event list and the controls shared by all four.
- Shared controls: QUANTIZE, DEQUANT, DELETE (left of the list); CATCH, CUE, up/down arrows, CHORD, MIDI OUT, MIDI IN, INSMODE, CYCLE, STEPINP, UNDO (right of the list). [doc 9-2]
- Event list and the other editors share the window; the divider can be dragged so the list shrinks to one line, or the other editors can be switched off to give the list the whole window. [doc 9-1, 9-3]
- Keys: G toggles the list's graphic display, K Matrix, J Hyper Edit, N score. [doc 9-4]

## 2. Opening and closing

- Open with E or the EDIT icon on the main page; close with E, Return, OK or the window close box. [doc 9-3]
- Opening an empty track asks for confirmation (suppressible). [doc 9-3]
- Transport keys (start, stop, pause/continue, record-delete), CYCLE (C toggles it off), solo (O) and arrange mode (A) work as on the main page. [doc 9-3, 9-4]

## 3. The event line

One line per event (MIDI or P_USER, the program's own pseudo-events). [doc 9-1, 9-8] Left to right ([doc 9-8], see screen):

| Column | Content |
|---|---|
| Position | bar, beat, format unit, pulse (e.g. `1 1 1 1`) [doc 9-8, 9-9] |
| Status | Note, Control, Program, C-Press, P-Press, PitchWh, SysExcl, P_USER… [doc 9-9, 9-16] |
| Channel | Channel the event was recorded on [doc 9-9, 9-16] |
| -1- | First data byte [doc 9-9, 9-10] |
| -2- | Second data byte [doc 9-9, 9-10] |
| Length / info | Note length (same bar/beat/format/pulse layout), or type-specific info for non-notes (e.g. controller name) [doc 9-9] |

**Position formats** (global switch) [doc 9-9]:
- bar | beat | format unit | pulse (default). FORMAT (display quantization, set in the info bar) and the time signature change how the beat subdivision reads. [doc 9-9]
- hour | minute | second | millisecond.
- hour | minute | second | frame | bit, computed from the sync start offset and the tempo map. [doc 9-9]
- Positions are relative to the pattern start; with "Global Position" on and arrange mode on, relative to the song. The pattern must be the one under the arrange cursor before entering. No effect with arrange off. [doc 9-13]

**Meaning of the data columns by status** [doc 9-17]:

| Status | -1- | -2- | Info column |
|---|---|---|---|
| Note | pitch (e.g. `E4`) | velocity | length |
| Control | controller number | value | controller name |
| P_USER | P_USER number | value | P_USER name |
| C-Press (channel pressure) | amount | unused | |
| P-Press (poly pressure) | pitch | amount | |
| Program | program number | unused | |
| PitchWh | amount (ignored by most synths) | amount (the one normally used; 64 = centre [doc 9-37]) | |
| SysExcl | manufacturer ID | device number | manufacturer name if known |

- SysEx and most P_USER events have no channel. [doc 9-10, 9-16]
- SysEx: header line as above; with "#" on in the filter, the data follows on extra lines of up to six bytes each, with an ASCII rendering in the position column (these lines have no position of their own). EOX is implied and never shown. A Shift-Alt key toggles hex display. [doc 9-10, 9-11, 9-20]
- Note Offs (two kinds: Note Off status, and Note On with velocity 0) are normally hidden by the filter. They follow their Note On's channel and pitch, and move when the note's length changes. When shown, double-clicking one converts it to the other kind. [doc 9-15, 9-16]
- A Note On with no Note Off shows "(missing)" in the length column; a separate Check function repairs it. [doc 9-24]
- Text (P_USER 60) and Lyric (P_USER 61) show their text to the right of the position. [doc 9-21, 9-24]

**Order:** list order is send order; earlier = sent first. Within notes on the same pulse, lowest pitch is listed first. Changing a position re-sorts the list immediately, cursor stays on the moved event. [doc 9-12]

## 4. Cursor and navigation

- One cursor: a block on one value of one event; its column decides what gets edited. [doc 9-4]
- Moved by: up/down arrow icons or cursor keys (each step plays the event via MIDI); right-click on an arrow reverses its direction; hold for continuous scroll; both buttons step a page silently. [doc 9-4, 9-5]
- Clr Home / Shift-Clr Home: first / last event. [doc 9-8]
- Scroll bar: drag to move the cursor; click above/below for page up/down. [doc 9-8]
- Clicking an event in another editor (Matrix, Hyper Edit, score) moves the list cursor to it. [doc 9-4, 9-7]
- Off-screen targets scroll the list; the cursor always stays visible. [doc 9-4]
- No multi-event selection in the list itself; group operations go through dedicated commands (sections 9, 10) or the graphic display lasso (section 8). [verify: chapter describes none]

**CUE** [doc 9-5, 9-6]:
- Left-drag over the CUE icon: scrub through the current track's events, hearing only that track.
- Right-hold: play the whole song from the cursored event until released; moving the mouse changes tempo while held, tempo restores on release. Key # (UK) or \ (US).
- Distinct from Continue ('.'), which starts from the main bar counter.

**CATCH** (L or icon) [doc 9-6, 9-7, 9-11]:
- Display follows playback; on enabling, cursors the event nearest the song position. Turns on automatically at START.
- Cursor keys or any click in the edit area switch it off and freeze the view; L turns it back on.
- While stopped, moving the song position (bar counter, [ ], autolocator, typed position, then L) jumps the editors there.
- Display may lag in dense passages; never affects timing. On stop, the last played event is cursored.

## 5. Editing values

- Every visible value is editable: left/right click (step up/down), Control+mouse drag, mouse-as-slider, +/- keys, typed numbers. [doc 9-11] (Generic value-editing gestures are in ch 3 [doc 3-3 to 3-5].)
- Typing: digits in quick succession form one number; 0 typed alone needs Shift to avoid triggering transport. Works on every field except position. Pitch is typed as a note number (C3 = 60); length in pulses (1/16 = 48 at 768 per whole note). [doc 9-12]
- Edits during playback are heard on the next pass; the sequencer always plays the latest version. [doc 9-11]
- **Status** of Control, Program, C-Press, P-Press and PitchWh can be changed by double-clicking the status name or Control+mouse. [doc 9-15]
- **Channel** is the recorded channel; normally overridden by the track's Channel parameter, but used to tell cycle layers apart. [doc 9-16]
- **MIDI IN** on: playing the keyboard sets values. Cursor on -2-: velocity only; on -1-: pitch only; elsewhere: both. Same for Control (number/value) and C-Press. [doc 9-18]
- **Note length** edits move the Note Off. A length can't be extended past the start of the next note of the same pitch and channel. [doc 9-18]

| Note value | 1/1 | 1/2 | 1/4 | 1/8 | 1/12 | 1/16 | 1/24 | 1/32 |
|---|---|---|---|---|---|---|---|---|
| pulses @768 | 768 | 384 | 192 | 96 | 64 | 48 | 32 | 24 |
| pulses @1536 | 1536 | 768 | 384 | 192 | 128 | 96 | 64 | 48 |

[doc 9-18]

### Position editing

- Position fields scroll per column (bar, beat, format, pulse). Control+mouse for large jumps. [doc 9-12, 3-4]
- **Insert mode** (Shift-I, INSMODE icon, menu): moving an event moves every later event by the same amount. Use: open a gap, or shift a whole track by moving its first event (late start after count-in). Moving earlier can merge into existing events; undo recovers. Manual warns to turn it off straight after use. [doc 9-13, 9-14, 9-37, 9-38]
- **Pickup clock** ('/' on keypad): sets the cursored event's position to the current song position at the moment of the key press; meant for tapping cues to video, with Insert mode on to keep list order. [doc 9-14]
- **Quantize single event** (Q or QUANTIZE icon): snaps one event to the current FORMAT, computed from its original recorded position, not its current (track-quantized) one. So a 1/16-quantized track can have individual events at 1/12. [doc 9-15]
- **De-quantize single event** (Shift-Q or DEQUANT): back to the original recorded position. [doc 9-15]

## 6. Inserting events

- Drag an icon out of the filter box (left of the list) into the list: new event of that type. Dropping exactly on an existing event gives the new one the same position. [doc 9-19]
- Insert key: new Control event (double-click status to change type). Shift-Insert: new Note. [doc 9-19]
- Examples: Program change at 4 1 1 1 reads `4 1 1 1 Program 1 27` [doc 9-35]; sustain off = Control 64 value 0 [doc 9-37]; pitch-bend reset `PitchWh 1 0 64` [doc 9-37].
- New notes get their Note Off automatically. [doc 9-24]
- **SysEx:** drag the SysEx icon in for a header (-1- manufacturer, -2- device). With "#" on, drop another SysEx onto it to add a six-byte line; drop the header onto a data line for further lines. Click an empty slot to add a byte; Delete or Shift-double-click removes one. [doc 9-20]
- **Text event** (Esc): new P_USER 60 plus a text window. Esc on an existing text event re-opens it. In the window: Return ends, cursor keys move, Esc clears all, Delete/Backspace delete, Insert inserts a space. Text events scroll with CATCH during playback (auto-scrolling text). [doc 9-20, 9-21]
- **Comment** (Shift-Esc) on any event except SysEx/P_USER: text attached to that event, moves with it. With COMMENT on in the filter, it replaces the info to the right of the status. Shift-C toggles original/comment view. Clear by emptying it. Not shown in the score. Example use: naming a Program event. [doc 9-22, 9-23]

## 7. Deleting and copying

- Delete key or DELETE icon marks the cursored event as deleted; Space or leaving the editor removes marked events. [doc 9-19]
- Dragging an event by its status column to the left edge deletes it. [doc 9-19]
- Text: drag out, Delete then Space, or open and clear. [doc 9-22]
- Copy: drag an event by its status column up or down and release; the copy lands at the drop position. [doc 9-19] No clipboard copy/paste is described in the chapter. [verify]

## 8. Graphic display

- Left of the list, one horizontal beam per list line, aligned to it. [doc 9-25]
- Background is one bar: left edge = bar start, solid lines = beats (numbered, up to six, per time signature), dotted lines = FORMAT subdivisions (whole fractions at 1/24 and finer). [doc 9-25]
- Beam start = position within the bar; beam length = note length, clipped at the right edge; non-notes are short blips. 1 pixel = 1/96 note. [doc 9-25]
- Purpose: spot chords and doubled notes at a glance. [doc 9-1, 9-26]
- Drag a beam sideways to move the event within the bar; drag its lower right corner to change note length. [doc 9-26]
- **Length copy:** drag from the top note's corner downwards over the following notes (lasso) and sideways; on release they all get that length, positions unchanged. Meant for equal chord lengths. [doc 9-26, 9-36]

## 9. Fast transform and fast delete/keep

- **T**: copy the value under the cursor (Channel, -1-, -2- or length) to the next event of the same status. **Shift-T**: to all following events of that status. [doc 9-31] Examples: equal chord lengths (T twice); same velocity from here on (Shift-T). [doc 9-36, 9-37]
- Manual notes that Velocity/Compress track parameters are usually better (reversible) but whole-track. [doc 9-37]
- **Shift-Delete**: delete all events in the track with the cursored event's status; for Notes only the same pitch, Control only the same controller number, P_USER only the same number. [doc 9-32, 9-38]
- **Alt-Shift-Delete**: the inverse within that status: keep that pitch / controller / P_USER number, delete the rest of that status. Other statuses are untouched. [doc 9-32]

## 10. Display filter

- The same icon box that sources new events toggles each status shown/hidden: Note on/off, Program, Control, Pitch wheel, C-Press (with a P-Press toggle), SysEx (with "#"), Pseudo (P_USER), Comment. [doc 9-23; icon list read from screen, verify grouping]
- Display only: hidden events still play and are kept. [doc 9-23] Muting output is the track's Output Filter instead. [doc 9-24, 9-38]
- "#" off hides SysEx data lines and P_USER extra info (not text/lyric). [doc 9-23, 9-24]
- When the filter hides the cursored event, it stays visible until the list is scrolled, so the cursor doesn't jump. [doc 9-24]

## 11. Monitoring and helpers

- **MIDI OUT** (Shift-O): edits are sent as they're made. Turn off for Control/Program/SysEx work or silent edits during playback. [doc 9-30]
- **CHORD**: notes on exactly the same pulse play together when cursored. [doc 9-31]
- **CYCLE** (Bar Cycle): cycle playback around the bar containing the cursor. [doc 9-31]

## 12. MIDI step input

- STEPINP, Shift-K or menu. While stopped, no record mode, no metronome; notes from the keyboard are written at the step position. A chord advances once all its keys are released. [doc 9-27]
- Tab: rest, advance one FORMAT unit. [doc 9-27]
- Step size = FORMAT, as long as note length doesn't exceed it; a default 1/16 note is too long for consecutive 1/16 steps because a gap of at least one pulse is kept, so the manual advises shortening the first note (e.g. to 20 pulses). [doc 9-27, 9-28] [verify: whether later step notes inherit that edited length]
- Editing a note interrupts the flow: the next note goes to that note's position. Clicking a note's status sets the restart point for Tab/further input. [doc 9-28, 9-29]
- (Score-based variant with note values from a palette: score edition only, out of scope.) [doc 9-29, 9-30]

## 13. Drum names

- A song-wide drum map gives any note number a four-character name (Options > Set Drum Map, one octave per screen). "Mapped Drum" (Edit menu, per track) shows names instead of pitches in the list; unnamed notes show pitch. Saved with the song. [doc 9-33, 9-34]

## 14. Undo

- All edits since entering the editor count as one operation for UNDO; UNDO toggles between before and after. Leaving the editor fixes the edits. [doc 9-34]
- Consequence: undoing a second change also re-applies the first one; manual advises leaving and re-entering (E E) after each important change. [doc 9-34, 9-35]
- Shift-Undo reverts everything since the last fix and fixes it. [doc 9-35]

## 15. Relation to track parameters

- Track Quantize/Groove results show in the list; originals are kept (SPEC section 4). [doc 6-1, 6-31] Single-event Q/Shift-Q works from those originals. [doc 9-15]
- Channel column is overridden by the track Channel parameter on playback. [doc 9-16]
- LOOP's Track Loop event lives in the list. [doc 6-18]
- For range-limited or conditional changes the manual points to Process Data and Transform instead. [doc 9-39]

---

## In our context

Event list = the pattern-track editor. REAPER's MIDI editor stays available for piano-roll work; this is the tracker-like numeric view. [ours]

| Item | Verdict |
|---|---|
| Event line columns (position, status, channel, -1-, -2-, length/info) and per-status meaning | keep [ours] |
| Position as bar beat format pulse, relative to pattern start | keep; pulses = REAPER PPQ [ours] |
| Global (song-relative) position | adapt: show position within the selected arrange entry's placement in the song [ours] |
| ms / SMPTE frame position display | drop; REAPER's ruler covers it [ours] |
| Send-order sorting; chord lowest-pitch-first | keep [ours] |
| Cursor on a single value; arrows; Home/End for first/last | keep [ours] |
| Audition on cursor move, CHORD, MIDI OUT toggle | keep [ours] |
| CATCH follow playback | keep [ours] |
| CUE scrub / hold-to-play with tempo bend | drop; REAPER scrub and transport [ours] |
| Bar cycle | adapt: set REAPER loop to the cursor's bar [ours] |
| GEM scroll bar, mouse button swap, right-click arrow reversal, empty-track dialog, keys opening other editors | drop [ours] |
| Value editing by click, drag, +/- and typed digits | keep; Shift-0 workaround drop [ours] |
| MIDI IN value entry | keep [ours] |
| Status change for channel-voice types | keep [ours] |
| Note length edit with same-pitch overlap clamp | keep [ours] |
| Note Off rows and Off-type conversion; "(missing)" + Check | drop; notes stored with length [ours] |
| Insert mode (ripple later events) | keep, with a visible mode indicator [ours] |
| Pickup clock (tap position during playback) | keep as an action, low priority [ours] |
| Single-event quantize / de-quantize from original position | keep; depends on reversible quantize [ours] |
| Insert by type (dragging from filter box, Insert / Shift-Insert) | adapt: insert commands per type at cursor; same-position drop rule kept [ours] |
| Delete by mark-then-Space | adapt: delete immediately, rely on undo [ours] |
| Drag-to-copy single event | adapt: duplicate command plus clipboard copy/paste of events [ours] |
| SysEx editor | adapt: hex byte lines; manufacturer name lookup optional [ours] |
| Text events (P_USER 60) | adapt: MIDI text meta events [ours] |
| Lyric events, comments' score interplay | drop [ours] |
| Comments on events | adapt: per-event note stored with our pattern data, shown in info column [ours] |
| Other P_USER events | drop, except our own Track Loop marker [ours] |
| Display filter per status | keep [ours] |
| Graphic display (bar grid beams beside list) | keep [ours] |
| Beam drag / length lasso | keep length lasso; beam drag low priority [ours] |
| Fast transform T / Shift-T | keep [ours] |
| Fast delete / keep (Shift-Delete, Alt-Shift-Delete) | keep [ours] |
| Step input (original style, FORMAT steps, Tab rest) | keep [ours] |
| Score-based step input | drop [ours] |
| Drum names | adapt: read REAPER's per-track note names instead of our own map [ours] |
| Session-wide toggle undo, Shift-Undo | drop; one REAPER undo point per edit [ours] |
