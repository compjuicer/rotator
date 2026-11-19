#include "recorder.h"

#include <cmath>

namespace rotator {

static double tick(double qn) { return std::round(qn * 192) / 192; }

double Recorder::rel(double qn) const {
  double r = tick(qn - origin);
  if (cycle && len > 0) r = std::fmod(std::fmod(r, len) + len, len);
  return r;
}

bool Recorder::feed(int status, int d1, int d2, double qn, std::vector<Event>& out) {
  double pos = rel(qn);
  int kind = status & 0xF0, chan = (status & 15) + 1;
  bool on = kind == 0x90 && d2 > 0, off = kind == 0x80 || (kind == 0x90 && d2 == 0);
  if (off) {
    auto it = held_.find({chan, d1});
    if (it == held_.end()) return false;
    auto [idx, start] = it->second;
    held_.erase(it);
    double l = pos - start;
    if (l <= 0) l += cycle ? len : 0;
    out[idx].len = std::max(1 / 192.0, tick(l));
    return true;
  }
  if (pos < 0 || (!cycle && pos >= len)) return false;
  switch (kind) {
    case 0x90: out.push_back({EventType::Note, pos, chan, d1, d2, 1 / 192.0}); break;
    case 0xB0: out.push_back({EventType::CC, pos, chan, d1, d2}); break;
    case 0xC0: out.push_back({EventType::Program, pos, chan, d1, 0}); break;
    case 0xE0: out.push_back({EventType::PitchBend, pos, chan, 0, ((d2 << 7) | d1) - 8192}); break;
    case 0xD0: out.push_back({EventType::ChanPressure, pos, chan, 0, d1}); break;
    case 0xA0: out.push_back({EventType::PolyPressure, pos, chan, d1, d2}); break;
    default: return false;
  }
  if (on) held_[{chan, d1}] = {out.size() - 1, pos};
  return !on;
}

void Recorder::finish(double qn, std::vector<Event>& out) {
  double pos = rel(qn);
  for (auto& [key, v] : held_) {
    double l = pos - v.second;
    if (l <= 0) l += cycle ? len : 0;
    out[v.first].len = std::max(1 / 192.0, tick(l));
  }
  held_.clear();
}

}  // namespace rotator
