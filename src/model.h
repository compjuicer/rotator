#pragma once
// Song model and render. No Reaper dependencies. Times are quarter notes (QN), relative to pattern start.
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace rotator {

enum class EventType { Note, CC, Program, PitchBend, ChanPressure, PolyPressure };

// data1 = what (pitch / controller / program), data2 = value:
//   Note:         data1 pitch, data2 velocity, len used
//   CC:           data1 controller, data2 value
//   Program:      data1 program, data2 unused
//   PitchBend:    data2 bend, signed -8192..8191 (0 = centre)
//   ChanPressure: data2 pressure
//   PolyPressure: data1 pitch, data2 pressure
// len is 0 for everything but notes.
struct Event {
  EventType type = EventType::Note;
  double pos = 0;
  int chan = 1;                  // 1..16
  int data1 = 60, data2 = 100;
  double len = 0;
};

// Data1 is part of an event's identity (pitch / controller) for these types.
inline bool has_data1(EventType t) { return t == EventType::Note || t == EventType::CC || t == EventType::PolyPressure; }

// Hyper Edit lane: which events it shows, plus entry settings and a playback offset.
struct HyperLane {
  std::string name;
  EventType type = EventType::Note;
  std::optional<int> data1;      // nullopt = ALL; ignored for types without data1
  std::optional<int> chan;       // nullopt = any channel
  double grid = 0.25;            // QN, entry grid
  double len = 0.0625;           // QN, length of entered notes
  double delay = 0;              // QN, non-destructive offset for tracks using the set
  bool hhat = false;             // adjacent hhat lanes form a mutually exclusive group
};

struct HyperSet {
  std::string name;
  std::vector<HyperLane> lanes;
};

bool lane_matches(const HyperLane&, const Event&);

struct Track {
  std::string name;
  std::vector<Event> events;
  std::string dest;              // Reaper track GUID; empty = silent
  std::optional<int> chan;       // 1..16 overrides event channels; nullopt = original
  int transpose = 0, velocity = 0;
  std::optional<double> loop;    // beats
  double delay = 0;              // QN
  int lowest = 0, highest = 127;
  int quantize = 0;              // grid as a note-value divisor (16 = 1/16); 0 = off. Note starts only, playback only
  int groove = 0;                // 0 off, 1-12 16A..8F swing, 13-18 8-12 1216 five sept nine Free, 19-34 user Us1-Us16
  int compress = 0;              // velocity around 64: 1 = 1:2, 2 = 1:4, 3 = 1:8, 4 = fix, -1 = expand 1:2, -2 = 1:4
  int ghost_pattern = 0, ghost_track = 0;  // play that track's events through these parameters; 0 = no ghost
  int filter = 0;                // output filter: bit (1 << EventType) blocks that type
  bool chan_filter = false;      // only pass events whose own channel equals chan
  bool mute = false, hide = false;
  int hyper_set = -1;            // index into Song::hyper_sets whose lane delays apply; -1 = none
};

struct Pattern {
  std::string name;
  std::map<int, Track> tracks;   // track number -> track
};

struct Entry {
  int bar = 1, pattern = 1;
  std::string name;
  int transpose = 0;
  int shift = 0;                 // changeover offset in 1/192 QN ticks: < 0 upbeat (starts early), > 0 cut (starts late)
  std::map<int, bool> mutes;     // track number -> muted
};

constexpr std::array<char, 4> kChains = {'a', 'b', 'c', 'd'};

enum class QuantMode { Musical, NoteOnOff, NoteOn, All };  // Musical: start snapped, length kept

struct UserGroove {
  int pattern = 0, track = 0;    // template track; 0 = unset
  int range = 0;                 // ticks (1/192 QN) either side of a grid point; 0 = any distance
  int strength = 100;            // % of the distance moved
};

struct Song {
  std::map<int, Pattern> patterns;
  std::map<char, std::vector<Entry>> chains;
  std::vector<HyperSet> hyper_sets;
  std::set<std::pair<std::string, int>> no_arrange_transpose;  // (dest GUID, output channel 1..16)
  char solo_chain = 0;           // with solo_track: only that track number plays in that chain; 0 = no solo
  int solo_track = 0;
  QuantMode quant_mode = QuantMode::Musical;
  int quant_range = 0, quant_strength = 100;  // capture: ticks either side of a grid step (0 = all), % moved
  std::array<UserGroove, 16> grooves;
  std::array<std::map<int, bool>, 10> mute_groups;
  std::string notes;             // Notepad text
};

struct Item {
  std::string dest;
  char chain = 'a';
  Entry entry;
  double start = 0, finish = 0;  // absolute QN
  std::vector<Event> events;    // pos is absolute QN
};

double pattern_length(const Pattern&);

// Lowest unused number >= 1.
int free_pattern(const Song&);
int free_track(const Pattern&);

int copy_track(Pattern&, int src);                 // to the lowest free number, returned
void exchange_tracks(Pattern&, int a, int b);
void merge_tracks(Pattern&, int src, int dst);     // src events added to dst; src stays

double quantized(double pos, int quantize);        // pos (QN from pattern start) snapped to the 1/quantize grid
double grooved(double pos, int groove);            // pos snapped to the swing grid of a groove preset
int compressed(int velocity, int compress);
constexpr int kFirstUserGroove = 19, kLastGroove = 34;
std::string groove_name(int groove);               // "16A" .. "Free", "Us1" .. "Us16", "" when off
Event placed(const Song&, const Track&, Event);    // quantize then groove, per the song's quantize mode
void do_quantize(const Song&, Track&);             // bake quantize and groove into the events, then switch them off
bool dynamic_groove(const Song&, Track&, int user_groove, int amount);  // template velocities, amount 0-15
bool filtered(const Track&, const Event&);         // blocked by the output or channel filter
void normalize(Track&);                            // bake transpose, velocity, compress, delay, range; reset them
const Track* ghost_source(const Song&, const Track&);  // the track a ghost plays, or nullptr

// Entry helpers. what: 1 transpose, 2 upbeat/cut, 4 mutes. To the next entry in start order, or every later one.
void copy_entry_params(Song&, char chain, int index, int what, bool all_following);
void move_keep_length(Song&, char chain, int index, int bar);  // later entries in the chain move along

// Arrange edits. Entries are addressed by chain and vector index; "next"/"previous" follow start-bar order.
std::optional<int> entry_length(const Song&, char chain, int index);  // bars; nullopt = last entry (no limit)
int add_entry(Song&, char chain, const Entry&);                      // -1 if an entry already starts at that bar
void move_start(Song&, char chain, int index, int bar);               // end stays put; clamped between neighbours
int move_to_chain(Song&, char from, int index, char to);              // returns new index
bool set_length(Song&, char chain, int index, int bars);              // later entries in the chain shift
void delete_close_gap(Song&, char chain, int index);                  // later entries pull earlier

// bar_to_qn(bar) -> QN of that bar's downbeat. Items are ordered by chain, then entry start, then dest.
std::vector<Item> render(const Song&, const std::function<double(int)>& bar_to_qn);

}  // namespace rotator
