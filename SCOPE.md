# Scope

## In: build it
- Patterns with tracks (ch 4, 6)
- Track parameters incl. loop, ghost, filters, and committing them: normalize, process data, fix, merge (ch 6)
- Quantize modes, preset and user grooves, dynamic groove (ch 17)
- Pattern/track operations: copy, merge, segment copy, multicopy, cut/insert and move pattern (ch 18)
- Pattern editors: Event Editor (ch 9), Matrix (ch 16), Hyper Edit (ch 15), in our own window. Reaper's MIDI editor is not used.
- Arrange: chains, entries, per-entry upbeat/cut/transpose/length/mutes, arrange copy, arrange-to-pattern, recording with arrange running (ch 20)

## Architecture [ours]
- The extension owns the model: patterns, tracks, track parameters, arrange. An arrange list in the style of the original is the main UI.
- Reaper items are rendered output, regenerated from the model. Edits to generated items aren't synced back.
- Output: each pattern track's destination is a Reaper track (by GUID) + optional MIDI channel. Rendering writes one item per arrange entry per destination track; tracks sharing a destination merge into that item.
- Implementation: native C++17 REAPER extension (CMake + Ninja). Pure model (`src/model.*`, doctest tests) separate from the Reaper glue. UI later via ReaImGui's C++ API. Vendored: reaper-sdk, WDL (SWELL headers), doctest.
- Deferred: Buzz-style MIDI wiring (generalizes track parameters and ghosts into a graph). Keep the model open to it.

## Reaper already does it
Transport, locators, cycle; recording modes; length/legato/humanize functions; tempo and time signatures; sync; MIDI thru and input processing; file handling.

## Out
Score, printing, lyrics; RMG; Transform; adaptive groove; hardware peripherals; Soft Link; Atari UI.

## Sources

| File | Covers | Text |
|---|---|---|
| complete manual scan | whole manual, two-page spreads | `text/completo.txt` (tesseract, noisy) |
| partial scan A | 2-10 to 9-39 | good text layer |
| partial scan B | 15-1 to 20-31 | good text layer |
| retyped manual | ch 1–12, own page numbers | redundant |

The PDFs and `text/` are copyrighted and stay local (gitignored).

Cite the manual's chapter-page footer (`[doc 20-24]`).
