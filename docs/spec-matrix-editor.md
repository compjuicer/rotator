# Matrix editor: behavioral spec

Draft for merging into SPEC.md. Source: manual chapter 16 (pages 16-1 to 16-3), plus cross-references in ch 9 and 15. Tags as in SPEC.md.

Screenshot: `manual-screens/matrix-16-1.png` (the only figure in the chapter).

## 1. What it is

- A graphical, mouse-driven display of a track's notes as beams; general-purpose, with emphasis on editing note lengths. [doc 16-1] Described as looking like a "pianola" roll. [doc 9-2]
- Part of the event editor, not a separate window. Opened with key K, "MATRIX display" in the Edit menu, or the MATRIX icon on the event editor's left side. K also closes it. [doc 16-1, 9-4]
- Shows notes only; other event types are left to the event list and Hyper Edit. [verify: chapter only ever talks about notes and beams; ch 15 intro says the matrix is "designed to display notes' pitches and positions" [doc 15-1]]

## 2. Layout

- **Vertical time, horizontal pitch.** Pitch runs left to right; time runs downward. A note's start is the top end of its beam, its length is the beam's length. [doc 16-1, 9-2]
- **Keyboard:** a five-octave on-screen keyboard along the bottom edge acts as the pitch ruler. Its base note is set by the LOWEST box (C1 in the figure). [doc 16-2, fig 16-1]
- **Grid:** solid black lines = bar lines (bar number at the left end), grey dashed = beats, dotted = beat subdivisions. The subdivision follows the event editor's "display format" in the top info bar, e.g. 1/16. [doc 16-2, fig 16-1]
- **Zoom:** the ZOOM value sets time resolution as "one pixel = one 1/N note", ranging at least from 1/768 (finest) to 1/16 (coarsest); the figure shows 1/96. Keys `\` / Shift-`\` zoom in/out. [doc 16-2, fig 16-1] Exact list of zoom steps not given. [verify]
- **Scroll:** keypad `(` / `)` scroll one bar; with Shift, smaller steps. [doc 16-2]
- **Split with event list:** the matrix sits above the event list; the divider is draggable. The list can shrink to one line, which then always shows the current event. [doc 16-2]
- Left-side control column: MATRIX, SNAP, VELOC toggles, ZOOM value, LOWEST value. [fig 16-1]

## 3. Linking with other editors

- Matrix and event list are live together: clicking a beam moves the list cursor to that note; edits in either show in both. [doc 16-1, 9-7]
- The matrix can't be open at the same time as the score editor or Hyper Edit. [doc 16-1, 15-2]
- Where the playback cursor is, the list follows it; the chapter doesn't say whether the matrix scrolls with playback. [verify]

## 4. Auditioning

- Left-click on empty matrix plays a note at the clicked pitch. A dot tracks the mouse's pitch on the keyboard. [doc 16-2]
- Dragging a note plays it as it moves ("you will hear the note move"). [doc 16-3]

## 5. Creating and deleting notes

- **Create:** right-click on empty matrix (with VELOC off) inserts a note at the exact pointer position (pitch and time). [doc 16-3]
- New note's length, MIDI channel and velocity are copied from the last note clicked or edited. There's no separate "draw length" setting. [doc 16-3]
- **Delete:** right-click on a beam. [doc 16-3]
- **SNAP:** when on, beams snap to the nearest grid division ("auto-quantize"); when off, placement is free at the zoom resolution. [doc 16-3] Grid = the display-format division. [verify: implied by "nearest division" plus the grid description on 16-2] Whether snap also affects dragging/resizing, not just inserting, isn't stated. [verify]

## 6. Moving and transposing

- Drag a beam horizontally = transpose; vertically = move in time. [doc 16-3]
- The first direction moved locks the axis. Holding the right button as well frees both axes. [doc 16-3]
- Channel and velocity are kept when moving. [verify: not stated, nothing suggests otherwise]

## 7. Lengths

- Drag a beam's lower (end) edge up/down to change length. [doc 16-3]
- For very short notes: press in the empty space just past the beam's end and move up into it to grab the end. [doc 16-3]

## 8. No overlapping notes

- Editing can't make two notes of the same pitch and channel overlap ("nesting"). [doc 16-3] What happens at the limit (blocked, clipped, refused) isn't described. [verify]

## 9. Velocity

- VELOC toggle switches the mouse into velocity mode: click on a beam and drag with left or right button to raise or lower its velocity. [doc 16-3] Which button/direction does which, isn't clear from the text. [verify]
- In VELOC mode right-click doesn't create notes. [doc 16-3]
- Velocity isn't shown graphically in the matrix (no shading or bar); the figure shows plain solid beams. [verify: inferred from fig 16-1 and from ch 15 pointing to Hyper Edit for velocity display [doc 15-1]]

## 10. Not in the chapter

- No multi-note selection, rubber-band, copy/paste, step input or per-note quantize in the matrix. Those live elsewhere (event list, ch 19 functions, step input in ch 7). [doc 16-1 to 16-3, by absence]
- Quantize/groove track parameters do show in the event editor as moved positions (originals kept). [doc 6-1, 6-31] Whether the matrix shows quantized or original positions isn't stated. [verify; presumably same as the list]

## In our context

Per SCOPE.md, Reaper's piano roll covers editing, but generated items aren't synced back to the model. So editing a pattern track needs either our own matrix view or a round-trip through Reaper's MIDI editor. [ours] That decision is open; the rows below hold either way.

| Item | Verdict |
|---|---|
| Notes as beams, pitch × time grid, bar/beat/subdivision lines | keep [ours] |
| Vertical time / horizontal pitch | adapt: horizontal time, matching Reaper's piano roll [ours] |
| Zoom as "pixel = 1/N note" | adapt: continuous zoom [ours] |
| Five-octave keyboard + LOWEST | adapt: scrollable full-range keyboard [ours] |
| Bar-wise scroll keys, `\` zoom keys, draggable list divider, one-line list | drop: Atari UI mechanics [ours] |
| Live link with event list (click selects, edits show in both) | keep if we build a list view; otherwise Reaper's [ours] |
| Exclusive with score / Hyper Edit | drop [ours] |
| Audition on click and on drag | keep [ours] |
| Insert at pointer; length/channel/velocity copied from last touched note | keep [ours] |
| Delete by right-click | adapt: right-click or Del, per Reaper habits [ours] |
| SNAP to grid division, toggleable | keep [ours] |
| Drag with axis lock on first direction; modifier frees both axes | keep [ours] |
| Resize from end edge; grab zone for very short notes | keep, with a minimum hit-area instead of the "behind the beam" trick [ours] |
| No same-pitch/same-channel overlap | keep [ours] |
| VELOC mode, drag on beam to change velocity | adapt: velocity lane or modifier-drag; also show velocity by shade [ours] |
| Edits are destructive to the track's stored events; track parameters (quantize, transpose, etc.) still apply on playback on top | keep [ours; consistent with doc 6-1] |
| Display of quantized vs. original positions | ours: draw original positions, optionally ghost the quantized ones [ours] |
| Multi-select, copy/paste, step input | add: not in ch 16 but expected in any modern editor [ours] |
