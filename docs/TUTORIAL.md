# Rotator — tutorial

Build a song the 1990 Atari way: write short **patterns**, then place them in an **arrangement** of up to four parallel chains. REAPER plays the result. You never edit REAPER's MIDI items directly; they're output, rebuilt every time you change something.

## 1. Setup

1. In REAPER, make one track per instrument (a drum sampler, a bass synth, a pad…). These are the **destinations**.
2. Run the action **Rotator: Show window**. Use the **Dock** button (top-left) to put it in REAPER's docker.

The window has two halves: **arrange** on the left, **pattern** on the right. Below the pattern's track table are three editors: **Events**, **Matrix**, **Hyper**.

Everything you do is one REAPER undo step, saved inside the project file.

## 2. Your first pattern

1. In the pattern panel, the selector shows `1 (empty)`. Click **Create**, then type a name, e.g. `Groove`.
2. Click **Add track**. A row appears in the track table.
3. Name it `Drums` and pick your drum track in **Destination**. Set **Chan** to `10` if your sampler wants it, or leave `orig` to keep each event's own channel.
4. Add a second track `Bass` → your bass synth.

### Entering notes

Select the `Bass` track row, then open the **Matrix** tab (or press **X**). Time runs downwards, pitch across, a keyboard along the bottom.

- **Right-click** an empty spot: insert a note (uses the length and velocity of the last note you touched).
- **Right-click** a note: delete it.
- **Drag** a note: move it. The first few pixels decide whether it moves in time or pitch; hold **Shift** to keep it locked that way.
- **Drag a note's bottom edge**: change length.
- **Alt-drag** (or turn on **VELOC**): change velocity.
- **Drag on empty space**: rubber-band select. **Ctrl-click** adds/removes, **Ctrl-A** selects all, **Delete** removes.
- **SNAP** quantizes moves to the grid. Click the keyboard to hear a pitch.

For drums, the **Hyper** tab (**J**) is quicker:

1. Click **New set**, then **Add lane** for each drum sound. Turn on **Params** to edit a lane: name, event type, note number, grid, note length.
   Or click **Lanes from track** to make lanes from notes already on the track.
2. In a lane, **left-drag** to paint hits on the lane's grid; the height of each beam is its velocity. Turn on **FixVel** for a constant velocity.
3. **Right-drag** erases. **Alt-right-drag** erases only hits that sit on the grid. **Alt-left-drag** inserts without overwriting. **Ctrl-drag** raises or lowers the whole lane's values.
4. Lanes marked **hihat** form a group: a hit in one removes hits at the same spot in the others (closed/open hats).
5. A lane's **delay** shifts its events on playback, non-destructively, for every track using the set.

The **Events** tab (**E**) is the precise list: bar, beat, unit, pulse, status, channel, data, length.

- **Arrows** move around. **+/-** change the selected value. **Enter** types a value directly.
- The **Note / Control / Program / …** buttons insert that event at the cursor. Or drag one onto a row's Status cell to insert at that row. **Insert** = Control, **Shift-Insert** = Note.
- **Delete** removes; **Shift-Delete** removes every event of the same kind (that pitch, that controller); **Alt-Shift-Delete** keeps only those.
- **Cmd-C / Cmd-V / Cmd-D**: copy, paste, duplicate. **T**: copy the selected value to the next event of the same kind; **Shift-T** to all following ones.
- **Step** on: play notes on your MIDI keyboard to enter them one after another; **Tab** adds a rest.
- **Catch** (**L**) follows playback. While playing, **/** moves the selected event to the play position.

You can also record in REAPER as normal, select the recorded item, and click **Capture selected MIDI item into track**.

> Hearing notes while editing: editor audition goes through REAPER's virtual keyboard, so the destination track must be armed with input monitoring on.

### Track parameters

Everything in the track table is non-destructive; the events stay as you entered them.

| Column | Does |
|---|---|
| Trn | Transpose in semitones |
| Vel | Velocity offset, −127…+127 |
| Loop | Repeat the first N beats for as long as the entry lasts; `off` = play once |
| Delay | Shift in quarter notes, may be negative |
| Low / High | Only notes in this range play — copy a piano track and split it at a key into bass and right hand |
| M / H | Mute / hide (hidden tracks are silent too) |

**Loop** is the classic trick: play four beats of a groove, set Loop to `4`, and the track repeats for the whole entry. Give two tracks different loop lengths (say 3 and 4) and you get polymeter, restarting together at every new entry.

Drag a track's **number**:
- onto the **+** row: copy it to a new track;
- onto another track: **Exchange** (swap them) or **Merge** (add its events to the target).

## 3. Arranging

Make a second pattern (**New pattern**), say a `Fill`. Now the left half.

### List view

- **Add** appends an entry with the current pattern after the last one. **Duplicate** copies the selected entry to the end. **Delete** removes it and closes the gap.
- Each row: start **Bar**, **Name**, and a pattern number in one of the chain columns **a–d**. Click an empty chain cell to move the entry to that chain.
- Below the list: the selected entry's **Pattern**, **Transpose**, **Length**, and per-track **Mutes** (1–16).

How long an entry plays: until the next entry in **its own chain** starts. The last entry in a chain plays its pattern once through ("no limit"). Patterns don't have a fixed length; the entry decides.

### Graphic view (press **K**)

Four vertical beams, one per chain, time running down. The red dashed line is the play position.

- **Right-click** in an empty lane: new entry there, using the current pattern.
- **Drag** an entry: move it in time or to another chain.
- **Drag the bottom edge**: change its length; later entries in the chain move with it.
- **Delete**: remove and close the gap. **Ctrl-wheel** or **+/−**: zoom.

### Why four chains

Chain **a** carries the song: verse, chorus, verse. Chains **b–d** play alongside it: a long pad part running under several **a** entries, a busk track, a controller sweep. Each chain has its own entry lengths, so a 12-bar entry in **b** can run under three 4-bar entries in **a**.

### Transpose and the drum exception

An entry's **Transpose** moves every note in it, so a second verse can go up a tone. Drums shouldn't move. Open **Disable arrange transpose** under the arrange panel: each destination track gets 16 channel toggles. Switch on your drum track's channel, and its notes ignore entry transposes.

## 4. Playing

REAPER plays the rendered items, so its transport, tempo map and time signatures all apply.

- Clicking an entry moves REAPER's edit cursor to its start.
- **ARR ON / OFF** (or **A**): with arrange off, REAPER loops the selected entry. Use it to work on one part while it plays.
- **[ ]**: cursor one bar back or forward.
- Numpad **0** play from the start, numpad **Enter** stop, numpad **.** pause.

These keys work when the Rotator window has focus and you're not typing in a field.

## 5. Keys

| Key | |
|---|---|
| K | List ↔ graphic arrange |
| E / X / J | Events / Matrix / Hyper tab |
| A | Arrange on/off |
| [ ] | One bar back/forward |
| Numpad 0 / Enter / . | Play from start / stop / pause |
