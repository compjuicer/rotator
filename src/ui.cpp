#include "ui.h"

#include "reaper_core_api.h"
#include "reaper_out.h"
#define REAIMGUIAPI_IMPLEMENT
#include "reaper_imgui_functions.h"
#include "editor.h"
#include "ed_event_ops.h"
#include "st.h"
#include "gem.h"
#include "recorder.h"
#include "song_io.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <tuple>
#include <vector>

#undef min
#undef max

namespace G = ImGui;
using rotator::Song;
using st::Font;
using st::kInk;
using st::kPaper;

namespace {

void* (*g_get_func)(const char*) = nullptr;
bool g_api = false, g_open = false;
ImGui_Context* g_ctx = nullptr;
std::string g_error;

char g_chain = 0;
int g_index = -1, g_pattern = 1, g_track = 1;
int g_force_tab = 0;
int g_dock_req = 0;
bool g_arrange_on = true, g_gam = false, g_edit_page = false, g_catch = true, g_rmg = false;
char g_view_chain = 'a';
int g_param = 1, g_list_top = 0;
bool g_open_nx = false, g_open_ugroove = false, g_open_filter = false;
double g_midi_in_t = -10;
int g_midi_seq = -1;

ReaProject* g_proj = nullptr;
Song* g_song = nullptr;
std::function<bool(Song&)> g_op;
const char* g_op_desc = "";

void commit(const char* desc) { ::commit(g_proj, *g_song, desc); }

// After a value field changed: preview while sliding, undo point when the gesture ends.
void changed(const char* desc) {
  if (st::final()) commit(desc);
  else preview(*g_song);
}

void later(const char* desc, std::function<bool(Song&)> op) {
  g_op_desc = desc;
  g_op = std::move(op);
}

double qn_time(double qn) { return TimeMap2_QNToTime(g_proj, qn); }

std::string pad(std::string s, size_t n) {
  if (s.size() > n) s.resize(n);
  return s;
}

std::string rpad(const std::string& s, size_t n) { return s.size() >= n ? s : std::string(n - s.size(), ' ') + s; }

// ---------- arrange selection / transport ----------

void loop_selected(const Song& s) {
  if (g_arrange_on || !g_chain) {
    GetSetRepeatEx(g_proj, 0);
    return;
  }
  const rotator::Entry& e = s.chains.at(g_chain)[g_index];
  auto len = rotator::entry_length(s, g_chain, g_index);
  auto p = s.patterns.find(e.pattern);
  double start = bar_to_qn(e.bar);
  double end = len ? bar_to_qn(e.bar + *len)
                   : start + (p == s.patterns.end() ? 4 : std::max(1.0, rotator::pattern_length(p->second)));
  double t0 = qn_time(start), t1 = qn_time(end);
  GetSet_LoopTimeRange2(g_proj, true, true, &t0, &t1, false);
  GetSetRepeatEx(g_proj, 1);
}

void select(char chain, int index, const Song& s, bool move_cursor = true) {
  g_chain = chain;
  g_index = index;
  g_pattern = s.chains.at(chain)[index].pattern;
  g_view_chain = chain;
  if (move_cursor) SetEditCurPos2(g_proj, qn_time(bar_to_qn(s.chains.at(chain)[index].bar)), true, false);
  if (!g_arrange_on) loop_selected(s);
}

rotator::Entry* selected_entry(Song& s) {
  auto c = s.chains.find(g_chain);
  if (c == s.chains.end() || g_index < 0 || g_index >= int(c->second.size())) return nullptr;
  return &c->second[g_index];
}

double cursor_time() { return GetPlayStateEx(g_proj) & 1 ? GetPlayPositionEx(g_proj) : GetCursorPositionEx(g_proj); }

double play_bar() {
  int meas = 0, cml = 4;
  double beats = TimeMap2_timeToBeats(g_proj, cursor_time(), &meas, &cml, nullptr, nullptr);
  return meas + 1 + beats / std::max(1, cml);
}

void step_bar(int d) {
  int meas = 0;
  TimeMap2_timeToBeats(g_proj, GetCursorPositionEx(g_proj) + 1e-6, &meas, nullptr, nullptr, nullptr);
  meas = std::max(0, meas + d);
  SetEditCurPos2(g_proj, TimeMap2_beatsToTime(g_proj, 0, &meas), true, false);
}

using Row = std::tuple<int, char, int>;  // bar, chain, index

std::vector<Row> entry_rows(const Song& s) {
  std::vector<Row> rows;
  for (auto& [c, v] : s.chains)
    for (int i = 0; i < int(v.size()); ++i) rows.emplace_back(v[i].bar, c, i);
  std::sort(rows.begin(), rows.end());
  return rows;
}

void step_entry(const Song& s, int d) {
  auto rows = entry_rows(s);
  if (rows.empty()) return;
  int r = d > 0 ? 0 : int(rows.size()) - 1;
  for (int k = 0; k < int(rows.size()); ++k)
    if (std::get<1>(rows[k]) == g_chain && std::get<2>(rows[k]) == g_index) r = k + d;
  r = std::clamp(r, 0, int(rows.size()) - 1);
  select(std::get<1>(rows[r]), std::get<2>(rows[r]), s);
}

int next_bar(const Song& s, char chain) {
  auto c = s.chains.find(chain);
  if (c == s.chains.end() || c->second.empty()) return 1;
  auto last = *std::max_element(c->second.begin(), c->second.end(), [](auto& x, auto& y) { return x.bar < y.bar; });
  auto p = s.patterns.find(last.pattern);
  double end = bar_to_qn(last.bar) + (p == s.patterns.end() ? 0 : rotator::pattern_length(p->second));
  int bar = last.bar + 1;
  while (bar_to_qn(bar) < end - 1e-9) ++bar;
  return bar;
}

std::string entry_name(const Song& s, const rotator::Entry& e) {
  if (!e.name.empty()) return e.name;
  auto p = s.patterns.find(e.pattern);
  return p != s.patterns.end() && !p->second.name.empty() ? p->second.name : "Pattern:";
}

// ---------- naming box ----------

std::string g_rename_key;
char g_rename_buf[64];
bool g_rename_focus = false;

void start_rename(const std::string& key, const std::string& cur) {
  g_rename_key = key;
  g_rename_focus = true;
  snprintf(g_rename_buf, sizeof g_rename_buf, "%s", cur.c_str());
}

// Naming box opened by double-click, drawn at (x, y). apply(name) runs on Return.
void rename_box(const std::string& key, double x, double y, double w, const std::function<void(const std::string&)>& apply) {
  if (g_rename_key != key) return;
  G::SetCursorScreenPos(g_ctx, st::sx(x), st::sy(y));
  G::SetNextItemWidth(g_ctx, w * st::scale());
  if (g_rename_focus) G::SetKeyboardFocusHere(g_ctx);
  bool done = G::InputText(g_ctx, ("##rn" + key).c_str(), g_rename_buf, sizeof g_rename_buf,
                           G::InputTextFlags_EnterReturnsTrue | G::InputTextFlags_AutoSelectAll);
  bool focus_frame = g_rename_focus;
  g_rename_focus = false;
  if (done) {
    apply(g_rename_buf);
    g_rename_key.clear();
  } else if (G::IsKeyPressed(g_ctx, G::Key_Escape) ||
             (!focus_frame && G::IsMouseClicked(g_ctx, G::MouseButton_Left) && !G::IsItemHovered(g_ctx))) {
    g_rename_key.clear();
  }
}

// ---------- tracks and parameters ----------

rotator::Track& ensure_track(Song& s, int n) { return s.patterns[g_pattern].tracks[n]; }

const rotator::Track* find_track(const Song& s, int n) {
  auto p = s.patterns.find(g_pattern);
  if (p == s.patterns.end()) return nullptr;
  auto t = p->second.tracks.find(n);
  return t == p->second.tracks.end() ? nullptr : &t->second;
}

std::string dest_name(const std::string& guid) {
  if (guid.empty()) return "";
  MediaTrack* tr = dest_track(guid);
  if (!tr) return "missing";
  char name[256] = "";
  GetTrackName(tr, name, sizeof name);
  return name;
}

int dest_index(const std::string& guid) {
  if (guid.empty()) return 0;
  for (int i = 0; i < CountTracks(g_proj); ++i)
    if (track_guid(GetTrack(g_proj, i)) == guid) return i + 1;
  return 0;
}

struct Param {
  const char* name;
  bool live;
};
const Param kParams[] = {{"OUTPUT", true},   {"CHANNEL", true},  {"QUANTIZE", true}, {"GROOVE", true},
                         {"TRANSPOS", true}, {"VELOCITY", true}, {"COMPRESS", true}, {"LOOP", true},
                         {"DELAY", true},    {"LOWEST", true},   {"HIGHEST", true},   {"GHOST OF", true}};
constexpr int kNumParams = sizeof kParams / sizeof kParams[0];

constexpr int kQuant[] = {0, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96};

int quant_index(int q) {
  for (int i = 0; i < 11; ++i)
    if (kQuant[i] == q) return i;
  return 0;
}

int get_param(const rotator::Track& t, int p) {
  switch (p) {
    case 2: return quant_index(t.quantize);
    case 3: return t.groove;
    case 6: return t.compress;
    case 11: return t.ghost_pattern ? (t.ghost_pattern - 1) * 16 + t.ghost_track : 0;
    case 0: return dest_index(t.dest);
    case 1: return t.chan.value_or(0);
    case 4: return t.transpose;
    case 5: return t.velocity;
    case 7: return t.loop ? int(std::lround(*t.loop)) : 0;
    case 8: return int(std::lround(t.delay * 192));
    case 9: return t.lowest;
    case 10: return t.highest;
  }
  return 0;
}

void set_param(rotator::Track& t, int p, int v) {
  switch (p) {
    case 0: t.dest = v > 0 && v <= CountTracks(g_proj) ? track_guid(GetTrack(g_proj, v - 1)) : ""; break;
    case 1: t.chan = v ? std::optional<int>(v) : std::nullopt; break;
    case 2: t.quantize = kQuant[std::clamp(v, 0, 10)]; break;
    case 3: t.groove = v; break;
    case 6: t.compress = v; break;
    case 11:
      t.ghost_pattern = v ? (v - 1) / 16 + 1 : 0;
      t.ghost_track = v ? (v - 1) % 16 + 1 : 0;
      break;
    case 4: t.transpose = v; break;
    case 5: t.velocity = v; break;
    case 7: t.loop = v > 0 ? std::optional<double>(v) : std::nullopt; break;
    case 8: t.delay = v / 192.0; break;
    case 9: t.lowest = v; break;
    case 10: t.highest = v; break;
  }
}

std::pair<int, int> param_range(int p) {
  switch (p) {
    case 0: return {0, std::max(0, CountTracks(g_proj))};
    case 1: return {0, 16};
    case 2: return {0, 10};
    case 3: return {0, rotator::kLastGroove};
    case 6: return {-2, 4};
    case 11: return {0, 99 * 16};
    case 4: return {-96, 96};
    case 5: return {-127, 127};
    case 7: return {0, 64};
    case 8: return {-768, 768};
    case 9:
    case 10: return {0, 127};
  }
  return {0, 0};
}

int param_default(int p) { return p == 10 ? 127 : 0; }

std::string param_text(int p, int v, bool list) {
  switch (p) {
    case 0: return v ? std::to_string(v) : (list ? "" : "--");
    case 1: return v ? std::to_string(v) : "orig";
    case 2: return v ? std::to_string(kQuant[v]) : (list ? "" : "768");
    case 3: return rotator::groove_name(v);
    case 6: {
      static const char* c[] = {"-1:4", "-1:2", "", "1:2", "1:4", "1:8", "fix"};
      return c[v + 2];
    }
    case 11: return v ? std::to_string((v - 1) / 16 + 1) + "." + std::to_string((v - 1) % 16 + 1) : "";
    case 4:
    case 5:
    case 7:
    case 8: return v ? std::to_string(v) : "";
    case 9: return v ? rotator::note_name(v) : "";
    case 10: return v != 127 ? rotator::note_name(v) : "";
  }
  return "";
}

// ---------- shared drawing ----------

constexpr double kRowH = 16;

void value_box(double x, double y, double w, double h, const std::string& val, const std::string& cap, bool inverse = false) {
  st::fill(x, y, w, h, inverse ? kInk : kPaper);
  st::frame(x, y, w, h);
  int col = inverse ? kPaper : kInk;
  st::text(x + (w - st::text_w(val, Font::Bold)) / 2, y + 2, val, Font::Bold, col);
  st::text_fit(x + 1, y + h - 8, w - 2, cap, col);
}

std::string bar_pos(double t) {
  int meas = 0, cml = 4, denom = 4;
  double beats = TimeMap2_timeToBeats(g_proj, t, &meas, &cml, nullptr, &denom);
  int beat = int(std::floor(beats + 1e-9));
  double frac = beats - beat;
  int per = std::max(1, 16 / std::max(1, denom));
  int six = int(std::floor(frac * per + 1e-9));
  int tick = int(std::floor((frac * per - six) * 48 + 1e-6));
  char buf[48];
  snprintf(buf, sizeof buf, "%d %2d %2d %3d", meas + 1, beat + 1, six + 1, tick);
  return buf;
}

// ---------- top area ----------

// Time signature at the cursor's bar: edits the marker there or adds one, keeping the tempo.
void set_signature(int num, int denom) {
  int meas = 0;
  double t = cursor_time();
  TimeMap2_timeToBeats(g_proj, t + 1e-6, &meas, nullptr, nullptr, nullptr);
  double bar_t = TimeMap2_beatsToTime(g_proj, 0, &meas);
  int n0 = 4, d0 = 4;
  double bpm = 120;
  TimeMap_GetTimeSigAtTime(g_proj, bar_t, &n0, &d0, &bpm);
  int idx = FindTempoTimeSigMarker(g_proj, bar_t + 1e-6);
  double mt = -1;
  if (idx >= 0) GetTempoTimeSigMarker(g_proj, idx, &mt, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  bool here = idx >= 0 && std::abs(mt - bar_t) < 1e-6;
  SetTempoTimeSigMarker(g_proj, here ? idx : -1, here ? mt : -1, here ? -1 : meas, here ? -1 : 0, bpm, num, denom, false);
  UpdateTimeline();
  commit("Rotator: signature");
}

// ---------- recording into a Rotator track ----------

struct Rec {
  bool on = false, started = false;
  int pattern = 1, track = 1, seq = -1;
  double t_start = 0;
  double last_qn = -1;
  rotator::Recorder r;
  std::vector<rotator::Event> buf;
};
Rec g_rec;

void rec_merge(Song& song, const char* desc) {
  if (g_rec.buf.empty()) return;
  auto& ev = song.patterns[g_rec.pattern].tracks[g_rec.track].events;
  ev.insert(ev.end(), g_rec.buf.begin(), g_rec.buf.end());
  std::stable_sort(ev.begin(), ev.end(), [](auto& a, auto& b) { return a.pos < b.pos; });
  g_rec.buf.clear();
  commit(desc);
}

int latest_midi_seq() {
  char buf[32];
  int sz = sizeof buf, ts = 0, dev = 0, loops = 0;
  double pp = 0;
  return MIDI_GetRecentInputEvent(0, buf, &sz, &ts, &dev, &pp, &loops);
}

// Clears the track (record replaces), counts in one bar, plays. Arrange off = cycle the entry and overdub each pass.
void rec_start(Song& song) {
  if (g_rec.on) return;
  g_rec = {};
  g_rec.on = true;
  g_rec.pattern = g_pattern;
  g_rec.track = g_track;
  double origin = 0, len = 0;
  rotator::Entry* e = selected_entry(song);
  if (e && e->pattern == g_pattern) {
    origin = bar_to_qn(e->bar);
    auto bars = rotator::entry_length(song, g_chain, g_index);
    len = bars ? bar_to_qn(e->bar + *bars) - origin : 0;
  } else if (auto pl = placements(song, g_pattern, bar_to_qn); !pl.empty()) {
    origin = pl.front().start;
    len = pl.front().end - origin;
  }
  if (len <= 0) len = std::max(bar_to_qn(5) - bar_to_qn(1), song.patterns.count(g_pattern) ? rotator::pattern_length(song.patterns[g_pattern]) : 0.0);
  g_rec.r.origin = origin;
  g_rec.r.len = len;
  g_rec.r.cycle = !g_arrange_on;
  rotator::Track& t = ensure_track(song, g_track);
  t.events.clear();
  if (MediaTrack* tr = dest_track(t.dest); tr && GetMediaTrackInfo_Value(tr, "I_RECARM") == 0) {
    SetMediaTrackInfo_Value(tr, "I_RECINPUT", 4096 | (63 << 5));
    SetMediaTrackInfo_Value(tr, "I_RECARM", 1);
    SetMediaTrackInfo_Value(tr, "I_RECMON", 1);
  }
  commit("Rotator: record (clear track)");
  if (g_rec.r.cycle) {
    double t0 = qn_time(origin), t1 = qn_time(origin + len);
    GetSet_LoopTimeRange2(g_proj, true, true, &t0, &t1, false);
    GetSetRepeatEx(g_proj, 1);
  }
  int meas = 0;
  TimeMap2_timeToBeats(g_proj, qn_time(origin) + 1e-6, &meas, nullptr, nullptr, nullptr);
  meas = std::max(0, meas - 1);
  SetEditCurPos2(g_proj, TimeMap2_beatsToTime(g_proj, 0, &meas), false, false);
  g_rec.seq = latest_midi_seq();
  g_rec.t_start = G::GetTime(g_ctx);
  Main_OnCommand(1007, 0);
}

void rec_poll(Song& song) {
  if (!g_rec.on) return;
  bool playing = GetPlayStateEx(g_proj) & 1;
  if (!g_rec.started) {
    g_rec.started = playing;
    if (!playing && G::GetTime(g_ctx) - g_rec.t_start > 2) g_rec.on = false;
    if (!playing) return;
  }
  struct Msg {
    int s, d1, d2;
    double pos;
  };
  std::vector<Msg> msgs;
  int newest = -1;
  for (int idx = 0; idx < 512; ++idx) {
    char buf[32];
    int sz = sizeof buf, ts = 0, dev = 0, loops = 0;
    double pp = -1;
    int seq = MIDI_GetRecentInputEvent(idx, buf, &sz, &ts, &dev, &pp, &loops);
    if (!seq || seq <= g_rec.seq) break;
    if (idx == 0) newest = seq;
    if (sz >= 2 && sz <= 3 && pp >= 0)
      msgs.push_back({(unsigned char)buf[0], (unsigned char)buf[1], sz > 2 ? (unsigned char)buf[2] : 0, pp});
  }
  if (newest >= 0) g_rec.seq = newest;
  // A jump back means the cycle wrapped: close the pass before anything played after the wrap.
  auto advance = [&](double qn) {
    if (g_rec.r.cycle && g_rec.last_qn >= 0 && qn + 0.5 < g_rec.last_qn) {
      g_rec.r.finish(g_rec.r.origin + g_rec.r.len - 1e-6, g_rec.buf);
      rec_merge(song, "Rotator: record pass");
    }
    g_rec.last_qn = qn;
  };
  for (auto m = msgs.rbegin(); m != msgs.rend(); ++m) {
    double qn = TimeMap2_timeToQN(g_proj, m->pos);
    advance(qn);
    g_rec.r.feed(m->s, m->d1, m->d2, qn, g_rec.buf);
  }
  if (playing) advance(TimeMap2_timeToQN(g_proj, GetPlayPositionEx(g_proj)));
  else {
    g_rec.r.finish(g_rec.last_qn, g_rec.buf);
    rec_merge(song, "Rotator: record");
    g_rec.on = false;
    if (g_rec.r.cycle && g_arrange_on) GetSetRepeatEx(g_proj, 0);
  }
}

void confirm(gem::Icon icon, const std::string& msg, const char* desc, std::function<bool(Song&)> op) {
  gem::alert(icon, msg, {"Cancel", "OK"}, 1, [desc, op](int b) {
    if (b == 1) later(desc, op);
  });
}

std::string g_file_dir;

std::string project_dir() {
  char buf[4096];
  for (int i = 0; ReaProject* p = EnumProjects(i, buf, sizeof buf); ++i)
    if (p == g_proj && *buf) return std::filesystem::path(buf).parent_path().string();
  return "";
}

void choose_file(const std::string& op, const std::string& ext, bool load, std::function<void(const std::string&)> done) {
  const char* h = getenv("HOME");
  std::string home = h ? h : "/", proj = project_dir();
  std::string dir = !g_file_dir.empty() ? g_file_dir : !proj.empty() ? proj : home + "/Documents";
  gem::file_select(op, dir, ext, load,
                   {{"Home", home}, {"Desktop", home + "/Desktop"}, {"Documents", home + "/Documents"}, {"Project", proj}},
                   [load, done](const std::string& path) {
                     g_file_dir = std::filesystem::path(path).parent_path().string();
                     std::error_code ec;
                     if (load || !std::filesystem::exists(path, ec)) return done(path);
                     gem::alert(gem::Icon::Question, std::filesystem::path(path).filename().string() + " exists.\nReplace it?",
                                {"Cancel", "OK"}, 1, [done, path](int b) {
                                  if (b == 1) done(path);
                                });
                   });
}

bool read_lines(const std::string& path, std::vector<std::string>& out) {
  std::ifstream in(path);
  for (std::string l; std::getline(in, l);) out.push_back(!l.empty() && l.back() == '\r' ? l.substr(0, l.size() - 1) : l);
  return !in.bad() && in.eof();
}

void cant(const std::string& what, const std::string& path) {
  gem::alert(gem::Icon::Note, "Can't " + what + "\n" + std::filesystem::path(path).filename().string(), {"Cancel"}, 0, nullptr);
}

void save(const std::string& path, const std::vector<std::string>& lines) {
  std::ofstream out(path);
  for (const std::string& l : lines) out << l << '\n';
  if (!out) cant("write", path);
}

void load_song(const std::string& path) {
  std::vector<std::string> lines;
  if (!read_lines(path, lines)) return cant("read", path);
  later("Rotator: load song", [song = rotator::from_lines(lines)](Song& s) {
    s = song;
    return true;
  });
}

void load_pattern(const std::string& path) {
  std::vector<std::string> lines;
  Song f = rotator::from_lines(read_lines(path, lines) ? lines : std::vector<std::string>{});
  if (f.patterns.empty()) return cant("load a pattern from", path);
  later("Rotator: load pattern", [p = f.patterns.begin()->second](Song& s) {
    rotator::Pattern& dst = s.patterns[g_pattern] = p;
    for (auto& [n, t] : dst.tracks)
      if (t.hyper_set >= int(s.hyper_sets.size())) t.hyper_set = -1;
    return true;
  });
}

void menu_bar() {
  st::fill(0, 0, 640, 19, kPaper);
  st::fill(0, 18, 640, 1, kInk);
  if (g_rec.on) {
    std::string help = "RECORD: deletes whole old track";
    st::text(320 - st::text_w(help) / 2, 1, help);
    return;
  }
  if (const std::string& op = gem::file_op(); !op.empty()) {
    std::string spaced;
    for (char c : op) spaced += spaced.empty() ? std::string(1, c) : std::string(" ") + c;
    st::text(320 - st::text_w(spaced) / 2, 1, spaced);
    return;
  }
  const char* titles[] = {"Desk", "File", "Functions", "Quantize", "MIDI", "Flags", "Options", "Edit", "Copy"};
  double x = 8, mx = 0, my = 0;
  st::mouse(&mx, &my);
  bool hover_ok = !gem::active() && !G::IsAnyMouseDown(g_ctx) && G::IsWindowHovered(g_ctx, G::HoveredFlags_AllowWhenBlockedByPopup);
  for (const char* t : titles) {
    double w = st::text_w(t) + 16;
    std::string id = std::string("##menu") + t;
    bool open = G::IsPopupOpen(g_ctx, t);
    if (st::hit(id.c_str(), x - 4, 1, w - 4, 17)) G::OpenPopup(g_ctx, t);
    else if (!open && hover_ok && mx >= x - 4 && mx < x + w - 8 && my >= 1 && my < 18) G::OpenPopup(g_ctx, t);
    if (open) st::fill(x - 4, 1, w - 4, 17, kInk);
    st::text(x, 1, t, Font::Sys, open ? kPaper : kInk);
    G::SetNextWindowPos(g_ctx, st::sx(x - 4), st::sy(18));
    if (G::BeginPopup(g_ctx, t)) {
      std::string m = t;
      if (m == "Desk") G::TextDisabled(g_ctx, "Rotator for REAPER");
      if (m == "File") {
        if (G::MenuItem(g_ctx, "Load Song...")) choose_file("Load SONG", ".ROT", true, load_song);
        if (G::MenuItem(g_ctx, "Save Song...")) choose_file("Save SONG", ".ROT", false, [](const std::string& p) { save(p, rotator::to_lines(*g_song)); });
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "Load Pattern...")) choose_file("Load PATTERN", ".PAT", true, load_pattern);
        if (G::MenuItem(g_ctx, "Save Pattern...")) choose_file("Save PATTERN", ".PAT", false, [](const std::string& p) {
          Song one;
          if (g_song->patterns.count(g_pattern)) one.patterns[g_pattern] = g_song->patterns.at(g_pattern);
          else one.patterns[g_pattern];
          save(p, rotator::to_lines(one));
        });
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "New Pattern"))
          later("Rotator: new pattern", [](Song& s) {
            g_pattern = rotator::free_pattern(s);
            s.patterns[g_pattern];
            return true;
          });
        if (G::MenuItem(g_ctx, "Delete Pattern"))
          confirm(gem::Icon::Question, "DELETE PATTERN?", "Rotator: delete pattern",
                  [](Song& s) { return s.patterns.erase(g_pattern) > 0; });
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "Capture MIDI Item"))
          later("Rotator: capture MIDI item", [](Song& s) {
            if (capture_selected_item(g_proj, ensure_track(s, g_track))) return true;
            ShowConsoleMsg("Rotator: no selected MIDI item\n");
            return false;
          });
        if (G::MenuItem(g_ctx, "Delete Track"))
          confirm(gem::Icon::Question, "DELETE TRACK?", "Rotator: delete track",
                  [](Song& s) { return s.patterns[g_pattern].tracks.erase(g_track) > 0; });
      }
      if (m == "Options") {
        if (G::MenuItem(g_ctx, "Pattern Overview")) gem::overview(g_pattern, [](int p) { g_pattern = p; });
        if (G::MenuItem(g_ctx, "Notepad")) gem::notepad([] { commit("Rotator: notepad"); });
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "Disable Transpose")) g_open_nx = true;
        if (G::MenuItem(g_ctx, "Output Filter...", nullptr, nullptr, find_track(*g_song, g_track) != nullptr)) g_open_filter = true;
        if (bool on = g_arrange_on; G::MenuItem(g_ctx, "Arrange Mode", "A", &on)) {
          g_arrange_on = !g_arrange_on;
          loop_selected(*g_song);
        }
      }
      if (m == "Edit") {
        if (G::MenuItem(g_ctx, "Event List", "E")) g_edit_page = true, g_force_tab = 1;
        if (G::MenuItem(g_ctx, "MATRIX Display", "X")) g_edit_page = true, g_force_tab = 2;
        if (G::MenuItem(g_ctx, "HYPER Display", "J")) g_edit_page = true, g_force_tab = 3;
        if (bool on = g_gam; G::MenuItem(g_ctx, "Graphic Arrange", "K", &on)) g_gam = !g_gam;
      }
      if (m == "Quantize") {
        const rotator::Track* t = find_track(*g_song, g_track);
        for (int i = 0; i < 11; ++i) {
          bool on = t && t->quantize == kQuant[i];
          std::string label = i ? "1/" + std::to_string(kQuant[i]) : "off";
          if (G::MenuItem(g_ctx, label.c_str(), nullptr, &on)) {
            set_param(ensure_track(*g_song, g_track), 2, i);
            commit("Rotator: track quantize");
          }
        }
        G::Separator(g_ctx);
        const char* modes[] = {"Musical", "Note On and Off", "Note On", "Quantize All"};
        for (int i = 0; i < 4; ++i) {
          bool on = int(g_song->quant_mode) == i;
          if (G::MenuItem(g_ctx, modes[i], nullptr, &on)) {
            g_song->quant_mode = rotator::QuantMode(i);
            commit("Rotator: quantize mode");
          }
        }
        G::SetNextItemWidth(g_ctx, 120);
        G::SliderInt(g_ctx, "Capture range", &g_song->quant_range, 0, 96, g_song->quant_range ? "%d" : "all");
        if (G::IsItemDeactivatedAfterEdit(g_ctx)) commit("Rotator: quantize range");
        G::SetNextItemWidth(g_ctx, 120);
        G::SliderInt(g_ctx, "Strength %", &g_song->quant_strength, 0, 100);
        if (G::IsItemDeactivatedAfterEdit(g_ctx)) commit("Rotator: quantize strength");
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "User Grooves...")) g_open_ugroove = true;
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "Do Quantize") && t && (t->quantize || t->groove))
          later("Rotator: do quantize", [](Song& s) {
            rotator::do_quantize(s, ensure_track(s, g_track));
            return true;
          });
      }
      if (m == "Copy") {
        bool has_e = selected_entry(*g_song);
        const char* what[] = {"Transpose", "Upbeat/Cut", "Mutes"};
        for (int w = 0; w < 3; ++w)
          for (int all = 0; all < 2; ++all) {
            std::string l = std::string("Entry ") + what[w] + (all ? " to all following" : " to next");
            if (G::MenuItem(g_ctx, l.c_str(), nullptr, nullptr, has_e)) {
              rotator::copy_entry_params(*g_song, g_chain, g_index, 1 << w, all);
              commit("Rotator: copy entry parameters");
            }
          }
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "Track to free track") && find_track(*g_song, g_track))
          later("Rotator: copy track", [](Song& s) {
            g_track = rotator::copy_track(s.patterns[g_pattern], g_track);
            return true;
          });
        if (G::MenuItem(g_ctx, "Pattern to free pattern") && g_song->patterns.count(g_pattern))
          later("Rotator: copy pattern", [](Song& s) {
            int n = rotator::free_pattern(s);
            s.patterns[n] = s.patterns[g_pattern];
            g_pattern = n;
            return true;
          });
      }
      if (m == "Functions") {
        bool has = find_track(*g_song, g_track);
        if (G::MenuItem(g_ctx, "Normalize", "N", nullptr, has))
          later("Rotator: normalize", [](Song& s) {
            rotator::normalize(ensure_track(s, g_track));
            return true;
          });
        if (G::MenuItem(g_ctx, "Fix Quantize/Groove", "F", nullptr, has))
          later("Rotator: fix quantize", [](Song& s) {
            rotator::do_quantize(s, ensure_track(s, g_track));
            return true;
          });
        if (G::BeginMenu(g_ctx, "Dynamic Groove", has)) {
          static int amount = 8;
          G::SetNextItemWidth(g_ctx, 120);
          G::SliderInt(g_ctx, "Amount", &amount, 0, 15);
          for (int k = 0; k < 16; ++k) {
            const rotator::UserGroove& ug = g_song->grooves[k];
            std::string l = "Us" + std::to_string(k + 1) + (ug.pattern ? "  " + std::to_string(ug.pattern) + "." + std::to_string(ug.track) : "");
            if (G::MenuItem(g_ctx, l.c_str(), nullptr, nullptr, ug.pattern != 0)) {
              int a = amount;
              later("Rotator: dynamic groove", [k, a](Song& s) { return rotator::dynamic_groove(s, ensure_track(s, g_track), k, a); });
            }
          }
          G::EndMenu(g_ctx);
        }
        if (G::MenuItem(g_ctx, "Make Ghost a Copy", nullptr, nullptr, has))
          later("Rotator: ghost to copy", [](Song& s) {
            rotator::Track& t = ensure_track(s, g_track);
            const rotator::Track* src = rotator::ghost_source(s, t);
            if (!src) return false;
            t.events = src->events;
            t.quantize = src->quantize, t.groove = src->groove, t.loop = src->loop;
            t.ghost_pattern = t.ghost_track = 0;
            return true;
          });
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "Clear Track", nullptr, nullptr, has))
          confirm(gem::Icon::Question, "CLEAR TRACK?", "Rotator: clear track", [](Song& s) {
            ensure_track(s, g_track).events.clear();
            return true;
          });
      }
      if (m == "MIDI") {
        const rotator::Track* t = find_track(*g_song, g_track);
        MediaTrack* tr = t ? dest_track(t->dest) : nullptr;
        bool thru = tr && GetMediaTrackInfo_Value(tr, "I_RECARM") > 0 && GetMediaTrackInfo_Value(tr, "I_RECMON") > 0;
        if (G::MenuItem(g_ctx, "MIDI Thru", nullptr, &thru, tr != nullptr)) {
          SetMediaTrackInfo_Value(tr, "I_RECARM", thru);
          SetMediaTrackInfo_Value(tr, "I_RECMON", thru);
        }
        bool click = GetToggleCommandState(40364) == 1;
        if (G::MenuItem(g_ctx, "MIDI Click", nullptr, &click)) Main_OnCommand(40364, 0);
        G::Separator(g_ctx);
        if (G::MenuItem(g_ctx, "All Notes Off")) Main_OnCommand(40345, 0);
        if (G::MenuItem(g_ctx, "Reset Controls"))
          for (int ch = 0; ch < 16; ++ch) StuffMIDIMessage(0, 0xB0 | ch, 121, 0);
      }
      if (m == "Flags") {
        G::MenuItem(g_ctx, "Catch", nullptr, &g_catch);
        if (bool on = g_arrange_on; G::MenuItem(g_ctx, "Arrange Mode", "A", &on)) {
          g_arrange_on = on;
          loop_selected(*g_song);
        }
        G::MenuItem(g_ctx, "Graphic Arrange", "K", &g_gam);
        G::Separator(g_ctx);
        G::MenuItem(g_ctx, "Mouse as Slider", nullptr, &st::slider_mode);
      }
      G::EndPopup(g_ctx);
    }
    x += w;
  }
}

void title_row(const std::string& left) {
  st::fill(0, 19, 640, 18, kPaper);
  st::fill(0, 36, 640, 1, kInk);
  st::frame(2, 21, 15, 14);
  st::text(24, 20, pad(left, 18));
  std::string title = "R O T A T O R";
  double tx = 210, tw = st::text_w(title, Font::Bold);
  st::stripes(180, 22, tx - 186, 12);
  st::text_box(tx, 20, title, Font::Bold, false);
  st::stripes(tx + tw + 6, 22, 612 - tx - tw - 6, 12);
  bool docked = G::IsWindowDocked(g_ctx);
  st::hit("##dock", 620, 21, 16, 14);
  if (G::IsItemClicked(g_ctx, G::MouseButton_Left)) g_dock_req = docked ? 2 : 1;
  if (st::hovered()) G::SetTooltip(g_ctx, docked ? "Undock" : "Dock into REAPER");
  st::fill(620, 21, 16, 14, docked ? kInk : kPaper);
  st::frame(620, 21, 16, 14);
  st::frame(623, 24, 10, 8, docked ? kPaper : kInk);
}

void box_row(Song& song) {
  st::fill(0, 37, 640, 37, kInk);
  int used = 0;
  for (auto& [n, p] : song.patterns) used += !p.tracks.empty();
  value_box(2, 41, 46, 28, std::to_string(99 - used), "FREE");

  st::hit("##arr", 52, 41, 43, 28);
  if (G::IsItemClicked(g_ctx, G::MouseButton_Left) || st::right_clicked()) {
    g_arrange_on = !g_arrange_on;
    loop_selected(song);
  }
  st::fill(52, 41, 43, 28, kPaper);
  st::frame(52, 41, 43, 28);
  if (g_arrange_on) st::text_box(65, 43, "ON", Font::Bold, true);
  else st::text(65, 43, "--", Font::Bold);
  st::text(52 + (43 - st::text_w("ARRANGE", Font::Small)) / 2, 61, "ARRANGE", Font::Small);

  value_box(98, 41, 48, 28, "", "SyncDEL");
  value_box(150, 41, 63, 28, "REAPER", "SYNC");

  double t = cursor_time();
  int num = 4, denom = 4;
  double tempo = 120;
  TimeMap_GetTimeSigAtTime(g_proj, t, &num, &denom, &tempo);
  char tb[32];
  snprintf(tb, sizeof tb, "%.4f", tempo);
  value_box(216, 41, 72, 28, "", "TEMPO");
  int bpm = int(std::lround(tempo));
  std::string tstr = tb;
  st::Value tv{&bpm, 10, 400, [tstr](int) { return tstr; }};
  if (st::value("##tempo", 218, 41, 68, 18, tv)) SetCurrentBPM(g_proj, bpm, true);

  static const int kFmt[] = {4, 8, 12, 16, 24, 32, 48, 64};
  value_box(291, 41, 38, 28, "1/" + std::to_string(kFmt[g_format]), "FORMAT");
  st::hit("##format", 291, 41, 38, 28);
  if (G::IsItemClicked(g_ctx, G::MouseButton_Left)) g_format = (g_format + 1) % 8;
  if (st::right_clicked()) g_format = (g_format + 7) % 8;
  char sig[16];
  snprintf(sig, sizeof sig, "%2d/%2d", num, denom);
  value_box(332, 41, 56, 28, sig, "SIGNATURE");
  for (int half = 0; half < 2; ++half) {
    int v = half ? denom : num;
    st::Value sv{&v, half ? 1 : 1, half ? 64 : 99, [](int) { return std::string(); }};
    G::PushID(g_ctx, half ? "sigd" : "sign");
    if (st::value("##sig", 332 + half * 28, 41, 28, 18, sv) && st::final()) {
      if (half) v = v > denom ? std::min(64, denom * 2) : std::max(1, denom / 2);
      set_signature(half ? num : v, half ? v : denom);
    }
    G::PopID(g_ctx);
  }
  int ms = int(t * 1000);
  char sw[32];
  snprintf(sw, sizeof sw, "%d %02d %02d.%03d", ms / 3600000, ms / 60000 % 60, ms / 1000 % 60, ms % 1000);
  value_box(391, 41, 112, 28, sw, "h  min  sec   ms");
  st::fill(506, 41, 8, 28, kPaper);
  st::frame(506, 41, 8, 28);
  st::stripes(507, 42, 6, 26);
  char cap[32];
  snprintf(cap, sizeof cap, "BAR  /%d  16  768", denom);
  value_box(517, 41, 103, 28, bar_pos(t), cap);
}

void header_row(Song& song) {
  st::fill(0, 74, 640, 23, kInk);
  if (st::button("##bar", 0, 78, 38, 16, "BAR", "", g_gam, Font::Small)) g_gam = !g_gam;
  st::text_box(44, 78, "ARRANGE", Font::Bold, false);
  for (int ci = 0; ci < 4; ++ci) {
    char c = rotator::kChains[ci];
    double x = 108 + ci * 19;
    G::PushID(g_ctx, std::to_string(ci).c_str());
    if (st::hit("##chain", x, 78, 17, 16)) g_view_chain = c;
    G::PopID(g_ctx);
    st::text_box(x + 4, 78, std::string(1, c), Font::Bold, c == g_view_chain);
  }
  st::header(186, 78, 27, 16, "MUTE", Font::Small);
  st::hit("##muteh", 186, 78, 27, 16);
  if (G::IsItemClicked(g_ctx, G::MouseButton_Left) || st::right_clicked()) G::OpenPopup(g_ctx, "##mutemenu");
  if (G::BeginPopup(g_ctx, "##mutemenu")) {
    rotator::Entry* e = selected_entry(song);
    if (!e) G::TextDisabled(g_ctx, "select an entry");
    else {
      bool ch = false;
      if (G::MenuItem(g_ctx, "Mute all")) {
        for (int n = 1; n <= 16; ++n) e->mutes[n] = true;
        ch = true;
      }
      if (G::MenuItem(g_ctx, "Demute all")) e->mutes.clear(), ch = true;
      if (G::MenuItem(g_ctx, "Invert")) {
        for (int n = 1; n <= 16; ++n)
          if (e->mutes.count(n) && e->mutes[n]) e->mutes.erase(n);
          else e->mutes[n] = true;
        ch = true;
      }
      if (G::MenuItem(g_ctx, "Solo selected track")) {
        e->mutes.clear();
        for (int n = 1; n <= 16; ++n)
          if (n != g_track) e->mutes[n] = true;
        ch = true;
      }
      G::Separator(g_ctx);
      for (int k = 0; k < 10; ++k) {
        G::PushID(g_ctx, std::to_string(k).c_str());
        std::string l = "Group " + std::to_string(k + 1);
        if (G::BeginMenu(g_ctx, l.c_str())) {
          if (G::MenuItem(g_ctx, "Recall", nullptr, nullptr, !song.mute_groups[k].empty())) e->mutes = song.mute_groups[k], ch = true;
          if (G::MenuItem(g_ctx, "Store")) song.mute_groups[k] = e->mutes, ch = true;
          G::EndMenu(g_ctx);
        }
        G::PopID(g_ctx);
      }
      if (ch) commit("Rotator: entry mutes");
    }
    G::EndPopup(g_ctx);
  }
  st::header(215, 78, 26, 16, "STATUS", Font::Small);

  auto p = song.patterns.find(g_pattern);
  std::string pname = p != song.patterns.end() && !p->second.name.empty() ? p->second.name : "Pattern:";
  st::fill(243, 78, 106, 16, kPaper);
  st::frame(243, 78, 106, 16);
  st::text(246, 78, pad(pname, 9), Font::Bold);
  st::hit("##pname", 243, 78, 80, 16);
  if (st::double_clicked()) start_rename("pattern", p != song.patterns.end() ? p->second.name : "");
  int pat = g_pattern;
  st::Value pv{&pat, 1, 99, nullptr};
  st::value("##patno", 323, 78, 24, 16, pv, Font::Bold);
  g_pattern = pat;
  rename_box("pattern", 246, 78, 78, [](const std::string& n) {
    g_song->patterns[g_pattern].name = n;
    commit("Rotator: pattern name");
  });
  st::header(352, 78, 68, 16, kParams[g_param].name, Font::Small);
  st::fill(423, 78, 85, 16, kPaper);
  st::frame(423, 78, 85, 16);
  st::stripes(424, 79, 83, 14);
  st::text_box(428, 78, "TRACK:" + rpad(std::to_string(g_track), 2), Font::Bold, false);
}

// ---------- arrange list ----------

void arrange_list(Song& song) {
  constexpr double X = 0, Y = 97, W = 183, H = 213;
  st::fill(X, Y, W, H, kPaper);
  st::frame(X, Y, W, H);
  double lmx = 0, lmy = 0;
  st::mouse(&lmx, &lmy);
  if (G::IsWindowHovered(g_ctx) && lmx >= X && lmx < X + W && lmy >= Y && lmy < Y + H) {
    double wh = 0, hw = 0;
    G::GetMouseWheel(g_ctx, &wh, &hw);
    if (wh) g_list_top = std::max(0, g_list_top - int(wh * 2));
  }
  auto rows = entry_rows(song);
  int vis = int((H - 4) / kRowH);
  if (g_catch && (GetPlayStateEx(g_proj) & 1)) {
    double pb = play_bar();
    auto it = song.chains.find('a');
    if (it != song.chains.end()) {
      int best = -1;
      for (int i = 0; i < int(it->second.size()); ++i)
        if (it->second[i].bar <= pb && (best < 0 || it->second[i].bar > it->second[best].bar)) best = i;
      if (best >= 0 && (g_chain != 'a' || g_index != best)) select('a', best, song, false);
    }
  }
  int sel_row = -1;
  for (int k = 0; k < int(rows.size()); ++k)
    if (std::get<1>(rows[k]) == g_chain && std::get<2>(rows[k]) == g_index) sel_row = k;
  static int last_sel = -2;
  if (sel_row != last_sel && sel_row >= 0 && (sel_row < g_list_top || sel_row >= g_list_top + vis))
    g_list_top = std::max(0, sel_row - vis / 2);
  last_sel = sel_row;
  g_list_top = std::clamp(g_list_top, 0, std::max(0, int(rows.size()) - vis));

  double colx[4];
  for (int ci = 0; ci < 4; ++ci) colx[ci] = X + 103 + ci * 20;
  for (int ci = 0; ci < 4; ++ci) st::dotted_v(colx[ci] - 2, Y + 2, H - 4);

  if (rows.size() < 6) {
    st::text(X + 40, Y + H - 48, "Rotator", Font::Bold);
    st::fill(X + 34, Y + H - 30, 70, 1, kInk);
    st::fill(X + 34, Y + H - 27, 70, 1, kInk);
    st::fill(X + 34, Y + H - 24, 70, 1, kInk);
  }

  for (int k = g_list_top; k < int(rows.size()) && k < g_list_top + vis; ++k) {
    auto [bar, c, i] = rows[k];
    rotator::Entry& e = song.chains[c][i];
    double y = Y + 2 + (k - g_list_top) * kRowH;
    bool sel = c == g_chain && i == g_index;
    G::PushID(g_ctx, ("entry" + std::to_string(k)).c_str());
    if (sel) st::fill(X + 2, y, W - 4, kRowH, kInk);
    int col = sel ? kPaper : kInk;
    st::text(X + 2, y, rpad(std::to_string(bar), 3), Font::Bold, col);
    std::string name = entry_name(song, e);
    st::text(X + 30, y, pad(name, 8), Font::Sys, col);
    if (st::hit("##row", X + 2, y, 99, kRowH)) select(c, i, song);
    if (st::double_clicked()) start_rename("entry", e.name.empty() ? name : e.name);
    for (int ci = 0; ci < 4; ++ci) {
      char to = rotator::kChains[ci];
      G::PushID(g_ctx, std::to_string(ci).c_str());
      if (to == c) {
        int pat = e.pattern;
        st::Value v{&pat, 1, 99, nullptr};
        bool ch = st::value("##pat", colx[ci], y, 18, kRowH, v, Font::Bold);
        if (sel) {
          std::string s = std::to_string(pat);
          st::fill(colx[ci], y, 18, kRowH, kInk);
          st::text(colx[ci] + 18 - st::text_w(s, Font::Bold), y, s, Font::Bold, kPaper);
        }
        if (ch) {
          e.pattern = pat;
          if (sel) g_pattern = pat;
          changed("Rotator: entry pattern");
        }
      } else if (st::hit("##mv", colx[ci], y, 18, kRowH)) {
        later("Rotator: move entry to chain", [c = c, i = i, to](Song& s) {
          select(to, rotator::move_to_chain(s, c, i, to), s);
          return true;
        });
      }
      G::PopID(g_ctx);
    }
    if (sel)
      rename_box("entry", X + 30, y, 70, [&e](const std::string& n) {
        e.name = n;
        commit("Rotator: entry name");
      });
    G::PopID(g_ctx);
  }
}

// ---------- graphic arrange ----------

struct Drag {
  char chain = 0;
  int index = -1;
  bool length = false, copy = false, keep = false;
  double y0 = 0;
};
Drag g_drag;
double g_zoom = 13;  // ST px per bar
int g_gam_first = 1;

bool same_layout(const Song& a, const Song& b) {
  for (char c : rotator::kChains) {
    auto x = a.chains.find(c), y = b.chains.find(c);
    size_t nx = x == a.chains.end() ? 0 : x->second.size(), ny = y == b.chains.end() ? 0 : y->second.size();
    if (nx != ny) return false;
    for (size_t k = 0; k < nx; ++k)
      if (x->second[k].bar != y->second[k].bar) return false;
  }
  return true;
}

void gam(Song& song) {
  constexpr double X = 0, Y = 97, W = 183, H = 213, R = 16, LW = 41;
  st::fill(X, Y, W, H, kPaper);
  st::frame(X, Y, W, H);
  if (g_drag.index >= 0) {
    auto c = song.chains.find(g_drag.chain);
    if (c == song.chains.end() || g_drag.index >= int(c->second.size())) g_drag = {};
  }
  double mx = 0, my = 0;
  G::GetMousePos(g_ctx, &mx, &my);
  double mxs = (mx - st::sx(0)) / st::scale(), mys = (my - st::sy(0)) / st::scale();
  auto lane_x = [&](int ci) { return X + R + 2 + ci * LW; };
  auto chain_at = [&](double x) { return std::clamp(int(std::floor((x - X - R - 2) / LW)), 0, 3); };
  auto y_of = [&](double bar) { return Y + 2 + (bar - g_gam_first) * g_zoom; };

  Song view;
  view.chains = song.chains;
  char sc = g_chain;
  int si = g_index;
  bool dragging = g_drag.index >= 0;
  if (dragging) {
    char c = g_drag.chain;
    int i = g_drag.index;
    int dy = int(std::lround((mys - g_drag.y0) / g_zoom));
    if (g_drag.length) {
      rotator::set_length(view, c, i, std::max(1, *rotator::entry_length(view, c, i) + dy));
    } else {
      int bar = view.chains.at(c)[i].bar + dy;
      char to = rotator::kChains[chain_at(mxs)];
      if (g_drag.copy) {
        rotator::Entry e = view.chains.at(c)[i];
        e.bar = bar;
        int k = rotator::add_entry(view, to, e);
        if (k >= 0) i = k, c = to;
      } else if (g_drag.keep && to == c) {
        rotator::move_keep_length(view, c, i, bar);
      } else {
        i = rotator::move_to_chain(view, c, i, to);
        rotator::move_start(view, to, i, bar);
        c = to;
      }
    }
    sc = c;
    si = i;
  }

  st::hit("##gam", X, Y, W, H);
  bool hovered = st::hovered();
  bool right = st::right_clicked();
  bool left_press = G::IsItemActivated(g_ctx) && G::IsMouseClicked(g_ctx, G::MouseButton_Left);
  if (hovered) {
    double wh = 0, hw = 0;
    G::GetMouseWheel(g_ctx, &wh, &hw);
    if (wh && (G::GetKeyMods(g_ctx) & G::Mod_Ctrl)) g_zoom = std::clamp(g_zoom * (wh > 0 ? 1.25 : 0.8), 4.0, 40.0);
    else if (wh) g_gam_first = std::max(-8, g_gam_first - int(wh * 2));
  }

  st::DrawClip clip(X + 1, Y + 1, W - 2, H - 2);
  int last = g_gam_first + int(H / g_zoom) + 1;
  int step = std::max(1, int(std::ceil(8 / g_zoom)));
  for (int b = g_gam_first; b <= last; ++b) {
    double y = y_of(b);
    st::dotted_h(X + R + 1, y, W - R - 2);
    if (((b - 1) % step + step) % step == 0) {
      st::fill(X + R - 3, y, 3, 1, kInk);
      std::string n = std::to_string(b);
      st::text(X + R - 3 - st::text_w(n, Font::Small), y + 1, n, Font::Small);
    }
  }
  st::fill(X + R, Y + 1, 1, H - 2, kInk);

  struct Hit {
    double x0, y0, x1, y1;
    char c;
    int i;
    bool open;
  };
  std::vector<Hit> hits;
  for (int ci = 0; ci < 4; ++ci) {
    char c = rotator::kChains[ci];
    auto it = view.chains.find(c);
    if (it == view.chains.end()) continue;
    for (int i = 0; i < int(it->second.size()); ++i) {
      const rotator::Entry& e = it->second[i];
      auto len = rotator::entry_length(view, c, i);
      Hit h{lane_x(ci), y_of(e.bar), lane_x(ci) + LW - 3, y_of(len ? e.bar + *len : last + 1), c, i, !len};
      hits.push_back(h);
      bool sel = c == sc && i == si;
      st::fill(h.x0, h.y0, h.x1 - h.x0, h.y1 - h.y0, sel ? kInk : kPaper);
      if (h.open) {
        st::fill(h.x0, h.y0, h.x1 - h.x0, 1, kInk);
        st::fill(h.x0, h.y0, 1, h.y1 - h.y0, kInk);
        st::fill(h.x1 - 1, h.y0, 1, h.y1 - h.y0, kInk);
      } else {
        st::frame(h.x0, h.y0, h.x1 - h.x0, h.y1 - h.y0);
      }
      if (e.shift < 0) st::dither(h.x0 + 1, y_of(e.bar + e.shift / 768.0), h.x1 - h.x0 - 2, (-e.shift / 768.0) * g_zoom);
      if (e.shift > 0) st::dither(h.x0 + 1, h.y0 + 1, h.x1 - h.x0 - 2, std::min(h.y1 - h.y0 - 2, e.shift / 768.0 * g_zoom), sel ? kPaper : kInk);
      int col = sel ? kPaper : kInk;
      std::string num = std::to_string(e.pattern);
      st::text(h.x1 - 1 - st::text_w(num, Font::Bold), h.y0 + 1, num, Font::Bold, col);
      st::text(h.x0 + 2, h.y0 + 3, pad(entry_name(song, e), 4), Font::Small, col);
    }
  }
  double py = y_of(play_bar());
  for (double x = X + R; x < X + W - 2; x += 4) st::fill(x, py, 2, 1, kInk);
  clip.pop();

  if (hovered && !dragging && mxs > X + R) {
    const Hit* hit = nullptr;
    for (auto& h : hits)
      if (mxs >= h.x0 && mxs < h.x1 && mys >= h.y0 && mys < h.y1 && (!hit || h.y0 >= hit->y0)) hit = &h;
    bool edge = hit && !hit->open && mys > hit->y1 - 3;
    if (edge) G::SetMouseCursor(g_ctx, G::MouseCursor_ResizeNS);
    if (hit && left_press) {
      select(hit->c, hit->i, song);
      int mods = G::GetKeyMods(g_ctx);
      g_drag = {hit->c, hit->i, edge, !edge && (mods & G::Mod_Shift), !edge && (mods & G::Mod_Alt), mys};
    }
    if (right && !hit)
      later("Rotator: add entry",
            [c = rotator::kChains[chain_at(mxs)], bar = g_gam_first + int(std::floor((mys - Y - 2) / g_zoom))](Song& s) {
              rotator::Entry e;
              e.bar = bar;
              e.pattern = g_pattern;
              int i = rotator::add_entry(s, c, e);
              if (i < 0) return false;
              select(c, i, s);
              return true;
            });
  }
  if (dragging) {
    if (g_drag.length) G::SetMouseCursor(g_ctx, G::MouseCursor_ResizeNS);
    if (!G::IsMouseDown(g_ctx, G::MouseButton_Left)) {
      if (!same_layout(view, song)) {
        song.chains = view.chains;
        select(sc, si, song);
        commit(g_drag.length ? "Rotator: entry length" : g_drag.copy ? "Rotator: copy entry" : "Rotator: move entry");
      }
      g_drag = {};
    }
  }
}

void arrange_panel(Song& song) {
  if (g_gam) gam(song);
  else arrange_list(song);

  st::fill(0, 310, 186, 70, kInk);
  if (st::button("##catch", 0, 312, 42, 18, "CATCH", "", g_catch, Font::Small)) g_catch = !g_catch;
  if (st::button("##up", 46, 312, 42, 18, "^", "F1", false, Font::Bold)) step_entry(song, -1);
  if (st::button("##down", 92, 312, 42, 18, "v", "F2", false, Font::Bold)) step_entry(song, 1);
  if (st::button("##new", 138, 312, 42, 18, "NEW", "", true, Font::Small))
    later("Rotator: add entry", [](Song& s) {
      char chain = g_view_chain;
      rotator::Entry e;
      e.bar = next_bar(s, chain);
      e.pattern = g_pattern;
      s.chains[chain].push_back(e);
      select(chain, int(s.chains[chain].size()) - 1, s);
      return true;
    });

  rotator::Entry* e = selected_entry(song);
  auto field = [](double x, double y, double w, const char* cap) {
    st::fill(x, y, w, 23, kPaper);
    st::frame(x, y, w, 23);
    st::text_fit(x + 1, y + 16, w - 2, cap);
  };
  field(0, 333, 90, "");
  if (e) {
    int sh = e->shift / 48;
    st::Value v{&sh, -64, 64, [](int n) {
      int a = std::abs(n);
      return std::string(n < 0 ? "-" : n > 0 ? "+" : " ") + std::to_string(a / 4) + "  " + std::to_string(a % 4) + "  0";
    }};
    if (st::value("##shift", 4, 333, 82, 15, v, Font::Bold, false)) {
      e->shift = sh * 48;
      changed("Rotator: entry upbeat/cut");
    }
  }
  st::text(3, 349, "UP BEAT-", Font::Small);
  st::text(87 - st::text_w("CUT+", Font::Small), 349, "CUT+", Font::Small);
  field(93, 333, 90, "TRANSPOSE");
  if (e) {
    int tr = e->transpose;
    st::Value v{&tr, -96, 96, [](int n) { return n ? std::to_string(n) : std::string(); }};
    if (st::value("##etr", 95, 333, 84, 15, v, Font::Bold)) {
      e->transpose = tr;
      changed("Rotator: entry transpose");
    }
  }
  field(0, 357, 90, "Pattern LENGTH");
  field(93, 357, 90, "Pattern POSITION");
  if (e) {
    auto len = rotator::entry_length(song, g_chain, g_index);
    if (!len) {
      st::text((90 - st::text_w("no limit", Font::Bold)) / 2, 357, "no limit", Font::Bold);
    } else {
      int bars = *len;
      st::Value v{&bars, 1, 999, [](int n) { return std::to_string(n) + "  0  0  0"; }};
      if (st::value("##elen", 4, 357, 82, 15, v, Font::Bold) && rotator::set_length(song, g_chain, g_index, bars))
        changed("Rotator: entry length");
    }
    int bar = e->bar;
    st::Value v{&bar, -9, 9999, [](int n) { return std::to_string(n) + "  1  1  1"; }};
    if (st::value("##epos", 97, 357, 82, 15, v, Font::Bold)) {
      rotator::move_start(song, g_chain, g_index, bar);
      select(g_chain, g_index, song);
      changed("Rotator: entry bar");
    }
  }
}

// ---------- track list ----------

void track_list(Song& song) {
  constexpr double Y = 97;
  int offset = int(g_view_chain - 'a') * 16;
  rotator::Entry* entry = selected_entry(song);
  st::fill(186, Y, 27, 256, kPaper);
  st::frame(186, Y, 27, 256);
  st::fill(215, Y, 26, 256, kPaper);
  st::frame(215, Y, 26, 256);
  st::fill(243, Y, 177, 256, kPaper);
  st::frame(243, Y, 177, 256);
  bool playing = GetPlayStateEx(g_proj) & 1;
  for (int n = 1; n <= 16; ++n) {
    double y = Y + (n - 1) * kRowH;
    const rotator::Track* t = find_track(song, n);
    G::PushID(g_ctx, std::to_string(n).c_str());

    bool muted = entry ? entry->mutes.count(n) && entry->mutes.at(n) : t && t->mute;
    std::string mn = std::to_string(n + offset);
    if (st::hit("##mute", 187, y, 25, kRowH)) {
      if (entry) {
        if (muted) entry->mutes.erase(n);
        else entry->mutes[n] = true;
      } else {
        ensure_track(song, n).mute = !muted;
      }
      commit("Rotator: mute");
    }
    if (muted) st::fill(188, y + 1, 23, kRowH - 2, kInk);
    st::text(186 + (27 - st::text_w(mn, Font::Small)) / 2, y + 5, mn, Font::Small, muted ? kPaper : kInk);

    bool has = t && !t->events.empty();
    bool hidden = t && t->hide;
    if (st::hit("##status", 216, y, 24, kRowH) && t) {
      ensure_track(song, n).hide = !hidden;
      commit("Rotator: track hide");
    }
    bool lit = has && !(playing && int(G::GetTime(g_ctx) * 8 + n * 5) % 3 == 0);
    if (lit) st::fill(218, y + 2, 20, kRowH - 4, kInk);
    if (hidden) st::text(224, y, "*", Font::Bold, lit ? kPaper : kInk);

    bool sel = n == g_track;
    if (sel) st::fill(244, y, 175, kRowH, kInk);
    int col = sel ? kPaper : kInk;
    st::text(244, y, rpad(std::to_string(n), 2), Font::Bold, col);
    if (t) st::text(261, y, pad(t->name, 8), Font::Bold, col);
    if (t) st::text(330, y + 5, pad(dest_name(t->dest), 8), Font::Small, col);
    if (t) {
      std::string v = param_text(g_param, get_param(*t, g_param), true);
      st::text(418 - st::text_w(v, Font::Bold), y, v, Font::Bold, col);
    }
    if (st::hit("##row", 244, y, 175, kRowH)) g_track = n;
    if (st::double_clicked()) start_rename("track" + std::to_string(n), t ? t->name : "");
    if (st::right_clicked()) {
      g_track = n;
      G::OpenPopup(g_ctx, "##destmenu");
    }
    if (G::BeginDragDropSource(g_ctx)) {
      G::SetDragDropPayload(g_ctx, "NTR_TRACK", std::to_string(n).c_str());
      G::Text(g_ctx, ("track " + std::to_string(n) + (t ? " " + t->name : "")).c_str());
      G::EndDragDropSource(g_ctx);
    }
    if (G::BeginDragDropTarget(g_ctx)) {
      char buf[8] = "";
      if (G::AcceptDragDropPayload(g_ctx, "NTR_TRACK", buf, sizeof buf) && std::atoi(buf) != n) {
        int src = std::atoi(buf);
        if (t) {
          gem::alert(gem::Icon::Stop, "Exchange or Merge\nthese tracks?", {"CANCEL", "EXCHANGE", "MERGE"}, 2, [src, n](int b) {
            if (b == 1)
              later("Rotator: exchange tracks", [src, n](Song& s) {
                rotator::exchange_tracks(s.patterns[g_pattern], src, n);
                return true;
              });
            if (b == 2)
              later("Rotator: merge tracks", [src, n](Song& s) {
                rotator::merge_tracks(s.patterns[g_pattern], src, n);
                g_track = n;
                return true;
              });
          });
        } else {
          later("Rotator: copy track", [src, n](Song& s) {
            auto& tr = s.patterns[g_pattern].tracks;
            if (!tr.count(src)) return false;
            tr[n] = tr[src];
            g_track = n;
            return true;
          });
        }
      }
      G::EndDragDropTarget(g_ctx);
    }
    rename_box("track" + std::to_string(n), 261, y, 66, [n](const std::string& name) {
      ensure_track(*g_song, n).name = name;
      g_track = n;
      commit("Rotator: track name");
    });
    G::PopID(g_ctx);
  }
  st::dotted_v(242, Y, 256);

  if (G::BeginPopup(g_ctx, "##destmenu")) {
    G::TextDisabled(g_ctx, ("Track " + std::to_string(g_track) + " output").c_str());
    if (G::MenuItem(g_ctx, "none")) {
      ensure_track(song, g_track).dest.clear();
      commit("Rotator: track destination");
    }
    for (int i = 0; i < CountTracks(g_proj); ++i) {
      char name[256] = "";
      GetTrackName(GetTrack(g_proj, i), name, sizeof name);
      if (G::MenuItem(g_ctx, (std::to_string(i + 1) + ": " + name).c_str())) {
        ensure_track(song, g_track).dest = track_guid(GetTrack(g_proj, i));
        commit("Rotator: track destination");
      }
    }
    G::EndPopup(g_ctx);
  }

  st::fill(186, 353, 237, 27, kInk);
  const char* lamps[] = {"MIDI THRU", "OVERFLOW", "SMPTE"};
  bool midi = G::GetTime(g_ctx) - g_midi_in_t < 0.25;
  for (int k = 0; k < 3; ++k) {
    double x = 215 + k * 69;
    st::fill(x, 357, 66, 22, kPaper);
    st::frame(x, 357, 66, 22);
    st::fill(x + 2, 359, 62, 9, kInk);
    if (k == 0 && midi) st::fill(x + 4, 361, 58, 5, kPaper);
    st::text(x + (66 - st::text_w(lamps[k], Font::Small)) / 2, 371, lamps[k], Font::Small);
  }
}

void param_box(Song& song) {
  constexpr double X = 423, Y = 97, W = 85, RH = 14;
  st::fill(X - 2, 94, W + 4, 286, kInk);
  st::fill(X, Y, W, kNumParams * RH + 4, kPaper);
  st::frame(X, Y, W, kNumParams * RH + 4);
  const rotator::Track* t = find_track(song, g_track);
  for (int p = 0; p < kNumParams; ++p) {
    double y = Y + 2 + p * RH;
    G::PushID(g_ctx, std::to_string(p).c_str());
    bool sel = p == g_param;
    if (st::hit("##pn", X + 1, y, 44, RH) && kParams[p].live) g_param = p;
    if (kParams[p].live) {
      int v = t ? get_param(*t, p) : param_default(p);
      auto [lo, hi] = param_range(p);
      st::Value val{&v, lo, hi, [p](int n) { return param_text(p, n, false); }, param_default(p)};
      int before = v;
      bool ch = st::value("##pv", X + 46, y, W - 49, RH, val, Font::Bold);
      if (ch) {
        if (v != before) set_param(ensure_track(song, g_track), p, v);
        changed("Rotator: track parameter");
      }
      if (sel) {
        std::string s = param_text(p, v, false);
        st::fill(X + 1, y, W - 2, RH, kInk);
        st::text(X + W - 3 - st::text_w(s, Font::Bold), y - 1, s, Font::Bold, kPaper);
      }
    }
    int col = sel ? kPaper : kInk;
    st::text(X + 3, y + 4, kParams[p].name, Font::Small, col);
    if (!kParams[p].live)
      for (double c = X + 3; c < X + 3 + st::text_w(kParams[p].name, Font::Small); c += 2) st::fill(c, y + 5, 1, 1, kPaper);
    G::PopID(g_ctx);
  }

  double by = Y + kNumParams * RH + 10;
  if (st::button("##rmg", X, by, 32, 20, "RMG", "", false, Font::Small)) g_rmg = true;
  if (st::button("##edit", X + 36, by, 47, 20, "EDIT", "", false, Font::Bold)) g_edit_page = true, g_force_tab = 1;
  if (st::button("##doq", X, by + 27, 83, 18, "DO QUANTIZE", "", false, Font::Small) && t && t->quantize)
    later("Rotator: do quantize", [](Song& s) {
      rotator::do_quantize(s, ensure_track(s, g_track));
      return true;
    });
  bool solo = song.solo_track == g_track && song.solo_chain == g_view_chain;
  if (st::button("##solo", X, by + 52, 40, 18, "SOLO", "", solo, Font::Small)) {
    song.solo_chain = solo ? 0 : g_view_chain;
    song.solo_track = solo ? 0 : g_track;
    commit("Rotator: solo");
  }
  if (st::button("##hide", X + 43, by + 52, 40, 18, "HIDE", "", t && t->hide, Font::Small) && t) {
    ensure_track(song, g_track).hide = !t->hide;
    commit("Rotator: track hide");
  }
  if (st::button("##default", X, by + 77, 83, 18, "DEFAULT", "", false, Font::Small) && t)
    later("Rotator: default track parameters", [](Song& s) {
      rotator::Track& tr = ensure_track(s, g_track);
      rotator::Track d;
      d.name = tr.name;
      d.dest = tr.dest;
      d.events = tr.events;
      d.hyper_set = tr.hyper_set;
      tr = d;
      return true;
    });
}

// ---------- locators and transport ----------

int loc_bar(double t) {
  int meas = 0;
  TimeMap2_timeToBeats(g_proj, t + 1e-6, &meas, nullptr, nullptr, nullptr);
  return meas + 1;
}

void transport(Song& song) {
  constexpr double X = 513;
  st::fill(X - 3, 74, 640 - X - 18 + 3, 306, kInk);
  double t0 = 0, t1 = 0;
  GetSet_LoopTimeRange2(g_proj, false, true, &t0, &t1, false);
  const char* caps[] = {"LEFT LOCATOR", "RIGHT LOCATOR"};
  for (int k = 0; k < 2; ++k) {
    double y = 78 + k * 34;
    st::fill(X, y, 107, 30, kPaper);
    st::frame(X, y, 107, 30);
    st::stripes(X + 1, y + 20, 105, 9);
    st::text_box(X + (107 - st::text_w(caps[k], Font::Small)) / 2, y + 22, caps[k], Font::Small, false);
    int bar = loc_bar(k ? t1 : t0);
    st::Value v{&bar, 1, 9999, [](int n) { return std::to_string(n) + "  1  1  1"; }};
    G::PushID(g_ctx, std::to_string(k).c_str());
    if (st::value("##loc", X + 4, y + 2, 99, 16, v, Font::Bold)) {
      int m = bar - 1;
      double nt = TimeMap2_beatsToTime(g_proj, 0, &m);
      if (k) t1 = std::max(nt, t0);
      else t0 = nt, t1 = std::max(t1, nt);
      GetSet_LoopTimeRange2(g_proj, true, true, &t0, &t1, false);
    }
    G::PopID(g_ctx);
  }
  bool autodrop = GetToggleCommandState(40076) == 1;
  st::hit("##autodrop", X, 148, 51, 24);
  if (G::IsItemClicked(g_ctx, G::MouseButton_Left) || st::right_clicked()) Main_OnCommand(autodrop ? 40252 : 40076, 0);
  value_box(X, 148, 51, 24, autodrop ? "ON" : "--", "AUTODROP", autodrop);
  bool rep = GetSetRepeatEx(g_proj, -1) > 0;
  st::hit("##cycle", X + 56, 148, 51, 24);
  if (G::IsItemClicked(g_ctx, G::MouseButton_Left) || st::right_clicked()) GetSetRepeatEx(g_proj, rep ? 0 : 1);
  value_box(X + 56, 148, 51, 24, rep ? "ON" : "--", "CYCLE", rep);

  int ps = GetPlayStateEx(g_proj);
  bool playing = ps & 1, paused = ps & 2, recording = ps & 4;
  if (st::button("##drop", X, 182, 50, 28, "DROP", "SPACE", false, Font::Bold) && !recording) {
    if (!autodrop) Main_OnCommand(40076, 0);
    int m = std::max(0, loc_bar(t0) - 2);
    SetEditCurPos2(g_proj, TimeMap2_beatsToTime(g_proj, 0, &m), true, false);
    Main_OnCommand(1013, 0);
  }
  if (st::button("##undo", X + 55, 182, 50, 28, "UNDO", "", false, Font::Bold)) Main_OnCommand(40029, 0);
  if (st::button("##punch", X, 220, 50, 28, "PUNCH", "/", false, Font::Bold) && playing && !recording) {
    if (autodrop) Main_OnCommand(40252, 0);
    Main_OnCommand(1013, 0);
  }
  if (st::button("##rec", X + 55, 220, 50, 28, "RECORD", "*", recording || g_rec.on, Font::Bold)) {
    if (g_rec.on) Main_OnCommand(1016, 0);
    else rec_start(song);
  }
  if (st::button("##rw", X, 260, 31, 26, "<<", "(", false, Font::Bold)) step_bar(-1);
  if (st::button("##ff", X + 35, 260, 31, 26, ">>", ")", false, Font::Bold)) step_bar(1);
  if (st::button("##prev", X, 292, 31, 30, "<<<", "[", false, Font::Bold)) step_entry(song, -1);
  if (st::button("##next", X + 35, 292, 31, 30, ">>>", "]", false, Font::Bold)) step_entry(song, 1);
  if (st::button("##stop", X + 70, 260, 35, 62, "STOP", "Enter", !playing && !paused, Font::Bold)) Main_OnCommand(1016, 0);
  if (st::button("##start", X, 336, 62, 34, "START", "0", playing && !paused, Font::Bold)) {
    Main_OnCommand(40042, 0);
    Main_OnCommand(1007, 0);
  }
  if (st::button("##cont", X + 66, 336, 39, 34, "CONT", ".", paused, Font::Bold)) Main_OnCommand(playing ? 1008 : 1007, 0);
}

void gem_chrome() {
  st::fill(622, 37, 18, 343, kPaper);
  st::frame(622, 37, 18, 343);
  st::frame(622, 37, 18, 18);
  st::text(627, 38, "^", Font::Bold);
  st::frame(622, 344, 18, 18);
  st::text(627, 345, "v", Font::Bold);
  st::dots(624, 57, 14, 285);
  st::fill(0, 380, 640, 20, kPaper);
  st::frame(0, 380, 640, 20);
  st::frame(622, 380, 18, 20);
  st::frame(626, 384, 10, 12);
}

// ---------- dialogs ----------

void user_groove_popup(Song& song) {
  const char* title = "USER GROOVES";
  if (g_open_ugroove) G::OpenPopup(g_ctx, title), g_open_ugroove = false;
  if (!G::BeginPopupModal(g_ctx, title, nullptr, G::WindowFlags_AlwaysAutoResize)) return;
  G::TextDisabled(g_ctx, "Template = any track; its note starts are the groove grid (looped by its LOOP).");
  if (G::BeginTable(g_ctx, "ug", 6, G::TableFlags_BordersInnerH | G::TableFlags_SizingFixedFit)) {
    for (const char* h : {"", "PATTERN", "TRACK", "RANGE", "STRENGTH", ""}) G::TableSetupColumn(g_ctx, h);
    G::TableHeadersRow(g_ctx);
    bool ch = false;
    for (int k = 0; k < 16; ++k) {
      rotator::UserGroove& g = song.grooves[k];
      G::PushID(g_ctx, std::to_string(k).c_str());
      G::TableNextRow(g_ctx);
      G::TableNextColumn(g_ctx);
      G::Text(g_ctx, ("Us" + std::to_string(k + 1)).c_str());
      int* f[] = {&g.pattern, &g.track, &g.range, &g.strength};
      int hi[] = {99, 16, 96, 100};
      for (int c = 0; c < 4; ++c) {
        G::TableNextColumn(g_ctx);
        G::SetNextItemWidth(g_ctx, 60);
        G::DragInt(g_ctx, ("##f" + std::to_string(c)).c_str(), f[c], 0.2, 0, hi[c], c == 2 && !g.range ? "all" : "%d");
        ch |= G::IsItemDeactivatedAfterEdit(g_ctx);
      }
      G::TableNextColumn(g_ctx);
      if (G::SmallButton(g_ctx, "this track")) g.pattern = g_pattern, g.track = g_track, ch = true;
      G::PopID(g_ctx);
    }
    G::EndTable(g_ctx);
    if (ch) commit("Rotator: user groove");
  }
  if (G::Button(g_ctx, "OK") || G::IsKeyPressed(g_ctx, G::Key_Escape)) G::CloseCurrentPopup(g_ctx);
  G::EndPopup(g_ctx);
}

void filter_popup(Song& song) {
  const char* title = "OUTPUT FILTER";
  if (g_open_filter) G::OpenPopup(g_ctx, title), g_open_filter = false;
  if (!G::BeginPopupModal(g_ctx, title, nullptr, G::WindowFlags_AlwaysAutoResize)) return;
  rotator::Track& t = ensure_track(song, g_track);
  G::Text(g_ctx, ("Track " + std::to_string(g_track) + " " + t.name + " sends:").c_str());
  const char* names[] = {"NOTES", "CONTROL", "PROGRAM", "PITCH W", "C PRESS", "P PRESS"};
  bool ch = false;
  for (int k = 0; k < 6; ++k) {
    bool pass = !(t.filter >> k & 1);
    if (G::Checkbox(g_ctx, names[k], &pass)) {
      t.filter ^= 1 << k;
      ch = true;
    }
  }
  G::Separator(g_ctx);
  ch |= G::Checkbox(g_ctx, "Channel filter: only events on the track's CHANNEL", &t.chan_filter);
  if (ch) commit("Rotator: output filter");
  if (G::Button(g_ctx, "OK") || G::IsKeyPressed(g_ctx, G::Key_Enter) || G::IsKeyPressed(g_ctx, G::Key_Escape))
    G::CloseCurrentPopup(g_ctx);
  G::EndPopup(g_ctx);
}

void no_transpose_popup(Song& song) {
  const char* title = "DISABLE ARRANGE TRANSPOSE (CHANNELS)";
  if (g_open_nx) G::OpenPopup(g_ctx, title), g_open_nx = false;
  if (!G::BeginPopupModal(g_ctx, title, nullptr, G::WindowFlags_AlwaysAutoResize)) return;
  std::vector<std::string> dests;
  for (auto& [_, p] : song.patterns)
    for (auto& [_, t] : p.tracks)
      if (!t.dest.empty() && std::find(dests.begin(), dests.end(), t.dest) == dests.end()) dests.push_back(t.dest);
  if (dests.empty()) G::TextDisabled(g_ctx, "no outputs in use");
  for (auto& d : dests) {
    G::PushID(g_ctx, d.c_str());
    G::AlignTextToFramePadding(g_ctx);
    G::Text(g_ctx, pad(dest_name(d), 10).c_str());
    for (int ch = 1; ch <= 16; ++ch) {
      G::SameLine(g_ctx, ch == 1 ? 110 : 0);
      bool off = song.no_arrange_transpose.count({d, ch});
      if (G::Selectable(g_ctx, (std::to_string(ch) + "##nx").c_str(), &off, 0, 22, 0)) {
        if (off) song.no_arrange_transpose.insert({d, ch});
        else song.no_arrange_transpose.erase({d, ch});
        commit("Rotator: disable arrange transpose");
      }
    }
    G::PopID(g_ctx);
  }
  if (G::Button(g_ctx, "OK") || G::IsKeyPressed(g_ctx, G::Key_Enter)) G::CloseCurrentPopup(g_ctx);
  G::EndPopup(g_ctx);
}

// ---------- pages ----------

double page_scale() {
  double w = 0, h = 0;
  G::GetContentRegionAvail(g_ctx, &w, &h);
  double s = std::min(w / 640, h / 400);
  return std::max(1.0, std::floor(s * 4) / 4);
}

std::string project_name() {
  char buf[512] = "";
  GetProjectName(g_proj, buf, sizeof buf);
  std::string s = buf;
  if (auto dot = s.rfind('.'); dot != std::string::npos) s.resize(dot);
  for (char& c : s) c = char(std::toupper((unsigned char)c));
  return s.empty() ? "UNTITLED.SON" : s + ".SON";
}

double g_scale = 2;

void main_page(Song& song) {
  st::begin(g_ctx, g_scale);
  st::fill(0, 0, 640, 400, kInk);
  title_row(project_name());
  box_row(song);
  header_row(song);
  arrange_panel(song);
  track_list(song);
  param_box(song);
  transport(song);
  gem_chrome();
  menu_bar();
  st::end(640, 400);
}


// ---------- realtime MIDI generator ----------

int g_rmg_cc = 7;
int g_rmg_val[16] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
int g_rmg_snap[8][16];
bool g_rmg_snap_set[8] = {};
int g_rmg_drag = -1;

const char* cc_label(int cc) {
  switch (cc) {
    case 1: return "Modulation";
    case 7: return "Volume";
    case 10: return "Pan";
    case 11: return "Expression";
    case 64: return "Sustain";
    case 91: return "Reverb";
    case 93: return "Chorus";
  }
  return "";
}

// Sends the fader; while REAPER plays, also writes it into the edited track at the play position.
void rmg_send(Song& song, int ch, int v) {
  StuffMIDIMessage(0, 0xB0 | ch, g_rmg_cc, v);
  if (!(GetPlayStateEx(g_proj) & 1)) return;
  double qn = TimeMap2_timeToQN(g_proj, GetPlayPositionEx(g_proj));
  auto pl = placements(song, g_pattern, bar_to_qn);
  const rotator::Placement* p = placement_at(pl, qn);
  if (!p) return;
  rotator::Track& t = ensure_track(song, g_track);
  double pos = qn - p->start - t.delay;
  if (t.loop && *t.loop > 0) pos = std::fmod(std::max(0.0, pos), *t.loop);
  t.events.push_back({rotator::EventType::CC, std::max(0.0, std::round(pos * 192) / 192), ch + 1, g_rmg_cc, v});
  std::stable_sort(t.events.begin(), t.events.end(), [](auto& a, auto& b) { return a.pos < b.pos; });
}

void rmg_page(Song& song) {
  st::begin(g_ctx, g_scale);
  st::fill(0, 0, 640, 400, kInk);
  const rotator::Track* t = find_track(song, g_track);
  title_row(std::to_string(g_track) + " " + (t ? t->name : "* New *"));
  box_row(song);
  menu_bar();

  st::fill(4, 78, 612, 18, kPaper);
  st::frame(4, 78, 612, 18);
  st::stripes(5, 79, 610, 16);
  st::text_box(320 - st::text_w("Realtime MIDI Generator", Font::Bold) / 2, 79, "Realtime MIDI Generator", Font::Bold, false);

  auto row = [](double y, const char* label, const std::string& val) {
    st::fill(4, y, 160, 18, kPaper);
    st::frame(4, y, 160, 18);
    st::text(8, y + 6, label, Font::Small);
    st::fill(70, y + 1, 1, 16, kInk);
    st::text(76, y + 1, val, Font::Bold);
  };
  row(102, "STATUS", "Control");
  row(124, "NUMBER", "");
  st::Value nv{&g_rmg_cc, 0, 127, [](int n) { return std::to_string(n) + " " + cc_label(n); }};
  st::value("##cc", 76, 124, 86, 18, nv, Font::Bold, false);
  row(146, "TRACK", std::to_string(g_track) + " " + (t ? pad(t->name, 8) : ""));
  bool playing = GetPlayStateEx(g_proj) & 1;
  row(168, "WRITE", playing ? "while play" : "-- stopped");
  if (st::button("##delmov", 4, 196, 160, 20, "DELETE MOVEMENTS", "", false, Font::Small) && t)
    later("Rotator: delete movements", [](Song& s) {
      auto& ev = ensure_track(s, g_track).events;
      auto n = ev.size();
      ev.erase(std::remove_if(ev.begin(), ev.end(), [](auto& e) { return e.type == rotator::EventType::CC && e.data1 == g_rmg_cc; }), ev.end());
      return ev.size() != n;
    });
  if (st::button("##copyall", 4, 222, 160, 20, "COPY TO ALL", "", false, Font::Small))
    for (int ch = 1; ch < 16; ++ch) g_rmg_val[ch] = g_rmg_val[0];

  constexpr double FX = 176, FY = 102, FW = 26, FH = 200;
  double mx = 0, my = 0;
  st::mouse(&mx, &my);
  for (int ch = 0; ch < 16; ++ch) {
    double x = FX + ch * (FW + 2);
    G::PushID(g_ctx, std::to_string(ch).c_str());
    st::fill(x, FY, FW, FH + 34, kPaper);
    st::frame(x, FY, FW, FH + 34);
    st::fill(x + 9, FY + 4, 8, FH - 8, kInk);
    st::dotted_v(x + 7, FY + 4, FH - 8);
    st::dotted_v(x + 18, FY + 4, FH - 8);
    double ky = FY + 4 + (FH - 8) * (1 - g_rmg_val[ch] / 127.0);
    st::fill(x + 9, ky, 8, FY + FH - 4 - ky, kPaper);
    st::frame(x + 9, FY + 4, 8, FH - 8);
    st::fill(x + 3, ky - 3, 20, 6, kInk);
    st::fill(x + 5, ky - 1, 16, 2, kPaper);
    std::string v = std::to_string(g_rmg_val[ch]);
    st::text(x + (FW - st::text_w(v, Font::Small)) / 2, FY + FH + 2, v, Font::Small);
    std::string n = std::to_string(ch + 1);
    st::fill(x + 2, FY + FH + 14, FW - 4, 16, kInk);
    st::text(x + (FW - st::text_w(n, Font::Bold)) / 2, FY + FH + 14, n, Font::Bold, kPaper);
    if (st::hit("##fader", x, FY, FW, FH)) g_rmg_drag = ch;
    G::PopID(g_ctx);
  }
  if (g_rmg_drag >= 0) {
    if (!G::IsMouseDown(g_ctx, G::MouseButton_Left)) {
      g_rmg_drag = -1;
      if (playing) commit("Rotator: RMG movements");
    } else {
      int v = std::clamp(int(std::lround((FY + FH - 4 - my) / (FH - 8) * 127)), 0, 127);
      if (v != g_rmg_val[g_rmg_drag]) {
        g_rmg_val[g_rmg_drag] = v;
        rmg_send(song, g_rmg_drag, v);
      }
    }
  }

  constexpr double SY = 344;
  st::fill(176, SY, 446, 30, kPaper);
  st::frame(176, SY, 446, 30);
  st::text(180, SY + 3, "SNAPSHOTS   CLICK LEFT: RECALL   RIGHT: SAVE", Font::Small);
  for (int k = 0; k < 8; ++k) {
    double x = 180 + k * 54;
    G::PushID(g_ctx, ("snap" + std::to_string(k)).c_str());
    bool left = st::hit("##snap", x, SY + 12, 50, 15);
    if (st::right_clicked()) {
      std::copy(g_rmg_val, g_rmg_val + 16, g_rmg_snap[k]);
      g_rmg_snap_set[k] = true;
    }
    if (left && g_rmg_snap_set[k]) {
      std::copy(g_rmg_snap[k], g_rmg_snap[k] + 16, g_rmg_val);
      for (int ch = 0; ch < 16; ++ch) rmg_send(song, ch, g_rmg_val[ch]);
      if (playing) commit("Rotator: RMG snapshot");
    }
    st::fill(x, SY + 12, 50, 15, g_rmg_snap_set[k] ? kInk : kPaper);
    st::frame(x, SY + 12, 50, 15);
    std::string n = std::to_string(k + 1);
    st::text(x + 21, SY + 12, n, Font::Bold, g_rmg_snap_set[k] ? kPaper : kInk);
    G::PopID(g_ctx);
  }
  if (st::button("##rmgexit", 4, 354, 160, 20, "EXIT", "", false, Font::Bold)) g_rmg = false;
  st::end(640, 400);
}

void edit_page(Song& song) {
  st::begin(g_ctx, g_scale);
  st::fill(0, 0, 640, 98, kInk);
  const rotator::Track* t = find_track(song, g_track);
  title_row(std::to_string(g_track) + " " + (t ? t->name : "* New *"));
  box_row(song);
  if (st::button("##exit", 2, 77, 60, 18, "EXIT", "E", false, Font::Small)) g_edit_page = false;
  const char* tabs[] = {"EVENT LIST", "MATRIX", "HYPER"};
  static int tab = 0;
  if (g_force_tab) tab = g_force_tab - 1;
  g_force_tab = 0;
  for (int k = 0; k < 3; ++k) {
    G::PushID(g_ctx, std::to_string(k).c_str());
    if (st::button("##tab", 70 + k * 84, 77, 80, 18, tabs[k], "", tab == k, Font::Small)) tab = k;
    G::PopID(g_ctx);
  }
  st::text_box(330, 78, "Pattern:" + rpad(std::to_string(g_pattern), 3) + "   TRACK:" + rpad(std::to_string(g_track), 2),
               Font::Bold, false);
  menu_bar();
  st::end(640, 98);
  G::PushStyleVar(g_ctx, G::StyleVar_WindowPadding, 0, 0);
  G::PushStyleVar(g_ctx, G::StyleVar_ItemSpacing, 0, 0);
  bool vis = G::BeginChild(g_ctx, "editpage", 640 * g_scale, 0, 0, G::WindowFlags_NoScrollbar | G::WindowFlags_NoScrollWithMouse);
  G::PopStyleVar(g_ctx, 2);
  if (!vis) {
    G::EndChild(g_ctx);
    return;
  }
  rotator::Track& trk = song.patterns[g_pattern].tracks[g_track];
  EditCtx ec{g_ctx, g_proj, song, g_pattern, g_track, trk, [](const char* d) { commit(d); }};
  if (tab == 0) event_editor(ec);
  else if (tab == 1) matrix_editor(ec);
  else hyper_editor(ec);
  G::EndChild(g_ctx);
}

int g_style_colors = 0, g_style_vars = 0;

void push_style() {
  auto col = [](int idx, int c) {
    G::PushStyleColor(g_ctx, idx, c);
    ++g_style_colors;
  };
  const int grey = 0xC8C8C8FF;
  col(G::Col_WindowBg, kPaper);
  col(G::Col_ChildBg, kPaper);
  col(G::Col_PopupBg, kPaper);
  col(G::Col_Text, kInk);
  col(G::Col_TextDisabled, 0x707070FF);
  col(G::Col_Border, kInk);
  col(G::Col_FrameBg, kPaper);
  col(G::Col_FrameBgHovered, kPaper);
  col(G::Col_FrameBgActive, grey);
  col(G::Col_Button, kPaper);
  col(G::Col_ButtonHovered, kPaper);
  col(G::Col_ButtonActive, grey);
  col(G::Col_Header, grey);
  col(G::Col_HeaderHovered, grey);
  col(G::Col_HeaderActive, grey);
  col(G::Col_Tab, kPaper);
  col(G::Col_TabHovered, grey);
  col(G::Col_TabSelected, grey);
  col(G::Col_TitleBg, kPaper);
  col(G::Col_TitleBgActive, kPaper);
  col(G::Col_TableHeaderBg, grey);
  col(G::Col_TableRowBg, kPaper);
  col(G::Col_TableRowBgAlt, kPaper);
  col(G::Col_TableBorderStrong, kInk);
  col(G::Col_TableBorderLight, kInk);
  col(G::Col_CheckMark, kInk);
  col(G::Col_SliderGrab, kInk);
  col(G::Col_SliderGrabActive, kInk);
  col(G::Col_ScrollbarBg, kPaper);
  col(G::Col_ScrollbarGrab, kInk);
  col(G::Col_ScrollbarGrabHovered, kInk);
  col(G::Col_ScrollbarGrabActive, kInk);
  col(G::Col_Separator, kInk);
  col(G::Col_ModalWindowDimBg, 0x00000060);
  auto var = [](int idx, double v) {
    G::PushStyleVar(g_ctx, idx, v);
    ++g_style_vars;
  };
  var(G::StyleVar_WindowRounding, 0);
  var(G::StyleVar_ChildRounding, 0);
  var(G::StyleVar_FrameRounding, 0);
  var(G::StyleVar_PopupRounding, 0);
  var(G::StyleVar_GrabRounding, 0);
  var(G::StyleVar_TabRounding, 0);
  var(G::StyleVar_ScrollbarRounding, 0);
  var(G::StyleVar_FrameBorderSize, 1);
  var(G::StyleVar_PopupBorderSize, 1);
}

void pop_style() {
  G::PopStyleColor(g_ctx, g_style_colors);
  G::PopStyleVar(g_ctx, g_style_vars);
  g_style_colors = g_style_vars = 0;
}

void poll_midi_lamp() {
  char buf[32];
  int sz = sizeof buf, ts = 0, dev = 0, loops = 0;
  double pp = 0;
  int seq = MIDI_GetRecentInputEvent(0, buf, &sz, &ts, &dev, &pp, &loops);
  if (g_midi_seq >= 0 && seq != g_midi_seq) g_midi_in_t = G::GetTime(g_ctx);
  g_midi_seq = seq;
}

void shortcuts(Song& song) {
  if (!G::IsWindowFocused(g_ctx, G::FocusedFlags_RootAndChildWindows) || G::IsAnyItemActive(g_ctx) ||
      !g_rename_key.empty() || G::GetKeyMods(g_ctx))
    return;
  auto key = [](int k) { return G::IsKeyPressed(g_ctx, k, false); };
  if (key(G::Key_K)) g_gam = !g_gam;
  if (key(G::Key_M)) g_rmg = !g_rmg;
  if (key(G::Key_Escape)) g_rmg = false;
  if (key(G::Key_E)) g_edit_page = !g_edit_page, g_force_tab = g_edit_page ? 1 : 0;
  if (key(G::Key_X)) g_edit_page = true, g_force_tab = 2;
  if (key(G::Key_J)) g_edit_page = true, g_force_tab = 3;
  if (key(G::Key_A)) {
    g_arrange_on = !g_arrange_on;
    loop_selected(song);
  }
  if (key(G::Key_LeftBracket)) step_entry(song, -1);
  if (key(G::Key_RightBracket)) step_entry(song, 1);
  if (key(G::Key_Keypad0)) {
    Main_OnCommand(40042, 0);
    Main_OnCommand(1007, 0);
  }
  if (key(G::Key_KeypadEnter)) Main_OnCommand(1016, 0);
  if (key(G::Key_KeypadDecimal)) Main_OnCommand(1008, 0);
  if (key(G::Key_KeypadMultiply)) {
    if (g_rec.on) Main_OnCommand(1016, 0);
    else rec_start(song);
  }
  if (!g_edit_page) {
    if (key(G::Key_UpArrow)) g_track = std::max(1, g_track - 1);
    if (key(G::Key_DownArrow)) g_track = std::min(16, g_track + 1);
    if (key(G::Key_Delete) && selected_entry(song))
      later("Rotator: delete entry", [](Song& s) {
        rotator::delete_close_gap(s, g_chain, g_index);
        g_chain = 0;
        return true;
      });
  }
}

}  // namespace

void ui_init(void* (*get_func)(const char*)) { g_get_func = get_func; }
bool g_rec_req = false;
void ui_record() { g_rec_req = true; }
void ui_set_open(bool open) { g_open = open; }
bool ui_is_open() { return g_open; }
void ui_set_gam(bool gam) { g_gam = gam; }
void ui_set_track(int t) { g_track = std::clamp(t, 1, 16); }
void ui_set_tab(int t) {  // 0 main, 1-3 editors, 4 RMG
  g_rmg = t == 4;
  g_force_tab = t < 4 ? t : 0;
  g_edit_page = t > 0 && t < 4;
}

void ui_frame(ReaProject* proj, Song& song) {
  if (!g_open) return;
  g_proj = proj;
  g_song = &song;
  try {
    if (!g_api) {
      G::init(g_get_func);
      g_api = true;
    }
    if (!g_ctx || !G::ValidatePtr(g_ctx, "ImGui_Context*")) {
      g_ctx = G::CreateContext("Rotator");
      st::attach_fonts(g_ctx);
    }
    if (!selected_entry(song)) g_chain = 0;
    poll_midi_lamp();
    if (g_rec_req) rec_start(song);
    g_rec_req = false;
    rec_poll(song);
    st::set_inverse(g_rec.on);
    G::SetNextWindowSize(g_ctx, 1300, 860, G::Cond_FirstUseEver);
    if (g_dock_req) G::SetNextWindowDockID(g_ctx, g_dock_req == 1 ? -1 : 0);
    g_dock_req = 0;
    push_style();
    G::PushFont(g_ctx, st::sys_font(), 16);
    G::PushStyleColor(g_ctx, G::Col_WindowBg, kInk);
    bool open = true;
    bool visible = G::Begin(g_ctx, "Rotator", &open, G::WindowFlags_NoScrollbar | G::WindowFlags_NoScrollWithMouse);
    G::PopStyleColor(g_ctx);
    if (visible) {
      bool modal = gem::active();
      if (!modal) shortcuts(song);
      g_scale = page_scale();
      double px = 0, py = 0;
      G::GetCursorScreenPos(g_ctx, &px, &py);
      G::PushStyleVar(g_ctx, G::StyleVar_DisabledAlpha, 1);
      G::BeginDisabled(g_ctx, modal);
      if (g_rmg) rmg_page(song);
      else if (g_edit_page) edit_page(song);
      else main_page(song);
      G::EndDisabled(g_ctx);
      G::PopStyleVar(g_ctx);
      no_transpose_popup(song);
      user_groove_popup(song);
      filter_popup(song);
      if (gem::active()) {
        G::SetCursorScreenPos(g_ctx, px, py);
        st::begin(g_ctx, g_scale);
        gem::frame(g_ctx, song);
      }
      G::End(g_ctx);
    }
    G::PopFont(g_ctx);
    pop_style();
    g_open = open;
    if (g_op && g_op(song)) commit(g_op_desc);
    g_op = nullptr;
  } catch (const ImGui_Error& e) {
    g_error = e.what();
    ShowConsoleMsg(("Rotator: ImGui error: " + g_error + "\n").c_str());
    g_open = false;
    g_ctx = nullptr;
    g_op = nullptr;
    g_style_colors = g_style_vars = 0;
  }
}

const char* ui_status() {
  static std::string s;
  s = std::string("ui open=") + (g_open ? "1" : "0") +
      " ctx=" + (g_ctx && G::ValidatePtr(g_ctx, "ImGui_Context*") ? "valid" : "none") + " pattern=" +
      std::to_string(g_pattern) + " track=" + std::to_string(g_track) + " view=" + (g_gam ? "gam" : "list") +
      " page=" + (g_rmg ? "rmg" : g_edit_page ? "edit" : "main") + " origin=" + std::to_string(int(st::sx(0))) + "," +
      std::to_string(int(st::sy(0))) + " scale=" + std::to_string(st::scale()) +
      " drag=" + st::debug_state() + " rename=" + (g_rename_key.empty() ? "-" : g_rename_key) + " sel=" + (g_chain ? std::string(1, g_chain) + std::to_string(g_index) : "-") +
      (g_error.empty() ? "" : " error=" + g_error) + "\n";
  return s.c_str();
}
