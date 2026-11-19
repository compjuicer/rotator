# Rotator

A REAPER extension (C++17, ReaImGui) with a pattern/arrange MIDI sequencer in the style of a 1990 Atari ST classic. Refer to that program as "the original"; never write its name anywhere in the repo.

## Layout
- `src/model.*`, `song_io.*`, `ed_*_ops.*`, `recorder.*`: pure logic, no REAPER/ImGui; covered by `tests/`.
- `src/ui.cpp`: main page, RMG, edit page host. `src/ed_*.cpp`: event list, matrix, Hyper Edit. `src/gem.*`: alerts, file selector, overview, notepad. `src/st.*`: Atari ST drawing toolkit in 640x400 ST pixels.
- `src/loader.cpp`: REAPER entry. Dev builds hot-reload the core and poll the `.bridge/` file bridge; release builds (`-DROTATOR_DEV=OFF`) link the core in.
- `SPEC.md`, `docs/spec-*.md`: behaviour of the original and of ours. Tags: [doc X-Y] manual page, [obs] seen on real hardware or in the video, [verify] unconfirmed, [ours] our decision.
- Manual PDFs, text extracts, video and reference frames are local only (gitignored).

## Workflow
- Build + hot reload into the running REAPER: `tools/reaper build-reload`. Tests: `./build/model_tests`.
- Drive REAPER: `tools/reaper ping|dump|action X|lua -`. UI state: ExtState `Rotator`/`cmd` (`tab N`, `track N`, `open`, `gam`, `rec`, `loadsong <file>`).
- Screenshots: `tools/shot <png>` (REAPER must be visible on screen). Mouse/keys: `tools/ui c|rc|dc X Y | drag ... | key K` in ST coordinates. Both need to run outside the sandbox.
- Release: `tools/release <version>` writes `index.xml` and pushes a tag; CI builds macOS, Windows and Linux plugins and attaches them.

## Code
- Minimal, no over-engineering, no comments on unchanged code.
- ReaImGui: PushID takes a string; always EndChild after BeginChild; MenuItem's selected arg is `bool*`.
