#include "ed_hyper_ops.h"

#include "ed_event_ops.h"

#include <algorithm>
#include <cmath>

namespace rotator {

constexpr double kEps = 1e-9;

std::pair<int, int> value_range(EventType t) {
  if (t == EventType::Note) return {1, 127};
  if (t == EventType::PitchBend) return {-8192, 8191};
  return {0, 127};
}

int event_value(const Event& e) { return e.type == EventType::Program ? e.data1 : e.data2; }

void set_event_value(Event& e, int v) {
  auto [lo, hi] = value_range(e.type);
  (e.type == EventType::Program ? e.data1 : e.data2) = std::clamp(v, lo, hi);
}

std::string lane_label(EventType t, int data1) {
  auto pitch = [&] { return note_name(data1); };
  switch (t) {
    case EventType::Note: return pitch();
    case EventType::CC: return "CC " + std::to_string(data1);
    case EventType::Program: return "Program";
    case EventType::PitchBend: return "Bend";
    case EventType::ChanPressure: return "Pressure";
    case EventType::PolyPressure: return "PolyAT " + pitch();
  }
  return {};
}

HyperSet default_hyper_set() {
  HyperSet s{"GM drums", {}};
  auto drum = [&](const char* name, int pitch, bool hhat = false) {
    HyperLane l;
    l.name = name;
    l.data1 = pitch;
    l.hhat = hhat;
    s.lanes.push_back(l);
  };
  drum("Kick", 36);
  drum("Snare", 38);
  drum("Clap", 39);
  drum("Hihat CL", 42, true);
  drum("Hihat PD", 44, true);
  drum("Hihat OP", 46, true);
  drum("Crash", 49, true);
  drum("Ride", 51);
  drum("Tom L", 45);
  drum("Tom M", 47);
  drum("Tom H", 50);
  HyperLane mod;
  mod.name = "Modwheel";
  mod.type = EventType::CC;
  mod.data1 = 1;
  mod.grid = 0.125;
  s.lanes.push_back(mod);
  return s;
}

HyperLane lane_from_event(const Event& e) {
  HyperLane l;
  l.type = e.type;
  if (has_data1(e.type)) l.data1 = e.data1;
  l.name = lane_label(e.type, e.data1);
  if (e.type != EventType::Note) l.grid = 0.125;
  return l;
}

int add_unmatched_lanes(HyperSet& s, const Track& t) {
  int added = 0;
  for (const Event& e : t.events)
    if (std::none_of(s.lanes.begin(), s.lanes.end(), [&](const HyperLane& l) { return lane_matches(l, e); })) {
      s.lanes.push_back(lane_from_event(e));
      ++added;
    }
  return added;
}

bool can_insert(const HyperLane& l) { return l.data1 || !has_data1(l.type); }

std::pair<int, int> hhat_group(const HyperSet& s, int lane) {
  int a = lane, b = lane;
  if (!s.lanes[lane].hhat) return {a, b};
  while (a > 0 && s.lanes[a - 1].hhat) --a;
  while (b + 1 < int(s.lanes.size()) && s.lanes[b + 1].hhat) ++b;
  return {a, b};
}

static int insert_chan(const Track& t, const HyperLane& l) {
  if (l.chan) return *l.chan;
  for (const Event& e : t.events)
    if (lane_matches(l, e)) return e.chan;
  return t.events.empty() ? 1 : t.events.front().chan;
}

static void insert_sorted(Track& t, const Event& e) {
  auto it = std::upper_bound(t.events.begin(), t.events.end(), e.pos, [](double p, const Event& x) { return p < x.pos; });
  t.events.insert(it, e);
}

static void clear_group_mates(Track& t, const HyperSet& s, int lane, double pos) {
  auto [a, b] = hhat_group(s, lane);
  const HyperLane& self = s.lanes[lane];
  for (int j = a; j <= b; ++j) {
    if (j == lane) continue;
    const HyperLane& m = s.lanes[j];
    t.events.erase(std::remove_if(t.events.begin(), t.events.end(),
                                  [&](const Event& e) {
                                    return std::abs(e.pos - pos) < kEps && lane_matches(m, e) && !lane_matches(self, e);
                                  }),
                   t.events.end());
  }
}

void paint(Track& t, const HyperSet& s, int lane, const Stroke& st, bool change, bool insert, std::optional<int> fixed) {
  const HyperLane& l = s.lanes[lane];
  double lo = std::min(st.t0, st.t1), hi = std::max(st.t0, st.t1);
  auto value = [&](double p) {
    if (fixed && l.type == EventType::Note) return *fixed;
    double f = st.t1 == st.t0 ? 1 : std::clamp((p - st.t0) / (st.t1 - st.t0), 0.0, 1.0);
    return int(std::lround(st.v0 + f * (st.v1 - st.v0)));
  };
  if (change)
    for (Event& e : t.events)
      if (lane_matches(l, e) && e.pos >= lo - st.tol && e.pos <= hi + st.tol) set_event_value(e, value(e.pos));
  if (!insert || !can_insert(l) || l.grid <= 0) return;
  for (long k = std::max(0L, std::lround(lo / l.grid)); k <= std::lround(hi / l.grid); ++k) {
    double pos = k * l.grid;
    if (std::any_of(t.events.begin(), t.events.end(), [&](const Event& e) {
          return lane_matches(l, e) && e.pos >= pos - l.grid / 2 - kEps && e.pos < pos + l.grid / 2 - kEps;
        }))
      continue;
    Event e{l.type, pos, insert_chan(t, l), has_data1(l.type) ? *l.data1 : 0, 0, l.type == EventType::Note ? l.len : 0};
    set_event_value(e, value(pos));
    if (l.type == EventType::Note) clear_group_mates(t, s, lane, pos);
    insert_sorted(t, e);
  }
}

int erase(Track& t, const HyperLane& l, double lo, double hi, bool grid_only) {
  auto on_grid = [&](double p) { return l.grid > 0 && std::abs(p - std::round(p / l.grid) * l.grid) < 1e-6; };
  size_t n = t.events.size();
  t.events.erase(std::remove_if(t.events.begin(), t.events.end(),
                                [&](const Event& e) {
                                  return lane_matches(l, e) && e.pos >= lo && e.pos <= hi && (!grid_only || on_grid(e.pos));
                                }),
                 t.events.end());
  return int(n - t.events.size());
}

void offset_lane(Track& t, const HyperLane& l, int delta) {
  for (Event& e : t.events)
    if (lane_matches(l, e)) set_event_value(e, event_value(e) + delta);
}

static void retarget(Event& e, const HyperLane& to) {
  auto [lo0, hi0] = value_range(e.type);
  int v = event_value(e);
  bool scale = (e.type == EventType::PitchBend) != (to.type == EventType::PitchBend);
  e.type = to.type;
  if (to.chan) e.chan = *to.chan;
  if (!has_data1(to.type)) e.data1 = 0;
  else if (to.data1) e.data1 = *to.data1;
  e.len = to.type != EventType::Note ? 0 : e.len > 0 ? e.len : to.len;
  if (scale) {
    auto [lo1, hi1] = value_range(to.type);
    v = int(std::lround(lo1 + double(v - lo0) * (hi1 - lo1) / (hi0 - lo0)));
  }
  set_event_value(e, v);
}

void convert_lane(Track& t, const HyperLane& from, const HyperLane& to, bool copy) {
  std::vector<Event> added;
  for (Event& e : t.events) {
    if (!lane_matches(from, e)) continue;
    if (!copy) {
      retarget(e, to);
    } else if (!lane_matches(to, e)) {
      added.push_back(e);
      retarget(added.back(), to);
    }
  }
  for (const Event& e : added) insert_sorted(t, e);
}

void convert_set(Track& t, const HyperSet& from, const HyperSet& to) {
  for (Event& e : t.events)
    for (size_t i = 0; i < from.lanes.size(); ++i)
      if (lane_matches(from.lanes[i], e)) {
        if (i < to.lanes.size()) retarget(e, to.lanes[i]);
        break;
      }
}

int delete_hhat_duplicates(Track& t, const HyperSet& s, int lane) {
  const HyperLane& self = s.lanes[lane];
  auto [a0, b0] = hhat_group(s, lane);
  int a = a0, b = b0;
  std::vector<double> pos;
  for (const Event& e : t.events)
    if (lane_matches(self, e)) pos.push_back(e.pos);
  size_t n = t.events.size();
  t.events.erase(std::remove_if(t.events.begin(), t.events.end(),
                                [&](const Event& e) {
                                  if (lane_matches(self, e)) return false;
                                  bool mate = false;
                                  for (int j = a; j <= b; ++j) mate |= j != lane && lane_matches(s.lanes[j], e);
                                  return mate && std::any_of(pos.begin(), pos.end(),
                                                             [&](double p) { return std::abs(p - e.pos) < kEps; });
                                }),
                 t.events.end());
  return int(n - t.events.size());
}

void delete_hyper_set(Song& s, int index) {
  s.hyper_sets.erase(s.hyper_sets.begin() + index);
  for (auto& [_, p] : s.patterns)
    for (auto& [_, t] : p.tracks) {
      if (t.hyper_set == index) t.hyper_set = -1;
      else if (t.hyper_set > index) --t.hyper_set;
    }
}

}  // namespace rotator
