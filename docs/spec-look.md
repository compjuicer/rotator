# Look and feel spec (the original's main page and editors)

What the original looks like on screen, written so we can re-skin our ReaImGui UI from it. It covers appearance and interaction feel only, not internals.

Source: the 1993 video manual (version 3.16 on a 4 MB STE with an SM144 mono monitor). Frames are in `docs/look/` (index: [look/README.md](look/README.md)).

Tags:
- **[obs-video]**: seen in the video. That is not our own hardware, so treat it as strong evidence, not [obs].
- **[doc]**: from the manual.
- **[verify]**: believed true, not yet confirmed.
- **[ours]**: our design decision.
- **[unclear]**: VHS blur leaves the detail uncertain.

Pixel figures are in ST high-res pixels (640×400, 1 bit). I estimated them by rectifying `docs/look/main-page/1-42-06-demo-song-full.jpg`, with the 16 px track row as a ruler. The CRT is barrel-distorted, so expect ±4 px. Treat every coordinate as **[verify]** if we ever get a real screenshot or emulator capture.

---

## 1. Visual language (applies everywhere)

### 1.1 Colour
- Pure black and white, no grey [obs-video]. The VHS makes white look pale blue: ignore that.
- "Grey" is faked in three ways:
  1. **Horizontal pinstripes**: alternating 1 px black and 1 px white rows. Used for panel header labels (MUTE, STATUS, CHANNEL, BAR), locator captions, transport button faces, the right end of the title bar, and the RMG title. See `top-bar/0-10-47-closeup-title-boxes-headers.jpg`, `top-bar/0-18-24-stopwatch-inverse-locators.jpg`, `transport/0-19-14-buttons-key-hints.png`.
  2. **50% dither / dotted pattern**: disabled menu items and the greyed group captions in the program's own boxes (`MIDI CLICK:`, `REMOTE:` in `dialogs/0-13-27-midi-thru-box.jpg`). Also dotted vertical separators: between the STATUS column and the track rows (`track-list/1-02-19-hidden-stars-closeup.jpg`) and between the arrange chain columns (`arrange-list/1-49-08-transpose-entry.jpg`). Menu separators are dashed lines.
  3. **Dark textured fill** behind the Hyper Edit lanes, and the vertical stripes of the RMG faders. Probably a dense dither [unclear] (`hyper-edit/2-10-36-lanes-closeup.jpg`, `rmg/2-03-00-faders.jpg`).
- **Black is the page background.** The main page is a black field with white panels on it, separated by black gutters 3–8 px wide. The box row sits on a black band. The gaps between transport buttons are black. This "white tiles on black" look is the single most defining trait [obs-video] (`main-page/1-42-06-demo-song-full.jpg`).

### 1.2 Inverse video
- **Selection** is a full-width black bar with white text. Used for the selected track row, the selected arrange entry, the selected parameter in the track box, the highlighted menu item, the selected file in the file selector, and the selected GAM block (`track-list/0-21-26-selection-bar.jpg`, `track-params/1-02-08-highest-selected.jpg`, `gam/1-51-44-closeup-lane-a.jpg`).
- **Active state**: the transport button for the current state is solid black with white text. When stopped, STOP is black. While playing, START is black and STOP returns to normal [obs-video] (`transport/0-27-27-stopped-stop-inverse.jpg`, `transport/0-26-27-playing-start-inverse.jpg`, `main-page/1-03-10-playing-start-inverse.jpg`).
- **Toggled-on field value**: ARRANGE shows `ON` in a black box when on, and `--` (plain) when off (`top-bar/1-47-27-arrange-on-field.jpg`).
- **Field under edit**: when a split field is clicked (signature numerator/denominator), the clicked half inverts while it changes (`top-bar/0-17-58-signature-denominator-inverse.jpg`). In the event list, the cell under the cursor is inverse (`event-list/2-08-16-control-volume-list.jpg`).
- **Record mode inverts the entire screen.** It is white-on-black everywhere, and the menu bar line shows a help text instead of the menus, e.g. `RECORD: deletes whole old track` (`main-page/0-25-49-record-mode-inverted.png`).
- The stopwatch box and the bar-counter box are sometimes drawn inverse. Which one seems to depend on a click or toggle [unclear] (`top-bar/0-18-24-stopwatch-inverse-locators.jpg` vs `top-bar/0-19-14-bar-counter-inverse.jpg`).

### 1.3 Fonts
Three faces, all bitmap, all monospace:

| Use | Font | Evidence |
|---|---|---|
| Menu bar, menus, title line, list text, box values, arrange entries, alert text, file selector | **ST system font 8×16** (normal weight in menus and alerts) | "Functions" is 9 chars over ~70 px |
| Track numbers and names, Pattern header, port/channel letters, TRACK: n, big button labels (EDIT, DROP, START, STOP), box-row values | **8×16, bold** (GEM bold: smeared 1 px right). It looks condensed and tall. | `track-list/1-02-03-named-tracks-closeup.jpg` |
| Captions under values (FREE, SYNC, TEMPO, BAR / 4 16 768, LEFT LOCATOR, Pattern LENGTH), parameter names in the track box (CHANNEL, QUANTIZE…), channel names in the track list (Korg M1, SR16), MUTE column numbers, small buttons (RMG, DO QUANTIZE, SOLO, HIDE, DEFAULT, MIDI THRU…), key hints | **small system font, ~6 px wide × 6–8 px high**, all caps mostly | "LEFT LOCATOR" is 12 chars over ~70 px |

- The window title uses letter-spaced caps for the program name, `<PROGRAM>   3.16` [obs-video].
- Values never use proportional fonts. Columns line up by fixed character cells.

### 1.4 Boxes, borders, bevels
- **Value box** (box row, locators, arrange bottom fields): a white rectangle with a 1 px black border on the black background. The value is centred in bold 8×16 in the upper part; the caption is centred in the small font along the bottom edge (`top-bar/0-17-28-closeup-sync-tempo.jpg`).
- **Locator boxes** have the caption on a pinstriped strip with the caption text knocked out (white behind the letters) (`top-bar/0-18-24-stopwatch-inverse-locators.jpg`).
- **Panel headers** (MUTE, STATUS, CHANNEL/selected param, BAR): a pinstriped rectangle with the small-font label on a white knock-out patch in its centre (`track-list/1-02-19-hidden-stars-closeup.jpg`). Title texts (`ARRANGE`, `Pattern: 1`, `TRACK: 1`) sit in white boxes in bold 8×16. `TRACK: n` has pinstripes filling the rest of its header box.
- **Buttons are raised** [obs-video]: white (or pinstriped) face, 1 px light outline, and a darker shadow on the right and bottom edges, on the black page. The exact mono pixel pattern is [unclear]. Suggested rendering [ours]: 1 px black outline, 1 px white inner highlight on top/left, 2 px black shadow offset right and bottom. Small buttons (RMG, SOLO, HIDE, DEFAULT, DO QUANTIZE) use the small font; EDIT uses bold 8×16 (`transport/0-19-14-buttons-key-hints.png`, `arrange-list/1-44-45-closeup-single-entry.jpg`).
- **Pressed button**: goes inverse while pressed (file selector OK in `file-selector/0-12-33-ok-pressed.jpg`). For main-page buttons I only see the state result (START/STOP), not the press flash [verify].
- **Dialog default button**: thicker black border, 2–3 px (`dialogs/0-11-30-alert-create-track.jpg` OK, `dialogs/0-32-05-alert-delete-track.jpg`).
- Lines are 1 px everywhere except default-button borders and the GEM alert's outer frame.

### 1.5 GEM window chrome on the main page
The main page is drawn inside a full-screen GEM-style window [obs-video] (`top-bar/0-18-24-stopwatch-inverse-locators.jpg`, `main-page/1-38-43-demo-song-stopped.jpg`):
- A **closer box** at the far left of the title row (small box with a glyph [unclear]).
- A **fuller box** (⊠) at the far right of the title row.
- A **right edge strip** about 18 px wide. It has an up arrow under the fuller, a down arrow near the bottom, and a size box (⊠) in the bottom-right corner. The strip between the arrows is plain [unclear].
- An empty **bottom strip** about 18–20 px high below the panels [unclear: possibly a horizontal scroll bar].

For us [ours]: draw the chrome for the look but make it inert, or map the fuller to "maximize".

### 1.6 Pointer
The standard GEM arrow is used in the editors; the edit pages use a hand/arrow as well [unclear]. Keep the OS pointer [ours].

---

## 2. Main page layout

### 2.1 Diagram (640×400, coordinates approximate)

```
x: 0        100       200       300       400       500       600   640
y0 +-----------------------------------------------------------------------+
   | Desk  File  Functions  Quantize  MIDI  Flags  Options  Edit  Copy      | menu bar 0-18
19 |[c] DEMO.SON          <PROGRAM>  3.16  (c) 1992 … ≡≡|⊠| title 19-36
38 |#[FREE][ARRNG][SyncDEL][ SYNC ][ TEMPO  ][FMT][SIGNAT][ h min sec ms ]▥[BAR /4 16 768]|^| box row 38-72
   |# 436747  --            intern  110.0000 1/16  4/ 4    00 02.832     2  2  1 189 | |
78 |[BAR][ARRANGE a b c d][MUTE][STATUS][ Verse    1 ][CHANNEL][TRACK:16][ 1 1 1 1     ]| | header 78-96
97 |+-----------------------+ +--+ +-++-------------------+ +---------+ [LEFT LOCATOR]| |
   ||  1 Verse     1 ¦ ¦ ¦  | |1 | |█||1Piano 2       A  9| |CHANNEL A 9| [ 5 1 1 1   ]| |
   ||                       | |2 | |█||2KickSnre      A 16| |QUANTIZE 16| [RIGHT LOCATOR]|
   ||  arrange list         | |..| |█||  ...  16 rows x 16px|GROOVE     |  [--  ][--  ]| |
   ||  (or GAM lanes)       | |  | | ||                     |TRANSPOS   |  AUTODROP CYCLE|
   ||                       | |  | | ||                     |VELOCITY   |  [DROP][UNDO]| |
   ||                       | |  | | ||                     |COMPRESS   |  [PUNCH][RECORD]
   ||   ~script~ logo       | |  | | ||                     |LOOP DELAY |              | |
   ||                       | |  | | ||                     |LOWEST     |  [<<][>>][    ]| |
310|+-----------------------+ |  | | ||                     |HIGHEST    |  [<<<][>>>][STOP]|
   |[CATCH][F1 ⇧][F2 ⇩][■■]   |  | | ||                     |GHOST OF   |              | |
334|[ 1  1  1  1 ][TRANSPOSE] |16| | ||16                   |[RMG][EDIT]|              | |
   | UP BEAT-  CUT+           +--+ +-++-------------------+ |DO QUANTIZE|  [ START ][CONT]
358|[  no limit ][ 1  1  1  1 ]     [■■■■■■][■■■■■■][■■■■■]|[SOLO][HIDE]|              |v|
   | Pattern LENGTH Pattern POSITION MIDI THRU OVERFLOW SMPTE [DEFAULT]                 | |
380+-----------------------------------------------------------------------------------+⊠+
   |                              (bottom strip)                                          |
400+---------------------------------------------------------------------------------------+
```

### 2.2 Panel table (x, y, w, h)

| # | Panel | x | y | w | h | Contents |
|---|---|---|---|---|---|---|
| 1 | Menu bar | 0 | 0 | 640 | 19 | GEM menu titles, white bar, black text |
| 2 | Title row | 0 | 19 | 640 | 18 | closer, file/track name, program title, pinstripe filler, fuller |
| 3 | Box row band (black) | 0 | 38 | 620 | 34 | 10 value boxes, each about 26 px high (y 42–68) |
| 4 | Header row | 0 | 78 | 620 | 18 | arrange header, MUTE, STATUS, pattern title, column header, TRACK: n, left-locator value |
| 5 | Arrange panel | 0 | 97 | 183 | 213 | arrange list or GAM, script logo when sparse |
| 6 | Arrange buttons | 0 | 312 | 183 | 20 | CATCH, F1 ⇧, F2 ⇩, black button |
| 7 | Arrange fields | 0 | 334 | 215 | 44 | UP BEAT-/CUT+, TRANSPOSE; Pattern LENGTH, Pattern POSITION |
| 8 | MUTE column | 186 | 97 | 26 | 255 | 16 small track numbers (global numbers 1–64, depending on chain) |
| 9 | STATUS column | 216 | 97 | 24 | 256 | black cell = track has data; `*` = hidden; activity while playing |
| 10 | Track rows | 240 | 97 | 180 | 256 | 16 rows × 16 px: number, name, channel name, right column |
| 11 | Indicator strip | 218 | 352 | 202 | 26 | MIDI THRU, OVERFLOW, SMPTE: black lamp areas over small captions |
| 12 | Track parameter box | 423 | 97 | 85 | 168 | 11 parameter rows |
| 13 | Track buttons | 423 | 268 | 85 | 110 | RMG, EDIT / DO QUANTIZE / SOLO, HIDE / DEFAULT |
| 14 | Right column (black) | 513 | 78 | 107 | 300 | locators, AUTODROP/CYCLE, transport |
| 15 | GEM right strip | 622 | 19 | 18 | 361 | fuller, arrows, size box |
| 16 | Bottom strip | 0 | 380 | 622 | 20 | empty [unclear] |

All coordinates are [verify].

---

## 3. Top area

### 3.1 Menu bar (y 0–18)
- Titles, left to right: `Desk  File  Functions  Quantize  MIDI  Flags  Options  Edit  Copy`, in 8×16 normal weight on white, with a 1 px black line under the bar [obs-video] (`menus/1-29-18-file-menu.jpg`).
- The open title is drawn inverse.
- **The menu-bar line is reused** [obs-video]:
  - In record mode it shows a help line for the item under the mouse, e.g. `RECORD: deletes whole old track` (`main-page/0-25-49-record-mode-inverted.png`).
  - While a file selector is open it shows the operation title, centred and letter-spaced, e.g. `Load S O N G` or `Save S O N G` (`file-selector/1-29-41-load-song-full.jpg`).
  - Elsewhere it shows status texts, e.g. `Automation: PROGRAM and CONTROL 7` (`transport/2-07-38-locators-autodrop-cycle.png`).

### 3.2 Title row (y 19–36)
- Left: a closer box, then the song file name (`DEMO.SON`, `AUTOLOAD.SON`, `TIM1.SON`) in 8×16.
  - In the edit pages this slot shows the current track number and name instead (`2 Piano`, `16 * New *`). A small-font text follows it [unclear] (`event-list/1-39-09-tempo-pseudo-event.jpg`).
- Centre/right: `<PROGRAM>   3.16   (c) 1992 …` [obs-video].
- Right end: a pinstripe filler, then the fuller box.

### 3.3 Box row (y 38–72): every field
Each field is a white box on a black band, with the value in bold 8×16 and the caption in small caps below it (`top-bar/0-10-47-closeup-title-boxes-headers.jpg`, `top-bar/0-17-28-closeup-sync-tempo.jpg`, `top-bar/1-45-57-box-row-closeup.jpg`).

| Field | x≈ | w≈ | Value examples | Caption | Notes |
|---|---|---|---|---|---|
| Free memory | 0 | 48 | `436747` | `FREE` | 6 digits; fills the box |
| Arrange mode | 52 | 43 | `--` / `ON` (inverse) | `ARRANGE` | toggled by A key / click [obs-video] |
| Sync delay | 98 | 47 | blank | `SyncDEL` | mixed-case caption |
| Sync source | 150 | 63 | `intern`, `SMPTE` | `SYNC` | in SMPTE mode a small `LTC` appears next to the value [unclear] (`hyper-edit/2-19-10-tempo-hyper.jpg`) |
| Tempo | 216 | 68 | `120.0000`, `110.0000` | `TEMPO` | always 4 decimals |
| Format | 288 | 30 | `1/16` | `FORMAT` | |
| Signature | 322 | 63 | `4/ 4`, `5/ 4`, `99/ 4`, `10/ 4` | `SIGNATURE` | numerator right-aligned in 2 chars, then `/`, space-padded denominator; click the left or right half to edit that part, which inverts it (`top-bar/0-17-58-signature-denominator-inverse.jpg`) |
| Stopwatch | 395 | 110 | `00 02.832` | `h  min  sec  ms` | in SMPTE mode the caption becomes `h min sec frame` (`transport/2-07-38-locators-autodrop-cycle.png`); sometimes inverse (§1.2) |
| (slider) | 508 | 8 | — | — | narrow vertical pinstriped box between stopwatch and counter; purpose [unclear] |
| Bar counter | 518 | 102 | `2  2  1 189` | `BAR  / 4  16  768` | four figures spread across the box; the second caption reflects the signature denominator (`/ 4`) [verify] |

### 3.4 Header row (y 78–96)
From left to right (`arrange-list/1-44-45-closeup-single-entry.jpg`, `top-bar/0-10-47-closeup-title-boxes-headers.jpg`):
- **BAR** button (x 0–40): small dark/pinstriped button with the small-font label `BAR`. In GAM mode it is solid black with small white text [obs-video] (`gam/1-51-44-closeup-lane-a.jpg`); its exact label there is [unclear].
- **`ARRANGE`** in bold 8×16 in a white box, then **`a b c d`**: four single-letter cells for the chains, separated by thin pinstripe/grey gaps. The selected chain is presumably marked but not legible [unclear].
- **`MUTE`** pinstripe header over the mute column.
- **`STATUS`** pinstripe header over the status column.
- **Pattern title**: `Pattern: 1` when unnamed; when named, the name left and the number right (`Verse      1`, `Intro      2`). Bold 8×16 in a white box. Double-click it to rename [obs-video, transcript 1:45].
- **Right column header**: a pinstripe box with the small-font name of the parameter currently shown in the track list's right column: `CHANNEL`, `QUANTIZE`, `VELOCITY`, `HIGHEST`, … It follows the parameter selected in the track box (`track-list/1-01-50-highest-column.jpg`, `track-list/1-44-52-velocity-column.jpg`).
- **`TRACK: 16`** (bold 8×16) heads the parameter box, with pinstripe filler.
- **Left locator value** box (part of the right column).

---

## 4. Arrange panel

### 4.1 Entry list (x 0–183, y 97–310)
- White panel. Rows are 16 px, text in 8×16 (`arrange-list/1-47-27-closeup-selected-entry.jpg`, `arrange-list/1-49-08-transpose-entry.jpg`).
- Row format: `<bar> <pattern name> <pattern number in its chain column>`.
  - The bar is right-aligned in about 3 chars: ` 1`, ` 5`, `14`.
  - The name is left-aligned, up to 8 chars; unnamed patterns read `Pattern:`.
  - The pattern number sits in column a, b, c or d at the right of the row. The a/b/c/d columns are separated by **dotted vertical lines** running down from each entry (`arrange-list/1-54-49-chain-columns-bcd.jpg`): e.g. `1 Pattern:   4` with 4 in column b, `1 Pattern:     5` in column c, `1 Pattern:       7` in column d.
- The selected/current entry is a full-width inverse bar.
- The list does **not** always start at the top. During playback and after clicks the entries appear several rows down, as if the current entry is kept near a fixed row [unclear] (`arrange-list/1-50-55-arrange-playing-status.jpg`).
- When the list is short, the scrawled **program logo** (italic script with a staff) is drawn in the lower part of the panel (`main-page/0-08-17-empty-song-logo.jpg`). It is not always visible with entries present [unclear].
- Initial song: one entry `1 Pattern: 1` [obs-video].

### 4.2 Arrange buttons (y 312–332) (`arrange-list/1-48-52-bottom-buttons-fields.jpg`)
Four buttons in a row, each about 45 px wide:
1. **CATCH**: pinstriped face, small label.
2. An up arrow ⇧ with a small key hint top-left (reads `F1` [unclear]), with pinstripe fills either side of the arrow.
3. A down arrow ⇩ with hint `F2` [unclear], styled like the up arrow.
4. A **black button** with a small white label that is not legible (`a-b`? `ALL`?) [unclear].

What they do is not shown in the video [verify against manual].

### 4.3 Arrange fields (y 334–378)
Two rows of value boxes:
- Row 1: `1  1  1  1` with captions `UP BEAT-` (left) and `CUT+` (right) in the same box (x 0–108). Next to it, **TRANSPOSE** (x 110–215), whose value appears above the caption when set, e.g. `1`/`-5` (`arrange-list/1-49-00-entry-transpose.jpg`).
- Row 2: **`no limit`** or `8  8  8  8` with caption `Pattern LENGTH` (x 0–105), then `5  1  1  1` with caption `Pattern POSITION` (x 108–215).
- Values are bold 8×16 with the four position figures spread evenly across the box. Captions are mixed case ("Pattern LENGTH").

### 4.4 Chain lanes
Selecting chain b/c/d renumbers the MUTE column to 17–32 / 33–48 / 49–64. The track list itself keeps showing 1–16 [obs-video] (`gam/1-53-40-gam-lane-b-tracks-17-32.png`, `arrange-list/1-54-49-chain-columns-bcd.jpg`).

---

## 5. GAM (graphic arrange), same panel area

Toggle with **K** [obs-video]. Frames: `gam/1-51-44-closeup-lane-a.jpg`, `gam/1-53-47-two-lanes.jpg`, `gam/1-54-42-four-lanes-d-selected.jpg`, `main-page/2-09-01-gam-on-full.jpg`.
- **Time runs downward.**
- **Bar ruler** at the left, about 14 px wide: small-font bar numbers `1, 2, 3 … 17` with a short tick per bar, one bar every ~13 px [unclear: scale probably zoomable].
- **Four lanes** (a b c d), each about 40 px wide, with 2–3 px gaps.
- **Block** = an arrange entry: a white rectangle with a 1 px black border spanning its bars vertically.
  - Pattern name in the small font at top-left, truncated or wrapped (`Intro`, `Patter`/`n:`).
  - Pattern number at top-right in bold 8×16.
  - A selected block is **solid black**, with the label inverse.
- **Song position**: a thin horizontal line across the lanes (`gam/1-51-57-position-line.jpg`).
- Clicking an empty lane with the right button creates an entry and prompts for a pattern number [obs-video] (`gam/1-52-22-create-entry.jpg`).
- The arrange buttons and fields below stay as in §4.2/4.3.

---

## 6. Track list (x 186–420, y 97–378)

### 6.1 Columns
Frames: `track-list/0-10-52-closeup-channel-names.jpg`, `track-list/1-02-03-named-tracks-closeup.jpg`, `track-list/1-44-52-velocity-column.jpg`.

| Column | x≈ | Content / style |
|---|---|---|
| MUTE | 186–212 | white column with a 1 px frame; small-font numbers 1–16 (or 17–64), one per row. The mute state's look is [unclear]: not seen toggled |
| STATUS | 216–240 | **black cell** for tracks containing data, white for empty ones (`main-page/1-38-43-demo-song-stopped.jpg`). A **white `*`** in the cell marks a hidden track (`track-list/1-02-19-hidden-stars-closeup.jpg`). During playback the cells flicker black/white per track like activity lamps [obs-video] (`menus/1-50-55-options-disable-transpose.jpg`). A dotted vertical line separates it from the rows |
| Number + name | 240–335 | bold 8×16. The number is right-aligned in 2 chars, **directly followed by the name with no space**: `1Piano 2`, `10Bass`, `16Vol`. Unnamed tracks show only the number. A newly recorded track gets the name `** OK **` (`track-list/0-32-36-ok-default-name.jpg`) |
| Channel name | 300–370 | **small font**: the name given to the port+channel (`Korg M1`, `SR16`, `Proteus`, `Wave SR2`, `A ExPort`, `UNIT.F`, `original`). Blank if unnamed. Placed right of the track name, left of the value column |
| Right column | 370–418 | depends on the selected parameter. For CHANNEL: port letter + right-aligned channel, both bold 8×16 (`A  9`, `B 13`, `F  2`; `original` shows just `A`[unclear]). For other parameters, the value right-aligned: `768`, `16`, `- 25`, `F#2`, `68`. **Default values are blank** (VELOCITY 0, HIGHEST off), so only changed tracks show a value (`track-list/1-02-03-named-tracks-closeup.jpg`) |

### 6.2 Rows and selection
- 16 rows × 16 px [obs-video].
- Empty tracks still show the shared empty-track channel (and its name) [obs-video] (`track-list/0-10-52-closeup-channel-names.jpg`).
- The selected track is a **full-width inverse bar** from the number to the right edge of the right column. The small-font channel name turns white on black (`track-list/0-21-26-selection-bar.jpg`, `track-list/2-08-32-ok-track-15.jpg`).
- Select with a left click on the number [obs-video]. Double-click the name for the naming box [obs-video].

### 6.3 Indicator strip (y 352–378)
- Three equal cells: **MIDI THRU**, **OVERFLOW**, **SMPTE**. Each is a black lamp area (~12 px) above a small-font caption on a pinstriped/white label strip (`arrange-list/1-44-45-closeup-single-entry.jpg`).
- MIDI THRU lights when MIDI comes in [obs-video, part1 §5]. How a lit lamp looks is [unclear] (`docs/video-frames/0-08-38-midi-in-indicator.png`).

---

## 7. Track parameter box (x 423–508, y 78–378)

Frames: `track-params/0-03-27-params-closeup.png`, `track-params/1-01-18-closeup-params-locators.jpg`, `track-params/1-01-45-lowest-highest-values.jpg`.

- Header: `TRACK: 1` / `TRACK:16`, bold 8×16, in a white box with pinstripe filler to the right.
- 11 rows of about 15 px. The **name is in the small font, left-aligned; the value is bold 8×16, right-aligned**. Default values are blank.

| Row | Default display | Examples seen |
|---|---|---|
| CHANNEL | `A  1` (port letter, then channel right-aligned) | `A  9`, `B 13`, `F  2`, `A 16` |
| QUANTIZE | `768` | `16`, `8` |
| GROOVE | blank | — |
| TRANSPOS | blank | — |
| VELOCITY | blank | `- 25`, `- 20` (shown in the list column) |
| COMPRESS | blank | — |
| LOOP | blank | `4` [obs-video part2] |
| DELAY | blank | — |
| LOWEST | blank | `C#-2` |
| HIGHEST | blank | `F#2`, `68` (a bare number appears too [unclear: 68 = note number?]) |
| GHOST OF | blank | — |

- The parameter name row the user clicks becomes **inverse**, and that parameter becomes the track list's right column (`track-params/1-02-08-highest-selected.jpg`).
- Below, raised buttons (§1.4):
  - Row 1 (y 268–292): `RMG` (small font, ~32 px) and `EDIT` (bold 8×16, ~47 px).
  - Row 2 (y 300–322): `DO QUANTIZE`, full width.
  - Row 3 (y 334–352): `SOLO` and `HIDE`, half width each.
  - Row 4 (y 360–378): `DEFAULT`, full width.

---

## 8. Right column: locators and transport (x 513–620)

Frames: `transport/0-19-14-buttons-key-hints.png`, `track-params/1-01-18-closeup-params-locators.jpg`, `top-bar/0-18-24-stopwatch-inverse-locators.jpg`. The background is black.

| Item | x | y | w | h | Look |
|---|---|---|---|---|---|
| LEFT LOCATOR | 515 | 78 | 105 | 32 | value `1  1  1  1` bold, spread; caption on pinstripe strip |
| RIGHT LOCATOR | 515 | 110 | 105 | 32 | `5  1  1  1` |
| AUTODROP | 515 | 150 | 50 | 22 | value `--` (off), caption small |
| CYCLE | 570 | 150 | 50 | 22 | `--` |
| DROP | 515 | 186 | 50 | 26 | white face, bold label, hint `SPACE` top-left |
| UNDO | 570 | 186 | 50 | 26 | pinstriped face, label faint, probably disabled when nothing to undo [unclear] |
| PUNCH | 515 | 228 | 50 | 28 | white face, hint `/` |
| RECORD | 570 | 228 | 50 | 28 | white face, hint `*` |
| `<<` | 515 | 270 | 30 | 24 | pinstriped, hint `(` |
| `>>` | 548 | 270 | 30 | 24 | pinstriped, hint `)` |
| `<<<` | 515 | 300 | 30 | 30 | pinstriped, hint `[` [unclear, per part4 notes] |
| `>>>` | 548 | 300 | 30 | 30 | pinstriped, hint `]` [unclear] |
| STOP | 582 | 270 | 38 | 60 | tall; hint `Enter` in a white strip at the top; **inverse when stopped** |
| START | 515 | 345 | 63 | 33 | pinstriped, hint `0`; **inverse while playing** |
| CONT | 582 | 345 | 38 | 33 | pinstriped, hint `.` |

- **Key hints** are tiny (small font or smaller) in the top-left corner of the button, on a small white knock-out [obs-video].
- In the RMG page the same transport block appears down the right side (`rmg/2-03-58-rmg-transport-panel.png`).

---

## 9. Menus

Frames: `menus/1-29-18-file-menu.jpg`, `menus/0-13-20-midi-menu.jpg`, `menus/1-24-56-options-menu-full.jpg`, `menus/0-08-01-edit-menu.jpg`.

- **Style**: standard GEM drop-down. The menu opens when the pointer touches the title, with no click [obs-video]. A white box with a 1 px black border; items in 8×16; the highlighted item is a full-width inverse bar.
  - Separators are dashed lines across the box.
  - Section captions are centred between dashes: `--Display Options--`, `--Edit Modes--`, `--SEND TO MIDI:--`.
  - Disabled items are drawn in dithered/light text.
  - A check mark `✓` sits in the left margin; a single-letter shortcut is right-aligned at the far right (`U`, `J`, `K`, `G`, …).
- **File**: `Load S O N G`, `Save S O N G`, `Load System` | `Load  Save ARRANGE` | `Load  Save PATTERN` | `Load  Save SEQUENCE` | `Load  Save MIDI File` | `Delete File`, `Format Disk`, `New Song`, `New Pattern`, `New Arrange`, `Quit`. The Load/Save pairs share one line, so the item is split in two halves.
- **MIDI**: `MIDI Thru`, `Input Handling`, `Output Filter` | `Key Remote`, `✓MIDI Click`, `TRUE Program`, `TRUE Volume`, `--SEND TO MIDI:--`, `Omni On /Poly (1)`, `Omni Off/Poly (3)`, `Maximum Volume`, `Reset Controls` | `Start +1/96`, `Continue +1/96`, `✓Clock Out`, `✓Position Pointer`, `Clock In`.
- **Options**: `SYNCHRONISATION`, `Tempo Interpreter`, `Fittime Calculator`, `SOFT LINK`, `<SYNC BOX> LEARN MODES`(disabled), `Learn SMPTE/Tap`, `Learn SMPTE/MIDI`, `Learn SMPTE/intern` | `Set MIDI Meaning`, `Set Drum Map`, `Set Universal Map` | `Pattern Overview`(disabled on main page), `Notepad`, `Disable Transpose`, `Count-In`.
- **Edit**: `Parameter Mode`, `Global Score Param.`, `Display Parameters`, `Printer   Fonts` (two items on one line), `--Display Options--`, `Full Score`, `✓NOTE Display`, `HYPER Display J`, `MATRIX Display K`[unclear letter], `✓GRAPHIC Display G`, `Global Position`, `Signature Changes`, `Mapped Drum Mode`, `--Edit Modes--`, `✓MIDI Out / ✓Chord`, `MIDI In`, `Insert Mode`, `Bar Cycle`, `MIDI Step Input`. Several items are disabled on the main page.
- **Functions** (score page): `Extract One Line`, `Extract All Lines`, `Lines to Channels` | `Overlap Correction`, `Force Legato` | `Process Note Attributes` (`menus/1-24-17-functions-menu-score.jpg`).
- Desk, Quantize, Flags and Copy menus are not shown open in the video.

---

## 10. Dialogs and alerts

### 10.1 GEM alerts
Frames: `dialogs/0-11-30-alert-create-track.jpg`, `dialogs/0-32-05-alert-delete-track.jpg`, `dialogs/1-16-51-alert-exchange-merge.jpg`.
- White box with a double frame: a 1 px outer line plus an inner border.
- Icon at top-left (~32 px): a `?` in an inverted black triangle, a `STOP` octagon, or a `!` triangle.
- Message in 8×16, left-aligned to the right of the icon, up to ~30 chars per line.
- Buttons in a row at the bottom right, 8×16 labels, 1 px border; the **default has a 2–3 px border**.
- Examples:
  - `Do you want to create a new track?` with `No message | Cancel | [OK]`
  - `DELETE TRACK?` with `Cancel | [OK]`
  - `Exchange or Merge these tracks?` with `CANCEL | EXCHANGE | MERGE`
  - `DELETE all Signature Changes and global signs?` (`dialogs/1-38-11-alert-delete-signatures.jpg`)
  - `DISK FULL Use new disk or delete something` with `CANCEL`
- They appear centred over the area being edited, not always the screen centre [obs-video]. They draw instantly (`dialogs/2-06-04-rmg-delete-movements.png` catches one mid-draw).

### 10.2 The program's own boxes
Frames: `dialogs/0-13-27-midi-thru-box.jpg`, `dialogs/1-50-05-disable-arrange-transpose.png`, `dialogs/2-14-52-smpte-sync-reference.jpg`, `hyper-edit/2-11-08-lane-functions-box.jpg`, `dialogs/1-25-26-midi-meaning-box.jpg`.
- A **black panel**, with white elements on it:
  - Title in bold 8×16 in a pinstriped bar (`MIDI THRU etc`), or a white box with inverse text (`DISABLE ARRANGE TRANSPOSE (CHANNELS)`).
  - An optional small-font subtitle in a white strip (`REMEMBER: PARAMETERS ACCESSED BY ANY EMPTY TRACK`).
  - Parameter rows as white strips: the label right-aligned (`MIDI THRU:`, `Auto-Off Channel:`), then a 1 px divider, then the value box (`ON`, `A 1`, `C5`, `64`, `--`).
  - Group captions (`MIDI CLICK:`, `REMOTE:`) in dithered text directly on the black.
  - The `EXIT` / `OK` button is a white raised button at the bottom.
- Grids (Disable Arrange Transpose): `PORT A`…`PORT F` label cells, then 16 white numbered cells per row. A disabled cell shows a struck/marked number [unclear] (`12` in PORT F).
- Full-screen table dialogs (`dialogs/1-06-32-track-display-params.jpg`): a white table on black, ruled with 1 px lines, ✓ marks in cells, and CANCEL/OK at the bottom right.

### 10.3 File selector (TOS item selector)
Frames: `file-selector/0-12-16-closeup-selected-file.jpg`, `file-selector/1-29-41-load-song-full.jpg`.
- A box titled `FILE SELECTOR`, with the operation name in the menu-bar line (§3.1).
- `Directory:` with the path on an underlined edit line (`C:\<PROGRAM>\*.SON`).
- `Selection:` with the filename on an edit line and a text cursor.
- File list:
  - A header bar with a closer box `⌂` at left and the mask (`*.SON`) on a pinstriped bar.
  - About 9 lines in 8×16. Folders are shown as `♦ NAME` (a diamond/bullet [unclear]); files as `NAME    .EXT` with the extension column aligned.
  - The selected file is inverse.
  - Empty slots are dotted/underscored lines.
- To the right of the list: a vertical scroll bar with ⇧/⇩ arrow boxes and a dithered slider.
- `DRIVE:` grid of 2 × 8 letter buttons `A`–`P`; the current drive is inverse.
- `OK` (default, thick border) and `Cancel` at the bottom right. Pressed buttons invert.

---

## 11. Edit page: event list

Frames: `event-list/1-04-22-notes-with-position-graphic.jpg`, `event-list/1-39-09-tempo-pseudo-event.jpg`, `event-list/2-08-16-control-volume-list.jpg`, `event-list/2-09-50-timpani-notes.jpg`, `event-list/2-08-08-left-buttons-matrix-snap.jpg`.

The edit page (E) keeps the menu bar, title row and box row. Below them, the event list sits above the score (or above Hyper Edit). A draggable divider sets the split.

- **Left button column** (~40 px): boxed toggle buttons, stacked, in bold 8×16 condensed:
  - Event types: `*ON|OFF`, `PROGRAM`, `CONTROL`, `PITCH W`, `p|cPRES`, `*SysEx`, `PSEUDO`, `COMMENT`.
  - Then small buttons: `QUANTIZE`, `DEQUANT`, `DELETE`.
  - Then `MATRIX`, `SNAP`, `VELOC`.
  - Event types work as a palette: drag one into the list to insert [obs-video part3].
- **Header ruler**, small font, across the top: `1 2 3 4 5 6 BAR /4 16 768 STATUS CHANNEL -1- -2- Length/Info`.
- **Position graphic**: a ~70 px strip of 6 columns with dotted vertical lines, labelled 1–6. Each event row puts a small black block in the column/offset matching its position, so a run of events makes a **staircase** [obs-video; exact meaning unclear]. It looks like a mini time graphic of the visible events.
- **Event rows**: 8×16 bold. The columns are:
  - Position `1  1  1  1`, four right-aligned fields.
  - Status: `NOTE`, `Control 7`, `P.USER`, `Program`.
  - Channel number.
  - `-1-`: pitch name `C2`/`D2`, or the controller number.
  - `-2-`: velocity or value.
  - Length/Info: `1  0  36`, or a text info such as `Volume` or `abs. Tempo`.
- The current event's position fields are inverse (`1  1  1  1` as a black bar). The field under the mouse is inverse too.
- **Right column**: a `CATCH` button at top, two scroll-arrow buttons (⇧ ⇩, inverse squares), then stacked mode buttons `INSMODE`, `CHORD`, `MIDIOUT`, `MIDI IN`, `CYCLE`, `STEP` [partly unclear].
- **Strip form**: a two-line version sits above the score in normal score editing (`event-list/1-05-08-strip-above-score.jpg`). The `insert event` variant shows a text prompt in an inverse cell (`event-list/1-27-14-insert-event-strip.jpg`).

## 12. Edit page: Hyper Edit

Frames: `hyper-edit/2-10-00-hyper-on.jpg`, `hyper-edit/2-10-36-lanes-closeup.jpg`, `hyper-edit/2-11-32-drawn-velocity-beams.jpg`, `hyper-edit/2-11-08-lane-functions-box.jpg`.
- Toggle with **J**. It replaces the score area below the event-list header.
- **Toolbar row** (small font, boxed cells): `SET 1`, `Zoom+ 3`, `Zoom- 9`, `PEN 12`, `COLOR 3`, `AutoDel`, `✓Fixed`, `REDraw`, `✓QuickScore` [partly unclear].
- **Lane header columns**, small font: `NAME`, `QUA`, `DLY`, `HMT`[unclear], `STATUS`.
  - Each lane is ~40 px tall.
  - The name (`BD`, `RIMSHOT`, `SNARE`, `CLAP`, `CL.HIHAT`) is in a white box; right-click it to define the lane.
  - The QUA value (`16`, `64`) is bold.
  - STATUS shows `NOTE` in bold.
- **Grid area**: a dark (black/dense dither) background. Lighter vertical lines mark the QUA steps; a ruler with bar numbers runs above.
- **Events** are white vertical beams from the lane bottom; the height is the velocity. A drawn roll looks like a white comb with an undulating top edge.
- The bottom strip shows a one-line score preview [obs-video].
- **Lane functions box**: a black panel of white strip buttons:
  - `Delete all duplicated "HiHAT"-Events`
  - `Do Groove of current instrument`
  - `Convert all instruments to Set [1]`
  - `Convert Events of current instr. to`, with the fields `status NOTE`, `CHN 1`, `-1- C2`
  - a name field and an `EXIT` button.

## 13. Matrix editor
Not in the video. The manual figure `docs/manual-screens/matrix-16-1.png` [doc] shows:
- Time runs **downward**, with numbered bar lines (`1`, `2`, `3`) as solid horizontal rules.
- Rows of dashed/dotted horizontal lines mark steps.
- Notes are **vertical black bars** at their pitch column.
- A piano keyboard runs horizontally along the bottom, with C3 labelled. Clicking a key shows a white square on it.
- A left panel has boxed buttons: `MATRIX`, `SNAP`, `VELOC`, a `1/ 96` field with `ZOOM` caption, and a `C1` field with `LOWEST` caption.

In the event list, the left buttons `MATRIX`/`SNAP`/`VELOC` are the same controls (`event-list/2-08-08-left-buttons-matrix-snap.jpg`).

## 14. RMG (for completeness; out of scope)
Frames: `rmg/2-03-00-faders.jpg`, `rmg/2-07-20-snapshots.jpg`, `rmg/2-03-58-rmg-transport-panel.png`.
- Title `Realtime MIDI Generator` in bold on a pinstriped bar.
- Field rows: `STATUS Control`, `NUMBER 7 Volume`, `LISTEN TO MIDI`, `PORT A`, `CHANNELNAME Piano`, `DELETE MOVEMENTS`, `COPY TO ALL`.
- 16 tall faders: black slots with dotted edges. The white part is the level, and the value is shown underneath (`127`, `88`).
- Channel numbers 1–16 sit in boxes below the faders.
- A snapshot strip of small cells, captioned `SNAPSHOTS  CLICK LEFT:RECALL  RIGHT:SAVE`; `EXIT` at the bottom right. The page background is black.

---

## 15. Interaction feel
- **Number fields**: left click increases, right click decreases [obs-video] (tempo 120 to 110). Holding repeats [verify]. Dragging a value up/down also changes it [verify; manual].
- **Split fields** (signature, positions): the half or figure under the pointer is the one edited, and it inverts while changing [obs-video].
- Right click on some fields **blanks** them (Auto-Off Channel) [obs-video].
- **Double-click** a name (track, pattern) to open an 8-char naming box; Return confirms, Esc clears [obs-video].
- **Menus** drop on hover, and a click selects [obs-video].
- **Alerts**: Return activates the default (thick-border) button [obs-video].
- **Drag** a track onto another track: copy into an empty slot, or the Exchange/Merge alert for an occupied one [obs-video].
- Feedback is immediate and flicker-free; there is no animation anywhere. Values change in place.

---

## 16. Rendering notes for our ReaImGui skin [ours]
1. Lay the main page out on a **fixed 640×400 logical canvas**, scaled by an integer factor (2× default) and centred. Use nearest-neighbour scaling so 1 px lines stay crisp.
2. Colours: two tokens, `ink` (black) and `paper` (white), plus an optional "phosphor" tint for paper. Implement pinstripe as a horizontal line on every other logical row. Implement dither as a 1-on/1-off checker.
3. Fonts: ship bitmap-style fonts.
   - An 8×16 regular and a bold (draw twice, offset +1 px x).
   - A ~6×6 caps small font for captions.
   - ImGui can load a pixel TTF at exact multiples; pick a public-domain 8×16 VGA-style font and a 5×7/6×6 pixel font. We cannot copy the Atari ROM font.
4. Primitives:
   - `ValueBox(caption, value)`
   - `StripeHeader(label)`
   - `RaisedButton(label, hint, style={white|stripe|black}, active)`
   - `InverseRow`
   - `DottedVLine`
   - `GemAlert(icon, text, buttons, default)`
   - `BlackPanelDialog`
5. Map the current UI: arrange list/GAM to panel 5, the pattern track table to panels 8–11, the track params to panel 12–13, transport to panel 14. Events/Hyper/Matrix replace panels 5–14 as a full "edit page" below the box row, rather than as tabs.
