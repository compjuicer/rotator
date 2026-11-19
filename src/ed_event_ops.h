#pragma once
// Event editor logic (manual ch 9), no Reaper/ImGui dependencies.
//
// Positions are shown as bar beat unit pulse, relative to the pattern start, all 1-based ("1 1 1 1" = pattern
// start). Lengths use the same fields 0-based, bar field omitted when zero. Pulses are kPPQ per quarter note
// (REAPER's default MIDI resolution). The beat is the time signature denominator; the unit is the display FORMAT
// (format 16 = 1/16 note), never longer than a beat. The caller supplies the time signature (project signature at
// the pattern's first placement, else 4/4). Event lists are kept in send order: position, then lowest pitch first
// among notes on the same pulse; other ties keep their order.
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "model.h"

namespace rotator {

constexpr int kPPQ = 192;  // 768 per 4/4 bar, like the original

struct Grid {
  int num = 4, denom = 4, format = 16;
  int beat_p() const { return 4 * kPPQ / denom; }
  int bar_p() const { return num * beat_p(); }
  int unit_p() const;
  int units_per_beat() const { return beat_p() / unit_p(); }
};

struct Bbup {
  int bar = 1, beat = 1, unit = 1, pulse = 1;
};

int to_pulses(double qn);
double from_pulses(int p);

Bbup to_bbup(double qn, const Grid&);         // 1-based position fields
double from_bbup(const Bbup&, const Grid&);   // linear, so out-of-range fields carry
std::string format_pos(double qn, const Grid&, int bar_offset = 0);
std::string format_len(double qn, const Grid&);
std::optional<double> parse_pos(const std::string&, const Grid&);  // "bar [beat [unit [pulse]]]", missing = 1
std::optional<double> parse_len(const std::string&, const Grid&);  // right-aligned "... beat unit pulse"; one number = pulses

std::string note_name(int pitch);             // C3 = 60
std::optional<int> parse_note(const std::string&);  // "E4", "F#2", "Bb-1" or a note number
const char* status_name(EventType);
const char* cc_name(int controller);          // "" if none

// Editable columns of an event line.
enum class Col { Bar, Beat, Unit, Pulse, Status, Chan, D1, D2, Len, Count };

// Chan/D1/D2/Len as shown: PitchWh -1- is the LSB and -2- the MSB (64 = centre), C-Press amount is -1-, Len is pulses.
bool has_field(EventType, Col);
std::pair<int, int> field_range(EventType, Col);
int get_field(const Event&, Col);
void set_field(Event&, Col, int);             // clamped; Len unclamped against neighbours (see set_length)

double step_pos(double pos, Col, int delta, const Grid&);  // move one of the four position fields, floored at 0

void set_status(Event&, EventType);           // between non-note types only
bool before(const Event&, const Event&);      // send order
int sort_events(std::vector<Event>&, int follow);  // returns follow's new index

// Sets the position (snapped to pulses, >= 0). Insert mode moves every later event in the list by the same amount.
// Doesn't re-sort, so it can run every frame of a drag.
void move_event(std::vector<Event>&, int idx, double pos, bool insert_mode);

// A note can't extend past the next note of the same pitch and channel. Min one pulse.
double max_length(const std::vector<Event>&, int idx);
void set_length(std::vector<Event>&, int idx, double len);
void set_lengths(std::vector<Event>&, int from, int to, double len);  // all notes in [from, to]

Event new_event(EventType, double pos, int chan);
int insert_event(std::vector<Event>&, const Event&);  // sorted insert, returns its index

// T / Shift-T: copy the value in column c to the next (all = every following) event of the same status.
void transform(std::vector<Event>&, int idx, Col c, bool all);

// Shift-Delete: delete every event of the cursor event's status (and pitch / controller for types with data1).
// keep (Alt-Shift-Delete): delete the rest of that status instead. Returns the new cursor index.
int delete_matching(std::vector<Event>&, int idx, bool keep);

// MIDI IN: a played message sets values of a matching event. Cursor on -1- sets only data1, on -2- only data2.
bool midi_in(Event&, Col cursor, int status, int d1, int d2);

// Step input: notes go in at pos; once every held key is released, pos advances one step.
struct StepInput {
  double pos = 0;
  int held = 0;
  bool wrote = false;
};
int step_note_on(StepInput&, std::vector<Event>&, int chan, int pitch, int vel, double len);
void step_note_off(StepInput&, double step);

// Where a pattern plays in the song (absolute QN), in start order. Same end rule as render().
struct Placement {
  int bar = 1;
  double start = 0, end = 0;
};
std::vector<Placement> placements(const Song&, int pattern, const std::function<double(int)>& bar_to_qn);
const Placement* placement_at(const std::vector<Placement>&, double qn);

// Last event at or before pos (0 if none), only among indices where shown(i).
int event_at(const std::vector<Event>&, double pos, const std::function<bool(int)>& shown);

}  // namespace rotator
