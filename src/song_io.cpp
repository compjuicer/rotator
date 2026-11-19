#include "song_io.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "lineparse.h"

namespace rotator {

static std::string quote(std::string s) {
  for (char q : {'"', '\'', '`'})
    if (s.find(q) == std::string::npos) return q + s + q;
  for (char& c : s)
    if (c == '`') c = '\'';
  return '`' + s + '`';
}

static std::string num(double d) {
  char buf[32];
  for (int prec = 15; prec <= 17; ++prec) {
    std::snprintf(buf, sizeof buf, "%.*g", prec, d);
    if (std::strtod(buf, nullptr) == d) break;
  }
  return buf;
}

static std::string event_line(const Event& e) {
  auto i = [](int v) { return " " + std::to_string(v); };
  std::string head = num(e.pos) + i(e.chan);
  switch (e.type) {
    case EventType::Note: return "N " + num(e.pos) + " " + num(e.len) + i(e.data1) + i(e.data2) + i(e.chan);
    case EventType::CC: return "C " + head + i(e.data1) + i(e.data2);
    case EventType::Program: return "P " + head + i(e.data1);
    case EventType::PitchBend: return "B " + head + i(e.data2);
    case EventType::ChanPressure: return "A " + head + i(e.data2);
    case EventType::PolyPressure: return "K " + head + i(e.data1) + i(e.data2);
  }
  return {};
}

static const char kTypes[] = "NCPBAK";

std::vector<std::string> to_lines(const Song& song) {
  std::vector<std::string> out;
  for (const auto& [pn, p] : song.patterns) {
    out.push_back("PATTERN " + std::to_string(pn) + " " + quote(p.name));
    for (const auto& [tn, t] : p.tracks) {
      out.push_back("TRACK " + std::to_string(tn) + " " + quote(t.name) + " " + quote(t.dest) + " " +
                    std::to_string(t.chan.value_or(0)) + " " + std::to_string(t.transpose) + " " +
                    std::to_string(t.velocity) + " " + num(t.loop.value_or(0)) + " " + num(t.delay) + " " +
                    std::to_string(t.lowest) + " " + std::to_string(t.highest) + " " + std::to_string(t.mute) +
                    " " + std::to_string(t.hide) + " " + std::to_string(t.quantize) + " " +
                    std::to_string(t.groove) + " " + std::to_string(t.compress) + " " + std::to_string(t.ghost_pattern) +
                    " " + std::to_string(t.ghost_track) + " " + std::to_string(t.filter) + " " +
                    std::to_string(t.chan_filter));
      if (t.hyper_set >= 0) out.push_back("HTRACK " + std::to_string(t.hyper_set));
      for (const Event& e : t.events) out.push_back(event_line(e));
    }
  }
  for (const auto& [c, entries] : song.chains)
    for (const Entry& e : entries) {
      std::string mutes;
      for (const auto& [tn, m] : e.mutes)
        if (m) mutes += (mutes.empty() ? "" : ",") + std::to_string(tn);
      out.push_back("ENTRY " + std::string(1, c) + " " + std::to_string(e.bar) + " " + std::to_string(e.pattern) +
                    " " + std::to_string(e.transpose) + " " + quote(e.name) + " " + quote(mutes) + " " +
                    std::to_string(e.shift));
    }
  for (const HyperSet& hs : song.hyper_sets) {
    out.push_back("HSET " + quote(hs.name));
    for (const HyperLane& l : hs.lanes)
      out.push_back("HLANE " + quote(l.name) + " " + kTypes[int(l.type)] + " " + std::to_string(l.data1.value_or(-1)) +
                    " " + std::to_string(l.chan.value_or(0)) + " " + num(l.grid) + " " + num(l.len) + " " +
                    num(l.delay) + " " + std::to_string(l.hhat));
  }
  if (song.quant_mode != QuantMode::Musical || song.quant_range || song.quant_strength != 100)
    out.push_back("QMODE " + std::to_string(int(song.quant_mode)) + " " + std::to_string(song.quant_range) + " " +
                  std::to_string(song.quant_strength));
  for (int i = 0; i < 16; ++i)
    if (const UserGroove& g = song.grooves[i]; g.pattern)
      out.push_back("UGROOVE " + std::to_string(i + 1) + " " + std::to_string(g.pattern) + " " + std::to_string(g.track) +
                    " " + std::to_string(g.range) + " " + std::to_string(g.strength));
  for (int i = 0; i < 10; ++i) {
    std::string m;
    for (const auto& [tn, on] : song.mute_groups[i])
      if (on) m += (m.empty() ? "" : ",") + std::to_string(tn);
    if (!m.empty()) out.push_back("MGROUP " + std::to_string(i + 1) + " " + quote(m));
  }
  if (song.solo_track) out.push_back("SOLO " + std::string(1, song.solo_chain) + " " + std::to_string(song.solo_track));
  for (const auto& [dest, ch] : song.no_arrange_transpose)
    out.push_back("NOXPOSE " + quote(dest) + " " + std::to_string(ch));
  return out;
}

Song from_lines(const std::vector<std::string>& lines) {
  Song song;
  Pattern* pat = nullptr;
  Track* trk = nullptr;
  LineParser lp;
  for (const std::string& line : lines) {
    if (lp.parse_ex(line.c_str()) < 0 || lp.getnumtokens() < 1) continue;
    std::string rec = lp.gettoken_str(0);
    int n = lp.getnumtokens();
    if (rec == "PATTERN" && n >= 3) {
      pat = &song.patterns[lp.gettoken_int(1)];
      pat->name = lp.gettoken_str(2);
      trk = nullptr;
    } else if (rec == "TRACK" && n >= 13 && pat) {
      trk = &pat->tracks[lp.gettoken_int(1)];
      trk->name = lp.gettoken_str(2);
      trk->dest = lp.gettoken_str(3);
      if (int ch = lp.gettoken_int(4)) trk->chan = ch;
      trk->transpose = lp.gettoken_int(5);
      trk->velocity = lp.gettoken_int(6);
      if (double l = lp.gettoken_float(7)) trk->loop = l;
      trk->delay = lp.gettoken_float(8);
      trk->lowest = lp.gettoken_int(9);
      trk->highest = lp.gettoken_int(10);
      trk->mute = lp.gettoken_int(11);
      trk->hide = lp.gettoken_int(12);
      if (n >= 14) trk->quantize = lp.gettoken_int(13);
      if (n >= 18) {
        trk->groove = lp.gettoken_int(14);
        trk->compress = lp.gettoken_int(15);
        trk->ghost_pattern = lp.gettoken_int(16);
        trk->ghost_track = lp.gettoken_int(17);
      }
      if (n >= 20) {
        trk->filter = lp.gettoken_int(18);
        trk->chan_filter = lp.gettoken_int(19);
      }
    } else if ((rec == "N" || rec == "E") && n >= 6 && trk) {
      trk->events.push_back({EventType::Note, lp.gettoken_float(1), lp.gettoken_int(5), lp.gettoken_int(3),
                             lp.gettoken_int(4), lp.gettoken_float(2)});
    } else if (rec == "C" && n >= 5 && trk) {
      trk->events.push_back({EventType::CC, lp.gettoken_float(1), lp.gettoken_int(2), lp.gettoken_int(3), lp.gettoken_int(4)});
    } else if (rec == "P" && n >= 4 && trk) {
      trk->events.push_back({EventType::Program, lp.gettoken_float(1), lp.gettoken_int(2), lp.gettoken_int(3), 0});
    } else if (rec == "B" && n >= 4 && trk) {
      trk->events.push_back({EventType::PitchBend, lp.gettoken_float(1), lp.gettoken_int(2), 0, lp.gettoken_int(3)});
    } else if (rec == "A" && n >= 4 && trk) {
      trk->events.push_back({EventType::ChanPressure, lp.gettoken_float(1), lp.gettoken_int(2), 0, lp.gettoken_int(3)});
    } else if (rec == "K" && n >= 5 && trk) {
      trk->events.push_back({EventType::PolyPressure, lp.gettoken_float(1), lp.gettoken_int(2), lp.gettoken_int(3), lp.gettoken_int(4)});
    } else if (rec == "HTRACK" && n >= 2 && trk) {
      trk->hyper_set = lp.gettoken_int(1);
    } else if (rec == "HSET" && n >= 2) {
      song.hyper_sets.push_back({lp.gettoken_str(1), {}});
    } else if (rec == "HLANE" && n >= 9 && !song.hyper_sets.empty()) {
      HyperLane l;
      l.name = lp.gettoken_str(1);
      const char* t = std::strchr(kTypes, lp.gettoken_str(2)[0]);
      if (!t || !*t) continue;
      l.type = EventType(t - kTypes);
      if (int d = lp.gettoken_int(3); d >= 0) l.data1 = d;
      if (int c = lp.gettoken_int(4)) l.chan = c;
      l.grid = lp.gettoken_float(5);
      l.len = lp.gettoken_float(6);
      l.delay = lp.gettoken_float(7);
      l.hhat = lp.gettoken_int(8);
      song.hyper_sets.back().lanes.push_back(l);
    } else if (rec == "QMODE" && n >= 4) {
      song.quant_mode = QuantMode(std::clamp(lp.gettoken_int(1), 0, 3));
      song.quant_range = lp.gettoken_int(2);
      song.quant_strength = lp.gettoken_int(3);
    } else if (rec == "UGROOVE" && n >= 6) {
      if (int i = lp.gettoken_int(1); i >= 1 && i <= 16)
        song.grooves[i - 1] = {lp.gettoken_int(2), lp.gettoken_int(3), lp.gettoken_int(4), lp.gettoken_int(5)};
    } else if (rec == "MGROUP" && n >= 3) {
      int i = lp.gettoken_int(1);
      if (i < 1 || i > 10) continue;
      for (const char* p = lp.gettoken_str(2); *p;) {
        char* end;
        long tn = std::strtol(p, &end, 10);
        if (end == p) break;
        song.mute_groups[i - 1][(int)tn] = true;
        p = *end == ',' ? end + 1 : end;
      }
    } else if (rec == "SOLO" && n >= 3) {
      song.solo_chain = lp.gettoken_str(1)[0];
      song.solo_track = lp.gettoken_int(2);
    } else if (rec == "NOXPOSE" && n >= 3) {
      song.no_arrange_transpose.insert({lp.gettoken_str(1), lp.gettoken_int(2)});
    } else if (rec == "ENTRY" && n >= 7) {
      Entry e;
      e.bar = lp.gettoken_int(2);
      e.pattern = lp.gettoken_int(3);
      e.transpose = lp.gettoken_int(4);
      e.name = lp.gettoken_str(5);
      for (const char* p = lp.gettoken_str(6); *p;) {
        char* end;
        long tn = std::strtol(p, &end, 10);
        if (end == p) break;
        e.mutes[(int)tn] = true;
        p = *end == ',' ? end + 1 : end;
      }
      if (n >= 8) e.shift = lp.gettoken_int(7);
      song.chains[lp.gettoken_str(1)[0]].push_back(e);
    }
  }
  return song;
}

}  // namespace rotator
