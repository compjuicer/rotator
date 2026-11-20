#include "model.h"

#include <algorithm>
#include <cmath>

namespace rotator {

double pattern_length(const Pattern& p) {
  double len = 0;
  for (const auto& [_, t] : p.tracks) {
    if (t.loop)
      len = std::max(len, *t.loop);
    else
      for (const auto& e : t.events) len = std::max(len, e.pos + e.len);
  }
  return len;
}

template <class M>
static int lowest_free(const M& m) {
  int n = 1;
  while (m.count(n)) ++n;
  return n;
}

int free_pattern(const Song& s) { return lowest_free(s.patterns); }
int free_track(const Pattern& p) { return lowest_free(p.tracks); }

static std::vector<int> order(const std::vector<Entry>& v) {
  std::vector<int> o(v.size());
  for (size_t i = 0; i < o.size(); ++i) o[i] = int(i);
  std::stable_sort(o.begin(), o.end(), [&](int x, int y) { return v[x].bar < v[y].bar; });
  return o;
}

static size_t rank(const std::vector<int>& o, int index) { return std::find(o.begin(), o.end(), index) - o.begin(); }

std::optional<int> entry_length(const Song& s, char chain, int index) {
  const auto& v = s.chains.at(chain);
  auto o = order(v);
  size_t k = rank(o, index);
  if (k + 1 >= o.size()) return std::nullopt;
  return v[o[k + 1]].bar - v[index].bar;
}

int add_entry(Song& s, char chain, const Entry& e) {
  auto& v = s.chains[chain];
  for (const auto& x : v)
    if (x.bar == e.bar) return -1;
  v.push_back(e);
  return int(v.size()) - 1;
}

void move_start(Song& s, char chain, int index, int bar) {
  auto& v = s.chains.at(chain);
  auto o = order(v);
  size_t k = rank(o, index);
  int cur = v[index].bar;
  int lo = k > 0 ? std::min(cur, v[o[k - 1]].bar + 1) : -9;
  int hi = k + 1 < o.size() ? std::max(cur, v[o[k + 1]].bar - 1) : 9999;
  v[index].bar = std::clamp(bar, lo, hi);
}

int move_to_chain(Song& s, char from, int index, char to) {
  if (from == to) return index;
  auto& v = s.chains.at(from);
  Entry e = v[index];
  v.erase(v.begin() + index);
  auto& w = s.chains[to];
  w.push_back(e);
  return int(w.size()) - 1;
}

static void shift_after(std::vector<Entry>& v, int index, int delta) {
  auto o = order(v);
  for (size_t k = rank(o, index) + 1; k < o.size(); ++k) v[o[k]].bar += delta;
}

bool set_length(Song& s, char chain, int index, int bars) {
  auto len = entry_length(s, chain, index);
  if (!len || bars < 1) return false;
  shift_after(s.chains.at(chain), index, bars - *len);
  return true;
}

void delete_close_gap(Song& s, char chain, int index) {
  auto& v = s.chains.at(chain);
  shift_after(v, index, -entry_length(s, chain, index).value_or(0));
  v.erase(v.begin() + index);
}

bool lane_matches(const HyperLane& l, const Event& e) {
  return e.type == l.type && (!l.chan || *l.chan == e.chan) && (!l.data1 || !has_data1(l.type) || *l.data1 == e.data1);
}

int copy_track(Pattern& p, int src) {
  int n = free_track(p);
  p.tracks[n] = p.tracks.at(src);
  return n;
}

void exchange_tracks(Pattern& p, int a, int b) { std::swap(p.tracks.at(a), p.tracks.at(b)); }

void merge_tracks(Pattern& p, int src, int dst) {
  auto& d = p.tracks.at(dst).events;
  const auto& s = p.tracks.at(src).events;
  d.insert(d.end(), s.begin(), s.end());
  std::stable_sort(d.begin(), d.end(), [](const Event& x, const Event& y) { return x.pos < y.pos; });
}

double quantized(double pos, int quantize) {
  if (quantize <= 0) return pos;
  double grid = 4.0 / quantize;
  return std::round(pos / grid) * grid;
}

static constexpr double kSwing[] = {0.5, 0.541, 0.583, 0.625, 0.666, 0.708};

static double nearest(double pos, std::initializer_list<double> grids) {
  double best = pos, d = 1e9;
  for (double g : grids) {
    double c = std::round(pos / g) * g;
    if (std::abs(c - pos) < d) best = c, d = std::abs(c - pos);
  }
  return best;
}

double grooved(double pos, int groove) {
  switch (groove) {
    case 13: return nearest(pos, {0.5, 1.0 / 3});        // 8-12: 1/8 or 1/8 triplets
    case 14: return nearest(pos, {0.25, 1.0 / 3});       // 1216: 1/16 or 1/8 triplets
    case 15: return nearest(pos, {0.2});                  // quintuplets
    case 16: return nearest(pos, {1.0 / 7});              // septuplets
    case 17: return nearest(pos, {1.0 / 9});              // nine-tuplets
    case 18: {                                            // Free: offbeats between 1/16 and the 3rd triplet stay
      double base = std::floor(pos / 0.5) * 0.5, r = pos - base;
      if (r >= 0.25 && r <= 1.0 / 3) return pos;
      return nearest(pos, {0.25, 1.0 / 3});
    }
  }
  if (groove <= 0 || groove > 12) return pos;
  double step = groove <= 6 ? 0.25 : 0.5, pair = 2 * step;
  double base = std::floor(pos / pair) * pair;
  double best = base;
  for (double c : {base + pair * kSwing[(groove - 1) % 6], base + pair})
    if (std::abs(c - pos) < std::abs(best - pos)) best = c;
  return best;
}

std::string groove_name(int groove) {
  static const char* other[] = {"8-12", "1216", "five", "sept", "nine", "Free"};
  if (groove >= kFirstUserGroove && groove <= kLastGroove) return "Us" + std::to_string(groove - kFirstUserGroove + 1);
  if (groove > 12 && groove < kFirstUserGroove) return other[groove - 13];
  if (groove <= 0 || groove > 12) return "";
  return (groove <= 6 ? "16" : "8") + std::string(1, char('A' + (groove - 1) % 6));
}

int compressed(int v, int compress) {
  switch (compress) {
    case 1: return 64 + (v - 64) / 2;
    case 2: return 64 + (v - 64) / 4;
    case 3: return 64 + (v - 64) / 8;
    case 4: return 64;
    case -1: return std::clamp(64 + (v - 64) * 2, 1, 127);
    case -2: return std::clamp(64 + (v - 64) * 4, 1, 127);
  }
  return v;
}

static const Track* find(const Song& s, int pattern, int track) {
  auto p = s.patterns.find(pattern);
  if (p == s.patterns.end()) return nullptr;
  auto t = p->second.tracks.find(track);
  return t == p->second.tracks.end() ? nullptr : &t->second;
}

// Nearest template note start to pos (template looped by its Loop); nullopt if none.
static std::optional<std::pair<double, int>> template_point(const Song& s, const UserGroove& g, double pos) {
  const Track* t = find(s, g.pattern, g.track);
  if (!t) return std::nullopt;
  std::optional<std::pair<double, int>> best;
  double loop = t->loop.value_or(0);
  double base = loop > 0 ? std::floor(pos / loop) * loop : 0;
  for (double off : loop > 0 ? std::vector<double>{base - loop, base, base + loop} : std::vector<double>{0})
    for (const Event& e : t->events)
      if (e.type == EventType::Note && (loop <= 0 || e.pos < loop) &&
          (!best || std::abs(e.pos + off - pos) < std::abs(best->first - pos)))
        best = std::pair{e.pos + off, e.data2};
  return best;
}

static double toward(double pos, double target, int range, int strength) {
  if (range > 0 && std::abs(target - pos) * 192 > range) return pos;
  return pos + (target - pos) * strength / 100.0;
}

static double groove_pos(const Song& s, int groove, double pos) {
  if (groove < kFirstUserGroove || groove > kLastGroove) return grooved(pos, groove);
  const UserGroove& g = s.grooves[groove - kFirstUserGroove];
  auto p = template_point(s, g, pos);
  return p ? toward(pos, p->first, g.range, g.strength) : pos;
}

Event placed(const Song& s, const Track& t, Event e) {
  if (!t.quantize && !t.groove) return e;
  if (e.type != EventType::Note && s.quant_mode != QuantMode::All) return e;
  auto snap = [&](double p) {
    double q = t.quantize ? toward(p, quantized(p, t.quantize), s.quant_range, s.quant_strength) : p;
    return t.groove ? groove_pos(s, t.groove, q) : q;
  };
  double end = e.pos + e.len;
  e.pos = snap(e.pos);
  if (e.type == EventType::Note) {
    if (s.quant_mode == QuantMode::NoteOnOff) e.len = std::max(1 / 192.0, snap(end) - e.pos);
    else if (s.quant_mode == QuantMode::NoteOn) e.len = std::max(1 / 192.0, end - e.pos);
  }
  return e;
}

void do_quantize(const Song& s, Track& t) {
  for (Event& e : t.events) {
    e = placed(s, t, e);
    e.pos = std::max(0.0, e.pos);
  }
  std::stable_sort(t.events.begin(), t.events.end(), [](const Event& x, const Event& y) { return x.pos < y.pos; });
  t.quantize = t.groove = 0;
}

bool dynamic_groove(const Song& s, Track& t, int user_groove, int amount) {
  if (user_groove < 0 || user_groove >= 16) return false;
  const UserGroove& g = s.grooves[user_groove];
  bool changed = false;
  for (Event& e : t.events) {
    if (e.type != EventType::Note) continue;
    auto p = template_point(s, g, e.pos);
    if (!p || (g.range > 0 && std::abs(p->first - e.pos) * 192 > g.range)) continue;
    int v = std::clamp(int(std::lround(e.data2 + (p->second - e.data2) * amount / 15.0)), 1, 127);
    changed |= v != e.data2;
    e.data2 = v;
  }
  return changed;
}

bool filtered(const Track& t, const Event& e) {
  return (t.filter >> int(e.type) & 1) || (t.chan_filter && t.chan && e.chan != *t.chan);
}

void copy_entry_params(Song& s, char chain, int index, int what, bool all_following) {
  auto& v = s.chains.at(chain);
  auto o = order(v);
  const Entry src = v[index];
  for (size_t k = rank(o, index) + 1; k < o.size(); ++k) {
    Entry& e = v[o[k]];
    if (what & 1) e.transpose = src.transpose;
    if (what & 2) e.shift = src.shift;
    if (what & 4) e.mutes = src.mutes;
    if (!all_following) break;
  }
}

void move_keep_length(Song& s, char chain, int index, int bar) {
  auto& v = s.chains.at(chain);
  int delta = bar - v[index].bar;
  if (!delta) return;
  auto o = order(v);
  size_t k = rank(o, index);
  if (k > 0 && bar <= v[o[k - 1]].bar) return;
  for (size_t j = k; j < o.size(); ++j) v[o[j]].bar += delta;
}

void normalize(Track& t) {
  std::vector<Event> out;
  for (Event e : t.events) {
    e.pos = std::max(0.0, e.pos + t.delay);
    if (e.type == EventType::Note || e.type == EventType::PolyPressure) {
      e.data1 += t.transpose;
      if (e.data1 < t.lowest || e.data1 > t.highest || e.data1 < 0 || e.data1 > 127) continue;
    }
    if (e.type == EventType::Note) e.data2 = std::clamp(compressed(e.data2, t.compress) + t.velocity, 1, 127);
    out.push_back(e);
  }
  std::stable_sort(out.begin(), out.end(), [](const Event& x, const Event& y) { return x.pos < y.pos; });
  t.events = out;
  t.delay = 0;
  t.transpose = t.velocity = t.compress = 0;
  t.lowest = 0;
  t.highest = 127;
}

const Track* ghost_source(const Song& s, const Track& t) {
  if (!t.ghost_pattern || !t.ghost_track) return nullptr;
  auto p = s.patterns.find(t.ghost_pattern);
  if (p == s.patterns.end()) return nullptr;
  auto tr = p->second.tracks.find(t.ghost_track);
  return tr == p->second.tracks.end() || &tr->second == &t ? nullptr : &tr->second;
}

// Emits the track's events whose position (QN from pattern start) falls in [lo, hi).
static void render_track(const Song& song, const Track& t, const HyperSet* set, double lo, double hi, int entry_transpose,
                         std::vector<Event>& out) {
  const Track* src = ghost_source(song, t);
  const Track& data = src ? *src : t;  // a ghost plays the source's events, quantize, groove and loop
  double delay = src ? std::max(0.0, t.delay) : t.delay;
  auto emit = [&](double pos, Event e) {
    e.pos = pos + delay;
    if (set)
      for (const auto& l : set->lanes)
        if (lane_matches(l, e)) {
          e.pos += l.delay;
          break;
        }
    if (e.pos < lo || e.pos >= hi) return;
    if (filtered(t, e)) return;
    e.chan = t.chan.value_or(e.chan);
    if (e.type == EventType::Note || e.type == EventType::PolyPressure) {
      e.data1 += t.transpose + (song.no_arrange_transpose.count({t.dest, e.chan}) ? 0 : entry_transpose);
      if (e.data1 < t.lowest || e.data1 > t.highest || e.data1 < 0 || e.data1 > 127) return;
    }
    if (e.type == EventType::Note) {
      e.data2 = std::clamp(compressed(e.data2, t.compress) + t.velocity, 1, 127);
      e.len = std::min(e.len, hi - e.pos);
    } else {
      e.len = 0;
    }
    out.push_back(e);
  };

  if (data.loop) {
    if (*data.loop <= 0) return;
    for (double offset = 0; offset < hi; offset += *data.loop)
      for (const auto& e : data.events)
        if (e.pos < *data.loop) {
          Event p = e;
          p.pos += offset;
          p = placed(song, data, p);
          emit(p.pos, p);
        }
  } else {
    for (const auto& e : data.events) {
      Event p = placed(song, data, e);
      emit(p.pos, p);
    }
  }
}

std::vector<Item> render(const Song& song, const std::function<double(int)>& bar_to_qn) {
  std::vector<Item> items;
  for (char cname : kChains) {
    auto it = song.chains.find(cname);
    if (it == song.chains.end()) continue;
    auto entries = it->second;
    std::stable_sort(entries.begin(), entries.end(), [](const Entry& x, const Entry& y) { return x.bar < y.bar; });
    for (size_t k = 0; k < entries.size(); ++k) {
      const Entry& entry = entries[k];
      auto pit = song.patterns.find(entry.pattern);
      if (pit == song.patterns.end()) continue;
      const Pattern& pattern = pit->second;
      double origin = bar_to_qn(entry.bar);
      double lo = entry.shift / 192.0;
      double hi = k + 1 < entries.size() ? bar_to_qn(entries[k + 1].bar) + entries[k + 1].shift / 192.0 - origin
                                         : pattern_length(pattern);
      if (hi <= lo) continue;
      std::map<std::string, std::vector<Event>> by_dest;
      for (const auto& [i, track] : pattern.tracks) {
        auto m = entry.mutes.find(i);
        bool soloed_out = song.solo_track && song.solo_chain == cname && song.solo_track != i;
        bool muted = track.mute || track.hide || soloed_out || (m != entry.mutes.end() && m->second);
        if (track.dest.empty() || muted) continue;
        bool hs = track.hyper_set >= 0 && track.hyper_set < int(song.hyper_sets.size());
        render_track(song, track, hs ? &song.hyper_sets[track.hyper_set] : nullptr, lo, hi, entry.transpose, by_dest[track.dest]);
      }
      for (auto& [dest, events] : by_dest) {
        for (auto& e : events) e.pos += origin;
        items.push_back({dest, cname, entry, origin + lo, origin + hi, std::move(events)});
      }
    }
  }
  return items;
}

}  // namespace rotator
