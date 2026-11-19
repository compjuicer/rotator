# Video training manual of the original (1993) — notes

Source: `vdo/` (2h23m). Transcript: `vdo/work/transcript.txt`. Frames: `../video-frames/` (named h-mm-ss).

- [part1.md](part1.md) 0:00–0:36 — main page, transport keys, ports/MIDI thru, recording, quantize, score basics
- [part2.md](part2.md) 0:36–1:12 — track list overview column, drag copy/exchange/merge, loop, solo/hide, split via lowest/highest; score/print (skimmed)
- [part3.md](part3.md) 1:12–1:48 — event list, tempo/program/signature events, arrange list in depth
- [part4.md](part4.md) 1:48–end — arrange transpose + disable grid, GAM chains, Hyper Edit, RMG, sync

## Build backlog from the video

Missing from what we have:
- Track list overview column: one parameter for all tracks, follows the parameter selected in the track box.
- Drag track onto track: empty slot = copy, occupied = "Exchange or merge?" prompt.
- Disable arrange transpose per destination (port×channel grid in the original; for us track GUID + channel).
- Click arrange entry = set play start; A toggles arrange mode; `[` `]` step bars.
- GAM: right-click in empty lane creates an entry (prompts pattern number).
- Event editor: position as bar/beat/16th/tick (192 ppq, 48 per 16th); insert by dragging from the type palette.
- Window shortcuts: K GAM, J Hyper, E edit, `(` `)` bar step, Clr Home start/end, numpad 0/Enter start/stop.

Confirmed (already in SPEC/build): empty tracks share params, loop in beats, record replaces track, count-in plays previous 4 beats, last entry "no limit", track channel "original", Hyper draw/erase/velocity-by-height, 64 tracks = 4 chains × 16.

Deliberately different: tempo stays in Reaper's tempo map (the original stores tempo as pseudo events on a track).

Open: lowest/highest defaults and inclusiveness; whether Hyper "Convert events" acts on the lane or the whole track.
