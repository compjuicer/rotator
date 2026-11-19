#include "ed_matrix_ops.h"

#include <algorithm>
#include <cmath>

namespace rotator {

static bool is_note(const Event& e) { return e.type == EventType::Note; }

double snap(double x, double grid) { return grid > 0 ? std::round(x / grid) * grid : x; }

Axis lock_axis(Axis cur, double dpitch_px, double dtime_px, double threshold, bool free) {
  if (free) return Axis::Free;
  if (cur == Axis::Time || cur == Axis::Pitch) return cur;
  double ax = std::abs(dpitch_px), ay = std::abs(dtime_px);
  if (std::max(ax, ay) < threshold) return Axis::Undecided;
  return ax > ay ? Axis::Pitch : Axis::Time;
}

NoteHit hit_note(const std::vector<Event>& ev, double pos, int pitch, double min_hit, double edge) {
  NoteHit h;
  for (int i = 0; i < int(ev.size()); ++i) {
    const Event& e = ev[i];
    if (!is_note(e) || e.data1 != pitch || pos < e.pos || pos >= e.pos + std::max(e.len, min_hit)) continue;
    if (h.index >= 0 && e.pos < ev[h.index].pos) continue;
    h.index = i;
    h.end = pos >= e.pos + std::max(e.len - edge, e.len / 2);
  }
  return h;
}

bool same_key(const Event& a, const Event& b) {
  return is_note(a) && is_note(b) && a.data1 == b.data1 && a.chan == b.chan;
}

bool overlaps(const Event& a, const Event& b) { return same_key(a, b) && a.pos < b.pos + b.len && b.pos < a.pos + a.len; }

static std::vector<bool> mask(const std::vector<Event>& ev, const std::vector<int>& sel) {
  std::vector<bool> m(ev.size());
  for (int i : sel)
    if (i >= 0 && i < int(ev.size()) && is_note(ev[i])) m[i] = true;
  return m;
}

bool move_notes(std::vector<Event>& ev, const std::vector<int>& sel, double dpos, int dpitch) {
  auto m = mask(ev, sel);
  for (size_t i = 0; i < ev.size(); ++i)
    if (m[i]) {
      dpos = std::max(dpos, -ev[i].pos);
      dpitch = std::clamp(dpitch, -ev[i].data1, 127 - ev[i].data1);
    }
  std::vector<Event> out = ev;
  for (size_t i = 0; i < ev.size(); ++i)
    if (m[i]) {
      out[i].pos += dpos;
      out[i].data1 += dpitch;
    }
  for (size_t i = 0; i < out.size(); ++i)
    if (m[i])
      for (size_t j = 0; j < out.size(); ++j)
        if (!m[j] && overlaps(out[i], out[j])) return false;
  ev = std::move(out);
  return true;
}

void resize_notes(std::vector<Event>& ev, const std::vector<int>& sel, double dlen, double min_len) {
  auto m = mask(ev, sel);
  std::vector<double> lens(ev.size());
  for (size_t i = 0; i < ev.size(); ++i) {
    if (!m[i]) continue;
    double len = std::max(min_len, ev[i].len + dlen);
    for (const Event& o : ev)
      if (&o != &ev[i] && same_key(o, ev[i]) && o.pos > ev[i].pos) len = std::min(len, o.pos - ev[i].pos);
    lens[i] = len;
  }
  for (size_t i = 0; i < ev.size(); ++i)
    if (m[i]) ev[i].len = lens[i];
}

void add_velocity(std::vector<Event>& ev, const std::vector<int>& sel, int dv) {
  auto m = mask(ev, sel);
  for (size_t i = 0; i < ev.size(); ++i)
    if (m[i]) ev[i].data2 = std::clamp(ev[i].data2 + dv, 1, 127);
}

int insert_note(std::vector<Event>& ev, Event n) {
  n.type = EventType::Note;
  for (const Event& o : ev) {
    if (!same_key(o, n)) continue;
    if (n.pos >= o.pos && n.pos < o.pos + std::max(o.len, 1e-9)) return -1;
    if (o.pos > n.pos) n.len = std::min(n.len, o.pos - n.pos);
  }
  ev.push_back(n);
  return int(ev.size()) - 1;
}

void delete_events(std::vector<Event>& ev, std::vector<int> sel) {
  std::sort(sel.rbegin(), sel.rend());
  sel.erase(std::unique(sel.begin(), sel.end()), sel.end());
  for (int i : sel)
    if (i >= 0 && i < int(ev.size())) ev.erase(ev.begin() + i);
}

std::vector<int> notes_in(const std::vector<Event>& ev, double p0, double p1, int k0, int k1) {
  if (p0 > p1) std::swap(p0, p1);
  if (k0 > k1) std::swap(k0, k1);
  std::vector<int> r;
  for (int i = 0; i < int(ev.size()); ++i) {
    const Event& e = ev[i];
    if (is_note(e) && e.data1 >= k0 && e.data1 <= k1 && e.pos <= p1 && e.pos + e.len >= p0) r.push_back(i);
  }
  return r;
}

}  // namespace rotator
