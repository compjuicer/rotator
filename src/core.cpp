#define REAPERAPI_IMPLEMENT
#include "reaper_core_api.h"
#include "reaper_out.h"
#include "song_io.h"
#include "core_abi.h"
#include "ui.h"
#include "lineparse.h"

#include <cinttypes>
#include <cstring>
#include <map>
#include <sstream>

namespace {

std::map<ReaProject*, rotator::Song> g_songs;

bool empty(const rotator::Song& s) { return s.patterns.empty() && s.chains.empty(); }

ReaProject* current() { return EnumProjects(-1, nullptr, 0); }

ReaProject* load_save_project() {
  ReaProject* p = GetCurrentProjectInLoadSave();
  return p ? p : current();
}

bool first_token_is(const char* line, const char* tok) {
  LineParser lp;
  return !lp.parse(line) && lp.getnumtokens() > 0 && !strcmp(lp.gettoken_str(0), tok);
}

std::string trim(const char* s) {
  std::string t = s;
  size_t b = t.find_first_not_of(" \t\r\n"), e = t.find_last_not_of(" \t\r\n");
  return b == std::string::npos ? "" : t.substr(b, e - b + 1);
}

bool config_line(const char* line, ProjectStateContext* ctx) {
  if (!first_token_is(line, "<ROTATOR")) return false;
  std::vector<std::string> lines;
  char buf[4096];
  while (ctx->GetLine(buf, sizeof(buf)) >= 0 && !first_token_is(buf, ">")) lines.push_back(trim(buf));
  g_songs[load_save_project()] = rotator::from_lines(lines);
  return true;
}

void config_save(ProjectStateContext* ctx) {
  auto it = g_songs.find(load_save_project());
  if (it == g_songs.end() || empty(it->second)) return;
  ctx->AddLine("<ROTATOR");
  for (auto& l : rotator::to_lines(it->second)) ctx->AddLine("%s", l.c_str());
  ctx->AddLine(">");
}

void config_begin_load() { g_songs.erase(load_save_project()); }

rotator::Event n(double pos, int pitch, double len = 0.25, int vel = 100) {
  return {rotator::EventType::Note, pos, 1, pitch, vel, len};
}

rotator::Track track(const char* name, const std::string& dest, std::vector<rotator::Event> events) {
  rotator::Track t;
  t.name = name;
  t.dest = dest;
  t.events = std::move(events);
  return t;
}

rotator::Song test_song() {
  while (CountTracks(nullptr) < 2) InsertTrackAtIndex(CountTracks(nullptr), true);
  std::string drums = track_guid(GetTrack(nullptr, 0));
  std::string bass = track_guid(GetTrack(nullptr, 1));

  rotator::Song song;

  auto& groove = song.patterns[1];
  groove.name = "Groove";
  groove.tracks[1] = track("Kick", drums, {n(0, 36)});
  groove.tracks[1].chan = 10;
  groove.tracks[1].loop = 1;
  groove.tracks[2] = track("Hat", drums, {n(0, 42, 0.1, 80), n(0.5, 42, 0.1, 60)});
  groove.tracks[2].chan = 10;
  groove.tracks[2].loop = 0.75;
  groove.tracks[3] = track("Bass", bass, {n(0, 36, 1.5), n(2, 39, 0.5), n(3, 41, 0.5), n(4, 36, 1.5), n(6, 43, 0.5), n(7, 41, 0.5)});

  auto& brk = song.patterns[2];
  brk.name = "Break";
  brk.tracks[1] = track("Kick", drums, {n(0, 36)});
  brk.tracks[1].chan = 10;
  brk.tracks[3] = track("Bass", bass, {n(0, 36, 4)});

  auto& a = song.chains['a'];
  a.resize(4);
  a[0].bar = 1, a[0].pattern = 1;
  a[1].bar = 5, a[1].pattern = 1, a[1].transpose = 5, a[1].mutes[2] = true;
  a[2].bar = 7, a[2].pattern = 2;
  a[3].bar = 8, a[3].pattern = 1;

  return song;
}

void load_test_song() {
  ReaProject* proj = current();
  g_songs[proj] = test_song();
  commit(proj, g_songs[proj], "Rotator: load test song");
}

void render() {
  auto it = g_songs.find(current());
  if (it == g_songs.end() || empty(it->second)) {
    ShowConsoleMsg("Rotator: no song in this project\n");
    return;
  }
  render_to_project(it->second);
}

const RotatorAction kActions[] = {
    {"ROTATOR_LOAD_TEST", "Rotator: Load test song"},
    {"ROTATOR_RENDER", "Rotator: Render"},
    {"ROTATOR_WINDOW", "Rotator: Show window"},
};

void run_action(int index) {
  if (index == 0) load_test_song();
  else if (index == 1) render();
  else if (index == 2) ui_set_open(!ui_is_open());
}

// Debug hook for the bridge: SetExtState("Rotator", "cmd",
//   "open" | "close" | "gam" | "list" | "tab <1-3>" | "track <1-16>" | "capture <pattern> <track>" | "add <chain> <bar> <pattern>").
void debug_command() {
  const char* cmd = GetExtState("Rotator", "cmd");
  if (!cmd || !*cmd) return;
  std::string c = cmd;
  DeleteExtState("Rotator", "cmd", false);
  int p = 0, t = 0;
  char ch = 0;
  if (c == "open" || c == "close") ui_set_open(c == "open");
  else if (c == "gam" || c == "list") ui_set_gam(c == "gam");
  else if (sscanf(c.c_str(), "tab %d", &t) == 1) ui_set_tab(t);
  else if (c == "rec") ui_record();
  else if (sscanf(c.c_str(), "track %d", &t) == 1) ui_set_track(t);
  else if (sscanf(c.c_str(), "add %c %d %d", &ch, &t, &p) == 3) {
    ReaProject* proj = current();
    rotator::Entry e;
    e.bar = t;
    e.pattern = p;
    if (rotator::add_entry(g_songs[proj], ch, e) >= 0) commit(proj, g_songs[proj], "Rotator: add entry");
  }
  else if (sscanf(c.c_str(), "capture %d %d", &p, &t) == 2) {
    ReaProject* proj = current();
    if (capture_selected_item(proj, g_songs[proj].patterns[p].tracks[t])) commit(proj, g_songs[proj], "Rotator: capture MIDI item");
  }
}

void tick() {
  debug_command();
  if (ui_is_open()) {
    ReaProject* proj = current();
    ui_frame(proj, g_songs[proj]);
  }
}

std::string g_ret;

const char* snapshot() {
  std::string s = ui_is_open() ? "!window\n" : "";
  for (auto& [proj, song] : g_songs) {
    s += "@" + std::to_string(reinterpret_cast<uintptr_t>(proj)) + "\n";
    for (auto& l : rotator::to_lines(song)) s += l + "\n";
  }
  return (g_ret = s).c_str();
}

void restore(const char* text) {
  g_songs.clear();
  std::istringstream in(text);
  std::string line;
  ReaProject* proj = nullptr;
  std::map<ReaProject*, std::vector<std::string>> lines;
  while (std::getline(in, line)) {
    if (line == "!window") ui_set_open(true);
    else if (line[0] == '@') lines[proj = reinterpret_cast<ReaProject*>(std::stoull(line.substr(1)))];
    else if (proj) lines[proj].push_back(line);
  }
  for (auto& [p, l] : lines) g_songs[p] = rotator::from_lines(l);
}

const char* dump() {
  ReaProject* proj = current();
  char buf[4096];
  auto add = [&](const char* fmt, auto... args) {
    snprintf(buf, sizeof(buf), fmt, args...);
    g_ret += buf;
  };
  g_ret = ui_status();
  for (int t = 0; t < CountTracks(nullptr); ++t) {
    MediaTrack* tr = GetTrack(nullptr, t);
    char name[256] = "";
    GetTrackName(tr, name, sizeof(name));
    add("track %d \"%s\" %s\n", t, name, track_guid(tr).c_str());
  }
  for (int t = 0; t < CountTracks(nullptr); ++t) {
    MediaTrack* tr = GetTrack(nullptr, t);
    for (int i = 0; i < CountTrackMediaItems(tr); ++i) {
      MediaItem* item = GetTrackMediaItem(tr, i);
      double pos = GetMediaItemInfo_Value(item, "D_POSITION"), len = GetMediaItemInfo_Value(item, "D_LENGTH");
      double qn = TimeMap2_timeToQN(proj, pos), len_qn = TimeMap2_timeToQN(proj, pos + len) - qn;
      MediaItem_Take* take = GetActiveTake(item);
      char tag[64] = "";
      GetSetMediaItemInfo_String(item, "P_EXT:rotator", tag, false);
      add("item track=%d pos_qn=%.4f pos_s=%.4f len_qn=%.4f len_s=%.4f take=\"%s\" tag=%s\n", t, qn, pos, len_qn, len,
          take ? GetTakeName(take) : "", tag[0] ? tag : "-");
      int notes = 0, ccs = 0;
      if (take) MIDI_CountEvts(take, &notes, &ccs, nullptr);
      for (int n = 0; n < notes; ++n) {
        double s = 0, e = 0;
        int chan = 0, pitch = 0, vel = 0;
        MIDI_GetNote(take, n, nullptr, nullptr, &s, &e, &chan, &pitch, &vel);
        double sq = MIDI_GetProjQNFromPPQPos(take, s);
        add("note track=%d qn=%.4f len_qn=%.4f pitch=%d vel=%d chan=%d\n", t, sq, MIDI_GetProjQNFromPPQPos(take, e) - sq,
            pitch, vel, chan + 1);
      }
      for (int n = 0; n < ccs; ++n) {
        double s = 0;
        int chanmsg = 0, chan = 0, msg2 = 0, msg3 = 0;
        MIDI_GetCC(take, n, nullptr, nullptr, &s, &chanmsg, &chan, &msg2, &msg3);
        add("cc track=%d qn=%.4f status=0x%02X chan=%d msg2=%d msg3=%d\n", t, MIDI_GetProjQNFromPPQPos(take, s), chanmsg,
            chan + 1, msg2, msg3);
      }
    }
  }
  auto it = g_songs.find(proj);
  if (it != g_songs.end())
    for (auto& l : rotator::to_lines(it->second)) g_ret += "song " + l + "\n";
  return g_ret.c_str();
}

void shutdown() {
  plugin_register("-timer", (void*)tick);
  g_songs.clear();
}

const RotatorCore kCore = {
    int(sizeof(kActions) / sizeof(kActions[0])), kActions, run_action, config_line, config_save, config_begin_load,
    snapshot, restore, dump, __DATE__ " " __TIME__, shutdown,
};

}  // namespace

extern "C" __attribute__((visibility("default"))) const RotatorCore* rotator_core_init(void* (*get_func)(const char*)) {
  if (REAPERAPI_LoadAPI(get_func)) return nullptr;
  ui_init(get_func);
  plugin_register("timer", (void*)tick);
  return &kCore;
}
