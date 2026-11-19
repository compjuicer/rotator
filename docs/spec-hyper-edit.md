# Hyper Edit: behavioral spec (draft, to merge into SPEC.md)

Source: the original's manual, chapter 15, pages 15-1 to 15-22. Tags as in SPEC.md. Screens: `manual-screens/hyper-edit-15-1.png` (drum set in use), `manual-screens/hyper-edit-15-3.png` (instrument parameter columns).

## 1. What it is

- A graphical editor for one event type at a time inside a track, even when that type is mixed with other events. Used to create material (drums, controller curves) and to analyse a track. [doc 15-1, 15-3]
- Always shows one track. Splitting it into lanes per pitch/controller makes it look like several tracks. [doc 15-3]
- Part of the event editor; event list, score and Hyper Edit are live at the same time and editing one updates the others. [doc 15-2] Matrix editor does not run alongside it. [doc 16-1]

## 2. Layout

- **Lanes ("Instruments")** stacked vertically; time runs left to right. Each event matched by a lane is drawn as a vertical beam: left edge = exact time position, height = value. [doc 15-1, 15-2, 15-11]
- Left of the grid: per-lane parameter columns NAME, QUA, DLY, HHAT, STATUS, CHA, -1-, LEN. The grid can be widened to cover them so only the name stays visible. [doc 15-4, 15-5; screen 15-3]
- Above the grid: one row of Set parameters (SET, horizontal zoom, vertical zoom, PEN, COLOR, AutoIns, FixVel, AllCha, LockScore). [doc 15-10; screen 15-1]
- Bar ruler over the grid; vertical grid lines follow each lane's QUA value, so different lanes can show different grids. [doc 15-5; screen 15-1]
- Horizontal zoom = number of bars on screen. Vertical zoom = number of lanes on screen; lane height adapts so lanes are always fully visible. Lanes beyond the view are reached by scrolling, page by page. [doc 15-10, 15-11]
- Positioning (song position, bar-by-bar stepping) is shared with the rest of the event editor. [doc 15-4]
- **PEN** 1–16: beam width, growing to the right of the event's position; wider = easier to grab. [doc 15-2, 15-11]
- **COLOR** 1–4: background/beam rendering styles; style 3 suggested for drums, 1 for continuous controllers. [doc 15-11] With wide pen on a dense controller lane, beams merge into a continuous curve. [doc 15-16]

## 3. Sets (lane presets)

- 8 Sets, each up to 16 lanes, so up to 128 distinct event definitions. [doc 15-1, 15-2]
- Set parameters are per Set and can be changed any time. [doc 15-10]
- Switching Set swaps the whole lane layout over the same track; typical use is one Set per drum machine/module key map. [doc 15-8, 15-21]
- Lane order is display only; two lanes can swap positions without touching data. [doc 15-19]
- Where Sets are stored (song, global) is not stated. [verify]

## 4. Lane definition (what an event must match)

An event shows in a lane when it matches all of: [doc 15-6]
- **STATUS**: any MIDI status or P_USER (pseudo) event. "*undef*" = lane unused. [doc 15-6]
- **-1-** (first data byte): a specific value (a pitch like C4, controller number like 7) or ALL. Ignored for channel pressure, program change and pitch bend. For P_USER, the P_USER number (P_USER 1 = tempo). [doc 15-2, 15-7]
- **CHA**: MIDI channel 1–16; only a condition when Set parameter AllCha is off. [doc 15-3, 15-7, 15-12]

Value shown as beam height is normally the second data byte (note velocity, controller value). [doc 15-2] For statuses with one data byte (program, channel pressure) and for pitch bend, which value is drawn and at what range is not stated. [verify]

Other lane parameters:
- **NAME**: label only. [doc 15-5]
- **QUA**: entry grid for this lane only. Does not move existing events; only mouse-entered events snap to it. Values follow the track quantize values (screens show 4, 8, 16, 32, 48, 96). [doc 15-5, 15-15]
- **DLY**: per-lane offset in clock pulses (or ms). Unlike everything else in Hyper Edit it moves the lane's existing events; new events entered get the offset too. A coarse step mode jumps in musical fractions (1/96, 1/64…). [doc 15-5, 15-6] Whether changing it back restores the original positions exactly is not stated. [verify]
- **HHAT**: mutual exclusion group, see 6. [doc 15-6]
- **LEN**: length of notes entered with the mouse, in pulses; manual advises ~16, not 0 or 1, so On and Off aren't sent together. [doc 15-7]

**Auto-define:** pick an event in the event list and send it to the first undefined lane (key or drag-and-drop); the list cursor then moves to the next event, so repeating the command defines a drum kit lane by lane. [doc 15-8] The same event can't be defined twice ("This Event is defined as Instrument"); with no undefined lane left the user is told to free one. [doc 15-9]

Whether one event can appear in two lanes (e.g. a pitch lane and an ALL lane) is not stated. [verify]

## 5. Editing values and events

- **Change value**: drag a beam vertically. [doc 15-9]
- **Insert**: with AutoIns on, left-click in an empty spot inserts; dragging horizontally inserts along the way. With AutoIns off, Shift+click does the same; plain dragging then only changes existing values. [doc 15-9, 15-12, 15-14]
- **Insert-only**: Alt+left-drag inserts in empty spots but never changes existing events, including ones just inserted in the same gesture. [doc 15-14]
- **Delete**: right-click a beam; right-drag wipes, regardless of AutoIns. [doc 15-9, 15-10, 15-12]
- **Delete on grid**: Alt+right-drag deletes only events exactly on the lane's QUA steps (e.g. 1/8 over sixteenths leaves the off-beat sixteenths). [doc 15-14]
- Wiping inserts at the lane's QUA spacing; changing QUA mid-way gives different event densities in different sections. [doc 15-10]
- **FixVel**: inserted notes get a fixed velocity = last velocity edited in event list or Hyper Edit. With AutoIns off, dragging over note beams sets them all to that velocity. [doc 15-12]
- **Relative scale**: Ctrl + mouse movement (no click) raises/lowers all events of the pointed lane together, including events outside the view. Clamped to 127 and 0 or 1; clamped changes don't reverse. [doc 15-15]
- Controllers: an entered value holds until the next one, so sparse entry (1/16–1/32) usually suffices; avoid flooding memory. [doc 15-17]
- No range/marquee selection is described; operations target a lane (the whole lane) or what the mouse passes over. [doc 15-14, 15-15, 15-19]

## 6. Hihat groups

- Ticking HHAT on neighbouring lanes forms an exclusive group: entering a note in one lane deletes a group-mate's note at the same time position (open hihat replaces closed). Applies to mouse-entered notes. [doc 15-6, 15-18]
- Several groups per Set, separated by at least one unticked lane; a group can hold up to 16 lanes. [doc 15-6]
- "Delete all duplicated HiHat events": retroactively removes coinciding events in group-mates, priority to the lane it is run from (e.g. for real-time recordings). [doc 15-20]
- "Same time position" = exact pulse or same grid step is not stated. [verify]

## 7. Lane functions (copy / convert / groove)

- **Copy lane**: drag one lane's name onto another; events are copied and take the destination's status, channel and pitch (e.g. double a snare with a clap). [doc 15-19]
- **Convert lane**: rewrite all of a lane's events to a given status / channel / -1- (kick → shaker). Lane name isn't changed. [doc 15-21]
- **Convert all lanes to Set N**: rewrite each lane's events to the definition of the same-numbered lane in Set N; remaps a drum track between modules whose kits are defined in the same order in two Sets. [doc 15-21, 15-22]
- **Do Groove of current Instrument**: apply a quantize/groove value (presets, user grooves) to one lane only. 16A and 8A equal straight 1/16 and 1/8. [doc 15-20] Combined with DLY and Ctrl-scaling this gives per-drum feel, delay and level within one track. [doc 15-20] Whether it is destructive or reversible like the track quantize is not stated. [verify]

## 8. Other

- LockScore: score editor scrolls bar-locked with Hyper Edit, formatted by its horizontal zoom. [doc 15-13]
- MIDI output toggle also applies in Hyper Edit (edits audible). [doc 15-4]
- Drum workflow: merge all drum parts onto one track, auto-define one lane per drum note in a free Set, name them, make HHAT groups (hats + crash; toms), set each lane's QUA. [doc 15-17, 15-18]

## In our context

Since Reaper items are regenerated from the model and not synced back (SCOPE), editing a pattern track's events needs our own editor; this partly revises SPEC §11, which lists ch 15 as out of scope. [ours]

| Item | Verdict |
|---|---|
| One track, lanes defined by status + first byte (value or all) + optional channel | keep [ours] |
| Beam per event, height = value; drag to change | keep [ours] |
| Per-lane entry grid (QUA) that doesn't move existing events | keep, values in Reaper PPQ [ours] |
| Insert / delete / wipe / insert-only / delete-on-grid | keep as gestures; modifier keys our own choice [ours] |
| AutoIns toggle | keep [ours] |
| LEN for entered notes | keep [ours] |
| FixVel, Ctrl relative scaling with clamping | keep [ours] |
| HHAT exclusion groups + delete duplicates | keep [ours] |
| Copy lane, convert lane, convert Set-to-Set | keep [ours] |
| Sets as named lane presets | adapt: any number, stored per song and exportable as drum maps [ours] |
| Auto-define from an event | adapt: "add lane from event under cursor / all distinct events in track" [ours] |
| DLY per lane | adapt: non-destructive per-lane offset in the model, like track Delay [ours] |
| Do Groove per lane | adapt: per-lane quantize/groove, reversible like the track parameter [ours] |
| P_USER lanes (tempo) | drop; tempo is Reaper's map [ours] |
| PEN, COLOR, zoom-as-lane-count, scroll paging, parameter-area divider | drop as specified; ordinary zoom/scroll and a fixed beam style [ours] |
| LockScore, MIDI out toggle, keyboard shortcuts | drop [ours] |
| Event list / matrix coexistence | drop; our editor is the only view of model events for now [ours] |
