#include "ed_event_ops.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numeric>
#include <sstream>

namespace rotator {

int Grid::unit_p() const { return std::max(1, std::min(beat_p(), 4 * kPPQ / std::max(1, format))); }

int to_pulses(double qn) { return int(std::lround(qn * kPPQ)); }
double from_pulses(int p) { return double(p) / kPPQ; }

static Bbup split(int p, const Grid& g) {
  Bbup b;
  b.bar = p / g.bar_p();
  p %= g.bar_p();
  b.beat = p / g.beat_p();
  p %= g.beat_p();
  b.unit = std::min(p / g.unit_p(), g.units_per_beat() - 1);
  b.pulse = p - b.unit * g.unit_p();
  return b;
}

Bbup to_bbup(double qn, const Grid& g) {
  Bbup b = split(std::max(0, to_pulses(qn)), g);
  return {b.bar + 1, b.beat + 1, b.unit + 1, b.pulse + 1};
}

double from_bbup(const Bbup& b, const Grid& g) {
  return from_pulses((b.bar - 1) * g.bar_p() + (b.beat - 1) * g.beat_p() + (b.unit - 1) * g.unit_p() + b.pulse - 1);
}

std::string format_pos(double qn, const Grid& g, int bar_offset) {
  Bbup b = to_bbup(qn, g);
  return std::to_string(b.bar + bar_offset) + " " + std::to_string(b.beat) + " " + std::to_string(b.unit) + " " +
         std::to_string(b.pulse);
}

std::string format_len(double qn, const Grid& g) {
  Bbup b = split(std::max(0, to_pulses(qn)), g);
  std::string s = std::to_string(b.beat) + " " + std::to_string(b.unit) + " " + std::to_string(b.pulse);
  return b.bar ? std::to_string(b.bar) + " " + s : s;
}

static std::optional<std::vector<int>> ints(const std::string& s) {
  std::istringstream in(s);
  std::vector<int> v;
  int x;
  while (in >> x) v.push_back(x);
  if (!in.eof() || v.empty() || v.size() > 4) return std::nullopt;
  return v;
}

std::optional<double> parse_pos(const std::string& s, const Grid& g) {
  auto v = ints(s);
  if (!v) return std::nullopt;
  v->resize(4, 1);
  int p = to_pulses(from_bbup({(*v)[0], (*v)[1], (*v)[2], (*v)[3]}, g));
  if (p < 0) return std::nullopt;
  return from_pulses(p);
}

std::optional<double> parse_len(const std::string& s, const Grid& g) {
  auto v = ints(s);
  if (!v) return std::nullopt;
  const int unit[4] = {1, g.unit_p(), g.beat_p(), g.bar_p()};
  int p = 0;
  for (size_t k = 0; k < v->size(); ++k) p += (*v)[v->size() - 1 - k] * unit[k];
  if (p < 1) return std::nullopt;
  return from_pulses(p);
}

static const char* const kNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

std::string note_name(int pitch) {
  int oct = (pitch >= 0 ? pitch / 12 : (pitch - 11) / 12);
  return kNames[pitch - oct * 12] + std::to_string(oct - 2);
}

std::optional<int> parse_note(const std::string& in) {
  std::string s;
  for (char c : in)
    if (!std::isspace((unsigned char)c)) s += c;
  if (s.empty()) return std::nullopt;
  int pitch;
  size_t i = 0;
  if (std::isdigit((unsigned char)s[0])) {
    pitch = 0;
  } else {
    static const int base[7] = {9, 11, 0, 2, 4, 5, 7};  // A..G
    char c = char(std::toupper((unsigned char)s[0]));
    if (c < 'A' || c > 'G') return std::nullopt;
    pitch = base[c - 'A'];
    i = 1;
    if (i < s.size() && s[i] == '#') pitch++, i++;
    else if (i < s.size() && s[i] == 'b') pitch--, i++;
  }
  size_t used = 0;
  int n;
  try {
    n = std::stoi(s.substr(i), &used);
  } catch (...) {
    return std::nullopt;
  }
  if (i + used != s.size()) return std::nullopt;
  pitch = i ? pitch + (n + 2) * 12 : n;
  if (pitch < 0 || pitch > 127) return std::nullopt;
  return pitch;
}

const char* status_name(EventType t) {
  switch (t) {
    case EventType::Note: return "Note";
    case EventType::CC: return "Control";
    case EventType::Program: return "Program";
    case EventType::PitchBend: return "PitchWh";
    case EventType::ChanPressure: return "C-Press";
    case EventType::PolyPressure: return "P-Press";
  }
  return "";
}

const char* cc_name(int c) {
  switch (c) {
    case 0: return "Bank MSB";
    case 1: return "Modulation";
    case 2: return "Breath";
    case 4: return "Foot";
    case 5: return "Porta time";
    case 6: return "Data entry";
    case 7: return "Volume";
    case 8: return "Balance";
    case 10: return "Pan";
    case 11: return "Expression";
    case 32: return "Bank LSB";
    case 64: return "Sustain";
    case 65: return "Portamento";
    case 66: return "Sostenuto";
    case 67: return "Soft pedal";
    case 91: return "Reverb";
    case 93: return "Chorus";
    case 120: return "Sound off";
    case 121: return "Reset ctrl";
    case 123: return "Notes off";
  }
  return "";
}

bool has_field(EventType t, Col c) {
  switch (c) {
    case Col::Chan: return true;
    case Col::D1: return true;
    case Col::D2: return t != EventType::Program && t != EventType::ChanPressure;
    case Col::Len: return t == EventType::Note;
    default: return false;
  }
}

std::pair<int, int> field_range(EventType t, Col c) {
  if (c == Col::Chan) return {1, 16};
  if (c == Col::Len) return {1, 1 << 24};
  if (c == Col::D2 && t == EventType::Note) return {1, 127};
  return {0, 127};
}

static int bend(const Event& e) { return std::clamp(e.data2 + 8192, 0, 16383); }

int get_field(const Event& e, Col c) {
  switch (c) {
    case Col::Chan: return e.chan;
    case Col::Len: return to_pulses(e.len);
    case Col::D1:
      if (e.type == EventType::PitchBend) return bend(e) & 127;
      if (e.type == EventType::ChanPressure) return e.data2;
      return e.data1;
    case Col::D2:
      if (e.type == EventType::PitchBend) return bend(e) >> 7;
      return e.data2;
    default: return 0;
  }
}

void set_field(Event& e, Col c, int v) {
  if (!has_field(e.type, c)) return;
  auto [lo, hi] = field_range(e.type, c);
  v = std::clamp(v, lo, hi);
  switch (c) {
    case Col::Chan: e.chan = v; break;
    case Col::Len: e.len = from_pulses(v); break;
    case Col::D1:
      if (e.type == EventType::PitchBend) e.data2 = (bend(e) & ~127) + v - 8192;
      else if (e.type == EventType::ChanPressure) e.data2 = v;
      else e.data1 = v;
      break;
    case Col::D2:
      if (e.type == EventType::PitchBend) e.data2 = (bend(e) & 127) + (v << 7) - 8192;
      else e.data2 = v;
      break;
    default: break;
  }
}

double step_pos(double pos, Col c, int delta, const Grid& g) {
  Bbup b = to_bbup(pos, g);
  switch (c) {
    case Col::Bar: b.bar += delta; break;
    case Col::Beat: b.beat += delta; break;
    case Col::Unit: b.unit += delta; break;
    case Col::Pulse: b.pulse += delta; break;
    default: break;
  }
  return std::max(0.0, from_bbup(b, g));
}

void set_status(Event& e, EventType t) {
  if (e.type == EventType::Note || t == EventType::Note || e.type == t) return;
  if (e.type == EventType::PitchBend) {
    e.data1 = 0;
    e.data2 = bend(e) >> 7;
  }
  if (t == EventType::PitchBend) {
    e.data1 = 0;
    e.data2 = 0;
  }
  e.data1 = std::clamp(e.data1, 0, 127);
  if (t != EventType::PitchBend) e.data2 = std::clamp(e.data2, 0, 127);
  e.type = t;
  e.len = 0;
}

bool before(const Event& a, const Event& b) {
  int pa = to_pulses(a.pos), pb = to_pulses(b.pos);
  if (pa != pb) return pa < pb;
  return a.type == EventType::Note && b.type == EventType::Note && a.data1 < b.data1;
}

int sort_events(std::vector<Event>& v, int follow) {
  std::vector<int> order(v.size());
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return before(v[a], v[b]); });
  std::vector<Event> out;
  out.reserve(v.size());
  int found = follow;
  for (int k = 0; k < int(order.size()); ++k) {
    if (order[k] == follow) found = k;
    out.push_back(v[order[k]]);
  }
  v = std::move(out);
  return found;
}

void move_event(std::vector<Event>& v, int idx, double pos, bool insert_mode) {
  pos = from_pulses(std::max(0, to_pulses(pos)));
  double delta = pos - v[idx].pos;
  v[idx].pos = pos;
  if (insert_mode)
    for (size_t j = idx + 1; j < v.size(); ++j) v[j].pos = std::max(0.0, v[j].pos + delta);
}

double max_length(const std::vector<Event>& v, int idx) {
  const Event& e = v[idx];
  double lim = 1e9;
  for (const Event& o : v)
    if (o.type == EventType::Note && o.data1 == e.data1 && o.chan == e.chan && o.pos > e.pos + 1e-9)
      lim = std::min(lim, o.pos - e.pos);
  return lim;
}

void set_length(std::vector<Event>& v, int idx, double len) {
  if (v[idx].type != EventType::Note) return;
  int p = std::max(1, to_pulses(std::min(len, max_length(v, idx))));
  v[idx].len = from_pulses(p);
}

void set_lengths(std::vector<Event>& v, int from, int to, double len) {
  if (from > to) std::swap(from, to);
  for (int i = std::max(0, from); i <= to && i < int(v.size()); ++i) set_length(v, i, len);
}

Event new_event(EventType t, double pos, int chan) {
  Event e{t, pos, chan, 0, 0, 0};
  switch (t) {
    case EventType::Note: e.data1 = 60, e.data2 = 100, e.len = from_pulses(kPPQ / 4); break;
    case EventType::CC: e.data1 = 1; break;
    case EventType::PolyPressure: e.data1 = 60; break;
    default: break;
  }
  return e;
}

int insert_event(std::vector<Event>& v, const Event& e) {
  v.push_back(e);
  return sort_events(v, int(v.size()) - 1);
}

void transform(std::vector<Event>& v, int idx, Col c, bool all) {
  const Event src = v[idx];
  int val = get_field(src, c);
  for (int j = idx + 1; j < int(v.size()); ++j) {
    if (v[j].type != src.type) continue;
    if (c == Col::Len) set_length(v, j, src.len);
    else set_field(v[j], c, val);
    if (!all) break;
  }
}

int delete_matching(std::vector<Event>& v, int idx, bool keep) {
  const Event cur = v[idx];
  bool d1 = has_data1(cur.type);
  if (keep && !d1) return idx;
  std::vector<Event> out;
  int cursor = 0;
  for (int i = 0; i < int(v.size()); ++i) {
    const Event& e = v[i];
    bool match = e.type == cur.type && (!d1 || e.data1 == cur.data1);
    bool del = e.type == cur.type && (keep ? !match : match);
    if (i == idx) cursor = int(out.size());
    if (!del) out.push_back(e);
  }
  v = std::move(out);
  return std::min(cursor, std::max(0, int(v.size()) - 1));
}

bool midi_in(Event& e, Col cursor, int status, int d1, int d2) {
  int kind = status & 0xF0;
  bool one = cursor == Col::D1, two = cursor == Col::D2;
  if ((e.type == EventType::Note && kind == 0x90 && d2 > 0) || (e.type == EventType::CC && kind == 0xB0)) {
    if (!two) e.data1 = d1;
    if (!one) e.data2 = d2;
    return true;
  }
  if (e.type == EventType::ChanPressure && kind == 0xD0) {
    e.data2 = d1;
    return true;
  }
  return false;
}

int step_note_on(StepInput& s, std::vector<Event>& v, int chan, int pitch, int vel, double len) {
  Event e{EventType::Note, s.pos, chan, pitch, vel, 0};
  int i = insert_event(v, e);
  set_length(v, i, len);
  s.held++;
  s.wrote = true;
  return i;
}

void step_note_off(StepInput& s, double step) {
  s.held = std::max(0, s.held - 1);
  if (s.held || !s.wrote) return;
  s.pos += step;
  s.wrote = false;
}

std::vector<Placement> placements(const Song& song, int pattern, const std::function<double(int)>& bar_to_qn) {
  std::vector<Placement> out;
  auto pit = song.patterns.find(pattern);
  if (pit == song.patterns.end()) return out;
  for (const auto& [_, chain] : song.chains) {
    auto entries = chain;
    std::stable_sort(entries.begin(), entries.end(), [](const Entry& x, const Entry& y) { return x.bar < y.bar; });
    for (size_t k = 0; k < entries.size(); ++k) {
      if (entries[k].pattern != pattern) continue;
      double origin = bar_to_qn(entries[k].bar);
      double start = origin + entries[k].shift / 192.0;
      double end = k + 1 < entries.size() ? bar_to_qn(entries[k + 1].bar) + entries[k + 1].shift / 192.0
                                          : origin + pattern_length(pit->second);
      if (end > start) out.push_back({entries[k].bar, origin, end});
    }
  }
  std::stable_sort(out.begin(), out.end(), [](const Placement& a, const Placement& b) { return a.start < b.start; });
  return out;
}

const Placement* placement_at(const std::vector<Placement>& v, double qn) {
  for (const auto& p : v)
    if (qn >= p.start && qn < p.end) return &p;
  return nullptr;
}

int event_at(const std::vector<Event>& v, double pos, const std::function<bool(int)>& shown) {
  int best = 0;
  bool any = false;
  for (int i = 0; i < int(v.size()); ++i) {
    if (!shown(i)) continue;
    if (!any || v[i].pos <= pos + 1e-9) best = i;
    any = true;
    if (v[i].pos > pos + 1e-9) break;
  }
  return best;
}

}  // namespace rotator
