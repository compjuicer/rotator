#include "reaper_core_api.h"
#include "reaper_out.h"
#include "editor.h"
#include "ed_event_ops.h"
#include "st.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#undef min
#undef max

namespace G = ImGui;
using namespace rotator;
using st::Font, st::kInk, st::kPaper;

namespace {

constexpr int kFormats[] = {4, 8, 12, 16, 24, 32, 48, 64};
constexpr EventType kTypes[] = {EventType::Note,      EventType::CC,           EventType::Program,
                                EventType::PitchBend, EventType::ChanPressure, EventType::PolyPressure};

struct Sounding {
  int chan, pitch;
  double off;
};

struct RowY {
  double y0, y1;
  int i;
};

struct State {
  int pattern = -1, track = -1;
  int cur = 0;
  Col col = Col::D1;
  bool insert = false, catch_on = false, midi_out = true, chord = false, midi_in = false, step = false;
  bool graphic = true, global = false, drums = false;
  bool show[6] = {true, true, true, true, true, true};
  bool scroll = false, was_playing = false, open_req = false;
  int top = 0;
  ImGui_Context* ctx = nullptr;
  struct {
    int i = -1;
    Col col = Col::Bar;
    double y0 = 0;
    int last = 0;
    bool moved = false, was = false, dbl = false;
    std::vector<Event> orig;
  } drag;
  const char* pending = nullptr;
  std::optional<Event> clip;
  StepInput steps;
  int seq = -1;
  std::vector<Sounding> sounding;
  std::string text;
  Col text_col = Col::D1;
  bool open_text = false;
  int lasso = -1, lasso_to = -1;
  double lasso_x0 = 0, lasso_w = 1, lasso_len = 0;
  std::vector<RowY> rows_y;
  int drop_type = -1, drop_chan = 1;
  double drop_pos = 0;
};
State S;

struct Frame {
  EditCtx& c;
  std::vector<Event>& ev;
  Grid g;
  const Placement* ref = nullptr;
  int bar_offset = 0;
  bool playing = false;
  double now = 0;
};

bool shown(int i, const std::vector<Event>& ev) { return i == S.cur || S.show[int(ev[i].type)]; }

void stuff(int s, int d1, int d2) { StuffMIDIMessage(0, s, d1, d2); }

void notes_off(double now, bool all) {
  for (auto it = S.sounding.begin(); it != S.sounding.end();)
    if (all || now >= it->off) {
      stuff(0x80 | it->chan, it->pitch, 0);
      it = S.sounding.erase(it);
    } else {
      ++it;
    }
}

void send(const Frame& f, const Event& e) {
  const Track& t = f.c.trk;
  int ch = (t.chan.value_or(e.chan) - 1) & 15;
  switch (e.type) {
    case EventType::Note: {
      int p = std::clamp(e.data1 + t.transpose, 0, 127);
      stuff(0x90 | ch, p, std::clamp(e.data2 + t.velocity, 1, 127));
      S.sounding.push_back({ch, p, f.now + std::clamp(e.len * 0.5, 0.1, 1.0)});
      break;
    }
    case EventType::CC: stuff(0xB0 | ch, e.data1, e.data2); break;
    case EventType::Program: stuff(0xC0 | ch, e.data1, 0); break;
    case EventType::PitchBend: stuff(0xE0 | ch, get_field(e, Col::D1), get_field(e, Col::D2)); break;
    case EventType::ChanPressure: stuff(0xD0 | ch, e.data2, 0); break;
    case EventType::PolyPressure: stuff(0xA0 | ch, e.data1, e.data2); break;
  }
}

void audition(const Frame& f, int i) {
  if (!S.midi_out || i < 0 || i >= int(f.ev.size())) return;
  notes_off(f.now, true);
  const Event& e = f.ev[i];
  if (S.chord && e.type == EventType::Note) {
    for (const Event& o : f.ev)
      if (o.type == EventType::Note && to_pulses(o.pos) == to_pulses(e.pos)) send(f, o);
  } else {
    send(f, e);
  }
}

void finish(Frame& f, const char* desc) {
  S.cur = sort_events(f.ev, S.cur);
  f.c.commit(desc);
  audition(f, S.cur);
}

void move_cursor(Frame& f, int i) {
  S.cur = i;
  S.scroll = true;
  S.catch_on = false;
  if (i < int(f.ev.size())) S.steps.pos = f.ev[i].pos;
}

std::string pitch_label(const Frame& f, MediaTrack* tr, const Event& e) {
  if (tr) {
    const char* n = GetTrackMIDINoteNameEx(f.c.proj, tr, e.data1, e.chan - 1);
    if (n && *n) return n;
  }
  return note_name(e.data1);
}

// Layout in ST pixels.
constexpr double kRowH = 16, kHead = 11, kLeftW = 60, kGfxX = 62, kGfxW = 108, kRightX = 580;
struct ColX {
  double x, w;
};
constexpr ColX kCols[] = {{174, 32}, {206, 24}, {230, 24}, {254, 32}, {294, 64}, {358, 24}, {382, 40}, {422, 32}, {462, 104}};
ColX col_x(Col c) { return kCols[int(c)]; }

void cell_text(double x, double w, double y, const std::string& s, bool right, int col) {
  double tw = st::text_w(s, Font::Bold) + 1;
  st::DrawClip clip(x, y, w + 1, kRowH);
  st::text(right ? x + w - tw : x, y, s, Font::Bold, col);
  clip.pop();
}

bool apply_step(Frame& f, int d, bool fine) {
  Event& e = f.ev[S.cur];
  if (S.col <= Col::Pulse) {
    move_event(f.ev, S.cur, step_pos(e.pos, S.col, d, f.g), S.insert);
  } else if (S.col == Col::Status) {
    if (e.type == EventType::Note) return false;
    int t = ((int(e.type) - 1 + d) % 5 + 5) % 5;
    set_status(e, kTypes[t + 1]);
  } else if (S.col == Col::Len) {
    if (!has_field(e.type, Col::Len)) return false;
    set_length(f.ev, S.cur, std::max(from_pulses(1), e.len + d * from_pulses(fine ? 1 : f.g.unit_p())));
  } else if (has_field(e.type, S.col)) {
    set_field(e, S.col, get_field(e, S.col) + d);
  } else {
    return false;
  }
  return true;
}

void drop_target(const Event& e) {
  if (!G::BeginDragDropTarget(S.ctx)) return;
  char buf[4] = "";
  if (G::AcceptDragDropPayload(S.ctx, "NTR_EVTYPE", buf, sizeof buf)) S.drop_type = buf[0] - '0', S.drop_pos = e.pos, S.drop_chan = e.chan;
  G::EndDragDropTarget(S.ctx);
}

void open_text(Frame& f);

// Click selects, click on the selected cell +1, right click -1, vertical drag steps, double click types.
void cell(Frame& f, int i, Col col, double y) {
  ImGui_Context* ctx = f.c.ctx;
  ColX c = col_x(col);
  G::PushID(ctx, std::to_string(int(col)).c_str());
  st::hit("##c", c.x - 2, y, c.w + 4, kRowH);
  drop_target(f.ev[i]);
  double mx, my;
  st::mouse(&mx, &my);
  bool sel = i == S.cur && col == S.col;
  if (G::IsItemActivated(ctx) && G::IsMouseDown(ctx, G::MouseButton_Left)) {
    move_cursor(f, i);
    S.scroll = false;
    S.col = col;
    S.drag = {i, col, my, 0, false, sel, G::IsMouseDoubleClicked(ctx, G::MouseButton_Left), f.ev};
  }
  if (S.drag.i == i && S.drag.col == col && G::IsItemActive(ctx)) {
    int d = -int(std::lround((my - S.drag.y0) / 4));
    if (std::abs(my - S.drag.y0) > 2) S.drag.moved = true;
    if (S.drag.moved && d != S.drag.last) {
      f.ev = S.drag.orig;
      S.cur = i;
      apply_step(f, d, G::GetKeyMods(ctx) & G::Mod_Shift);
      S.drag.last = d;
    }
  }
  if (S.drag.i == i && S.drag.col == col && G::IsItemDeactivated(ctx)) {
    if (S.drag.moved) {
      if (S.drag.last) S.pending = "Rotator: edit event";
    } else if (S.drag.dbl) {
      S.open_req = true;
    } else if (S.drag.was && apply_step(f, 1, false)) {
      S.pending = "Rotator: edit event";
    }
    S.drag = {};
  }
  if (st::right_clicked()) {
    if (sel && apply_step(f, -1, false)) S.pending = "Rotator: edit event";
    move_cursor(f, i);
    S.scroll = false;
    S.col = col;
  }
  G::PopID(ctx);
}

void graphic(Frame& f, int i, double y) {
  ImGui_Context* ctx = f.c.ctx;
  const Event& e = f.ev[i];
  double bar_qn = from_pulses(f.g.bar_p());
  bool lassoed = S.lasso >= 0 && e.type == EventType::Note && i >= std::min(S.lasso, S.lasso_to) &&
                 i <= std::max(S.lasso, S.lasso_to);
  double len = e.type == EventType::Note ? (lassoed ? S.lasso_len : e.len) : 0;
  double x0 = kGfxX + std::fmod(e.pos, bar_qn) / bar_qn * kGfxW;
  double x1 = std::min(kGfxX + kGfxW, x0 + std::max(2.0, len / bar_qn * kGfxW));
  if (lassoed) {
    st::fill(x0, y + 4, x1 - x0, kRowH - 8, kPaper);
    st::stripes(x0, y + 4, x1 - x0, kRowH - 8);
    st::frame(x0, y + 4, x1 - x0, kRowH - 8);
  } else {
    st::fill(x0, y + (e.type == EventType::Note ? 4 : 6), x1 - x0, e.type == EventType::Note ? kRowH - 8 : kRowH - 12, kInk);
  }
  S.rows_y.push_back({y, y + kRowH, i});

  G::PushID(ctx, "g");
  st::hit("##gfx", kGfxX, y, kGfxW, kRowH);
  drop_target(e);
  double mx, my;
  st::mouse(&mx, &my);
  bool on_end = e.type == EventType::Note && std::abs(mx - x1) <= 3;
  if (G::IsItemClicked(ctx)) {
    move_cursor(f, i);
    S.scroll = false;
    if (on_end) {
      S.lasso = S.lasso_to = i;
      S.lasso_x0 = x0;
      S.lasso_w = kGfxW;
      S.lasso_len = e.len;
    }
  }
  if (on_end && st::hovered()) G::SetMouseCursor(ctx, G::MouseCursor_ResizeEW);
  G::PopID(ctx);
}

void lasso_update(Frame& f) {
  if (S.lasso < 0) return;
  double mx, my;
  st::mouse(&mx, &my);
  G::SetMouseCursor(f.c.ctx, G::MouseCursor_ResizeEW);
  double bar_qn = from_pulses(f.g.bar_p());
  S.lasso_len = from_pulses(std::max(1, to_pulses((mx - S.lasso_x0) / S.lasso_w * bar_qn)));
  for (const RowY& r : S.rows_y) {
    if (my >= r.y0) S.lasso_to = r.i;
    if (my < r.y1) break;
  }
  if (S.lasso_to < S.lasso) S.lasso_to = S.lasso;
  if (!G::IsMouseDown(f.c.ctx, G::MouseButton_Left)) {
    if (S.lasso < int(f.ev.size())) {
      set_lengths(f.ev, S.lasso, std::min(S.lasso_to, int(f.ev.size()) - 1), S.lasso_len);
      S.cur = S.lasso;
      f.c.commit("Rotator: event lengths");
    }
    S.lasso = -1;
  }
}

void row(Frame& f, int i, double y, MediaTrack* drums) {
  ImGui_Context* ctx = f.c.ctx;
  Event& e = f.ev[i];
  G::PushID(ctx, std::to_string(i).c_str());
  if (S.graphic) graphic(f, i, y);
  bool cur = i == S.cur;
  if (cur) st::fill(col_x(Col::Bar).x - 4, y, col_x(Col::Pulse).x + col_x(Col::Pulse).w + 6 - col_x(Col::Bar).x, kRowH, kInk);
  Bbup b = to_bbup(e.pos, f.g);
  int pos[] = {b.bar + f.bar_offset, b.beat, b.unit, b.pulse};
  for (Col col : {Col::Bar, Col::Beat, Col::Unit, Col::Pulse, Col::Status, Col::Chan, Col::D1, Col::D2, Col::Len}) {
    ColX c = col_x(col);
    bool sel = cur && col == S.col;
    int ink = kInk;
    if (sel && col <= Col::Pulse) st::fill(c.x - 1, y + 1, c.w + 2, kRowH - 2, kPaper);
    else if (sel) st::fill(c.x - 2, y, c.w + 4, kRowH, kInk), ink = kPaper;
    else if (cur && col <= Col::Pulse) ink = kPaper;
    std::string s;
    bool right = true;
    if (col <= Col::Pulse) s = std::to_string(pos[int(col)]);
    else if (col == Col::Status) s = e.type == EventType::Note ? "NOTE" : status_name(e.type), right = false;
    else if (col == Col::Len && e.type == EventType::Note) {
      std::vector<std::string> parts;
      std::string t = format_len(e.len, f.g);
      for (size_t a = 0, z; a < t.size(); a = z + 1) {
        z = t.find(' ', a);
        if (z == std::string::npos) z = t.size();
        parts.push_back(t.substr(a, z - a));
      }
      double x = c.x + c.w;
      for (int k = int(parts.size()) - 1, n = 0; k >= 0; --k, ++n) {
        cell_text(x - (n ? 24 : 32), n ? 24 : 32, y, parts[k], true, ink);
        x -= n ? 24 : 32;
      }
    } else if (col == Col::Len && e.type == EventType::CC) {
      s = cc_name(e.data1), right = false;
    } else if (has_field(e.type, col)) {
      if (col == Col::D1 && (e.type == EventType::Note || e.type == EventType::PolyPressure)) s = pitch_label(f, drums, e);
      else s = std::to_string(get_field(e, col));
    }
    if (!s.empty()) cell_text(c.x, c.w, y, s, right, ink);
    cell(f, i, col, y);
  }
  G::PopID(ctx);
}

void open_text(Frame& f) {
  const Event& e = f.ev[S.cur];
  if (S.col <= Col::Pulse) S.text = format_pos(e.pos, f.g, f.bar_offset);
  else if (S.col == Col::Len && e.type == EventType::Note) S.text = format_len(e.len, f.g);
  else if (S.col == Col::D1 && (e.type == EventType::Note || e.type == EventType::PolyPressure))
    S.text = note_name(e.data1);
  else if (S.col != Col::Status && has_field(e.type, S.col)) S.text = std::to_string(get_field(e, S.col));
  else {
    return;
  }
  S.text_col = S.col;
  S.open_text = true;
}

void text_popup(Frame& f) {
  ImGui_Context* ctx = f.c.ctx;
  if (S.open_text) G::OpenPopup(ctx, "##evtext");
  S.open_text = false;
  if (!G::BeginPopup(ctx, "##evtext")) return;
  if (S.cur >= int(f.ev.size())) {
    G::CloseCurrentPopup(ctx);
    G::EndPopup(ctx);
    return;
  }
  const char* label = S.text_col <= Col::Pulse ? "Position (bar beat unit pulse)"
                      : S.text_col == Col::Len ? "Length ([bar] beat unit pulse, or pulses)"
                      : S.text.size() && std::isalpha((unsigned char)S.text[0]) ? "Pitch (E4 or 76)"
                                                                               : "Value";
  G::Text(ctx, label);
  char buf[64];
  snprintf(buf, sizeof buf, "%s", S.text.c_str());
  if (G::IsWindowAppearing(ctx)) G::SetKeyboardFocusHere(ctx);
  G::SetNextItemWidth(ctx, 200);
  bool enter = G::InputText(ctx, "##t", buf, sizeof buf, G::InputTextFlags_EnterReturnsTrue | G::InputTextFlags_AutoSelectAll);
  S.text = buf;
  if (enter) {
    Event& e = f.ev[S.cur];
    bool ok = false;
    if (S.text_col <= Col::Pulse) {
      std::string t = S.text;
      if (f.bar_offset) {
        size_t n = 0;
        try {
          int bar = std::stoi(t, &n) - f.bar_offset;
          t = std::to_string(bar) + t.substr(n);
        } catch (...) {
        }
      }
      if (auto p = parse_pos(t, f.g)) move_event(f.ev, S.cur, *p, S.insert), ok = true;
    } else if (S.text_col == Col::Len) {
      if (auto l = parse_len(S.text, f.g)) set_length(f.ev, S.cur, *l), ok = true;
    } else if (S.text_col == Col::D1 && (e.type == EventType::Note || e.type == EventType::PolyPressure)) {
      if (auto p = parse_note(S.text)) e.data1 = *p, ok = true;
    } else {
      try {
        set_field(e, S.text_col, std::stoi(S.text));
        ok = true;
      } catch (...) {
      }
    }
    if (ok) finish(f, "Rotator: edit event");
    G::CloseCurrentPopup(ctx);
  }
  G::EndPopup(ctx);
}

void step_value(Frame& f, int d) {
  if (apply_step(f, d, false)) finish(f, "Rotator: edit event");
}

int new_chan(const Frame& f) { return f.ev.empty() || S.cur >= int(f.ev.size()) ? 1 : f.ev[S.cur].chan; }
double new_pos(const Frame& f) { return f.ev.empty() || S.cur >= int(f.ev.size()) ? 0 : f.ev[S.cur].pos; }

void insert(Frame& f, Event e, const char* desc) {
  S.cur = insert_event(f.ev, e);
  S.scroll = true;
  f.c.commit(desc);
  audition(f, S.cur);
}

void erase(Frame& f) {
  if (S.cur >= int(f.ev.size())) return;
  f.ev.erase(f.ev.begin() + S.cur);
  S.cur = std::max(0, std::min(S.cur, int(f.ev.size()) - 1));
  f.c.commit("Rotator: delete event");
}

double rel_pos(const Frame& f, double qn) {
  double rel = qn - f.ref->start - f.c.trk.delay;
  if (f.c.trk.loop && *f.c.trk.loop > 0) rel = std::fmod(std::max(0.0, rel), *f.c.trk.loop);
  return rel;
}

void keys(Frame& f, const std::vector<int>& rows, double play_qn) {
  ImGui_Context* ctx = f.c.ctx;
  int mods = G::GetKeyMods(ctx);
  bool shift = mods & G::Mod_Shift, alt = mods & G::Mod_Alt, cmd = mods & G::Mod_Ctrl;
  auto pressed = [&](int k) { return G::IsKeyPressed(ctx, k); };
  bool has = S.cur < int(f.ev.size());

  if (!rows.empty()) {
    int r = int(std::find(rows.begin(), rows.end(), S.cur) - rows.begin());
    int to = r;
    if (pressed(G::Key_UpArrow)) to = r - 1;
    if (pressed(G::Key_DownArrow)) to = r + 1;
    if (pressed(G::Key_PageUp)) to = r - 20;
    if (pressed(G::Key_PageDown)) to = r + 20;
    if (pressed(G::Key_Home)) to = 0;
    if (pressed(G::Key_End)) to = int(rows.size()) - 1;
    to = std::clamp(to, 0, int(rows.size()) - 1);
    if (to != r) {
      move_cursor(f, rows[to]);
      audition(f, S.cur);
    }
  }
  if (pressed(G::Key_LeftArrow)) S.col = Col(std::max(0, int(S.col) - 1));
  if (pressed(G::Key_RightArrow)) S.col = Col(std::min(int(Col::Len), int(S.col) + 1));
  if (pressed(G::Key_I) && shift) S.insert = !S.insert;
  if (pressed(G::Key_L) && !mods) {
    S.catch_on = !S.catch_on;
    S.scroll = true;
  }
  if (pressed(G::Key_G) && !mods) S.graphic = !S.graphic;
  if (pressed(G::Key_O) && shift) S.midi_out = !S.midi_out;
  if (pressed(G::Key_Tab) && S.step) S.steps.pos += from_pulses(f.g.unit_p());
  if (pressed(G::Key_Insert)) {
    EventType t = shift ? EventType::Note : EventType::CC;
    insert(f, new_event(t, new_pos(f), new_chan(f)), "Rotator: insert event");
    return;
  }
  if (cmd && pressed(G::Key_V) && S.clip) {
    Event e = *S.clip;
    e.pos = new_pos(f);
    insert(f, e, "Rotator: paste event");
    return;
  }
  if (!has) return;
  Event& e = f.ev[S.cur];
  if (pressed(G::Key_Enter)) open_text(f);
  if (pressed(G::Key_Equal) || pressed(G::Key_KeypadAdd)) step_value(f, 1);
  else if (pressed(G::Key_Minus) || pressed(G::Key_KeypadSubtract)) step_value(f, -1);
  else if (cmd && pressed(G::Key_C)) S.clip = e;
  else if (cmd && pressed(G::Key_D)) insert(f, e, "Rotator: duplicate event");
  else if (pressed(G::Key_T) && !cmd && S.col >= Col::Chan) {
    transform(f.ev, S.cur, S.col, shift);
    f.c.commit("Rotator: fast transform");
  } else if ((pressed(G::Key_Delete) || pressed(G::Key_Backspace)) && shift) {
    S.cur = delete_matching(f.ev, S.cur, alt);
    f.c.commit(alt ? "Rotator: keep events" : "Rotator: delete events");
  } else if (pressed(G::Key_Delete) || pressed(G::Key_Backspace)) {
    erase(f);
  } else if ((pressed(G::Key_KeypadDivide) || pressed(G::Key_Slash)) && f.playing && f.ref) {
    move_event(f.ev, S.cur, rel_pos(f, play_qn), S.insert);
    finish(f, "Rotator: pickup clock");
  }
}

void poll_midi(Frame& f) {
  struct Msg {
    int s, d1, d2;
  };
  std::vector<Msg> msgs;
  int newest = -1;
  for (int idx = 0; idx < 256; ++idx) {
    char buf[32];
    int sz = sizeof buf, ts = 0, dev = 0, loops = 0;
    double pp = 0;
    int seq = MIDI_GetRecentInputEvent(idx, buf, &sz, &ts, &dev, &pp, &loops);
    if (S.seq < 0) S.seq = seq;
    if (!seq || seq <= S.seq) break;
    if (idx == 0) newest = seq;
    if (sz >= 2 && sz <= 3) msgs.push_back({(unsigned char)buf[0], (unsigned char)buf[1], sz > 2 ? (unsigned char)buf[2] : 0});
  }
  if (newest >= 0) S.seq = newest;
  bool stepping = S.step && !f.playing;
  if (!stepping && !S.midi_in) return;
  for (auto m = msgs.rbegin(); m != msgs.rend(); ++m) {
    int kind = m->s & 0xF0;
    bool on = kind == 0x90 && m->d2 > 0, off = kind == 0x80 || (kind == 0x90 && m->d2 == 0);
    if (stepping) {
      double unit = from_pulses(f.g.unit_p());
      if (on) {
        S.cur = step_note_on(S.steps, f.ev, (m->s & 15) + 1, m->d1, m->d2, unit - from_pulses(1));
        S.scroll = true;
        f.c.commit("Rotator: step input");
      } else if (off) {
        step_note_off(S.steps, unit);
      }
    } else if (S.cur < int(f.ev.size()) && midi_in(f.ev[S.cur], S.col, m->s, m->d1, m->d2)) {
      finish(f, "Rotator: MIDI in");
    }
  }
}

void type_button(Frame& f, int t, double x, double y, double w, const char* label) {
  ImGui_Context* ctx = f.c.ctx;
  G::PushID(ctx, ("ty" + std::to_string(t)).c_str());
  st::hit("##ty", x, y, w, 16);
  if (G::IsItemDeactivated(ctx) && G::IsMouseReleased(ctx, G::MouseButton_Left) && st::hovered()) S.show[t] = !S.show[t];
  if (st::right_clicked()) insert(f, new_event(kTypes[t], new_pos(f), new_chan(f)), "Rotator: insert event");
  G::SetItemTooltip(ctx, "Click: show/hide in the list. Drag onto a row: insert there. Right click: insert at the cursor event. Keys: Insert = Control, Shift-Insert = Note");
  if (G::BeginDragDropSource(ctx)) {
    G::SetDragDropPayload(ctx, "NTR_EVTYPE", std::to_string(t).c_str());
    G::Text(ctx, status_name(kTypes[t]));
    G::EndDragDropSource(ctx);
  }
  G::PopID(ctx);
  st::fill(x, y, w, 16, kPaper);
  st::frame(x, y, w, 16);
  st::DrawClip clip(x + 1, y, w - 2, 16);
  st::text(x + (w - st::text_w(label, Font::Bold) - 1) / 2, y, label, Font::Bold);
  clip.pop();
  if (!S.show[t]) st::dither(x + 1, y + 1, w - 2, 14, kPaper);
}

void left_column(Frame& f) {
  st::fill(0, 0, kLeftW + 1, 400, kInk);
  struct {
    int t;
    const char* label;
  } rows[] = {{0, "*ON|OFF"}, {2, "PROGRAM"}, {1, "CONTROL"}, {3, "PITCH W"}};
  double y = 1;
  for (auto& r : rows) {
    type_button(f, r.t, 1, y, kLeftW - 1, r.label);
    y += 17;
  }
  type_button(f, 5, 1, y, 13, "p");
  type_button(f, 4, 14, y, kLeftW - 14, "cPRES");
  y += 22;
  bool has = S.cur < int(f.ev.size());
  if (st::button("##dup", 2, y, kLeftW - 4, 12, "DUP", "", false, Font::Small) && has)
    insert(f, f.ev[S.cur], "Rotator: duplicate event");
  G::SetItemTooltip(f.c.ctx, "Duplicate (Cmd-D). Cmd-C / Cmd-V copy and paste at the cursor position");
  if (st::button("##del", 2, y + 16, kLeftW - 4, 12, "DELETE", "", false, Font::Small) && has) erase(f);
  G::SetItemTooltip(f.c.ctx, "Delete (Del). Shift-Del: all of this status/pitch/controller; Alt-Shift-Del: keep only those");
}

bool box_button(const char* id, double x, double y, double w, double h, const std::string& label, bool on, Font font) {
  bool hit = st::hit(id, x, y, w, h);
  on = on != (G::IsItemActive(S.ctx) && st::hovered());
  st::fill(x, y, w, h, on ? kInk : kPaper);
  st::frame(x, y, w, h, on ? kPaper : kInk);
  double tw = st::text_w(label, font) + (font == Font::Bold);
  st::DrawClip clip(x + 1, y, w - 2, h);
  st::text(x + (w - tw) / 2, y + (h - st::line_h(font)) / 2, label, font, on ? kPaper : kInk);
  clip.pop();
  return hit;
}

bool mode_button(const char* id, double y, const char* label, bool* v, const char* tip, Font font = Font::Sys) {
  bool hit = box_button(id, kRightX + 1, y, 57, font == Font::Small ? 11 : 16, label, *v, font);
  if (hit) *v = !*v;
  G::SetItemTooltip(S.ctx, tip);
  return hit;
}

void arrow(double x, double y, double w, double h, bool up) {
  st::fill(x, y, w, h, kPaper);
  st::frame(x, y, w, h);
  st::fill(x + w / 2, y + 1, w / 2 - 1, h - 2, kInk);
  double cx = x + w / 4, cy = y + h / 2;
  for (int k = 0; k < 5; ++k) st::fill(cx - k, cy + (up ? k - 3 : 2 - k), 2 * k + 1, 1, kInk);
}

void right_column(Frame& f, int page) {
  ImGui_Context* ctx = f.c.ctx;
  st::fill(kRightX - 1, 0, 641 - kRightX, 400, kInk);
  if (box_button("##catch", kRightX + 1, 1, 57, 11, "CATCH", S.catch_on, Font::Small)) {
    S.catch_on = !S.catch_on;
    S.scroll = true;
  }
  G::SetItemTooltip(ctx, "Follow playback (L)");
  arrow(kRightX + 4, 15, 50, 18, true);
  if (st::hit("##up", kRightX + 4, 15, 50, 18)) S.top -= page;
  arrow(kRightX + 4, 35, 50, 18, false);
  if (st::hit("##down", kRightX + 4, 35, 50, 18)) S.top += page;
  double y = 57;
  bool was_step = S.step;
  mode_button("##ins", y, "INSMODE", &S.insert, "Moving an event moves all later events too (Shift-I)");
  mode_button("##chord", y += 17, "CHORD", &S.chord, "Play all notes on the same pulse");
  mode_button("##out", y += 17, "MIDIOUT", &S.midi_out, "Play events on cursor moves and edits (Shift-O). Uses REAPER's virtual keyboard queue");
  mode_button("##in", y += 17, "MIDI IN", &S.midi_in, "Playing sets the cursor event's values");
  mode_button("##step", y += 17, "STEP", &S.step, "Step input while stopped; Tab = rest");
  if (S.step && !was_step) S.steps = {new_pos(f), 0, false};
  bool has = S.cur < int(f.ev.size());
  if (box_button("##cycle", kRightX + 1, y += 17, 57, 16, "CYCLE", false, Font::Sys) && has && f.ref) {
    double bar_qn = from_pulses(f.g.bar_p());
    double start = f.ref->start + (to_bbup(f.ev[S.cur].pos, f.g).bar - 1) * bar_qn;
    double t0 = TimeMap2_QNToTime(f.c.proj, start), t1 = TimeMap2_QNToTime(f.c.proj, start + bar_qn);
    GetSet_LoopTimeRange2(f.c.proj, true, true, &t0, &t1, false);
    GetSetRepeatEx(f.c.proj, 1);
  }
  G::SetItemTooltip(ctx, "Loop REAPER around the cursor event's bar");
  y += 22;
  mode_button("##gfx", y, "GRAPHIC", &S.graphic, "Bar graphic beside the list (G)", Font::Small);
  mode_button("##glob", y += 12, "GLOBAL", &S.global, "Bar numbers in song position of the placement under the play cursor", Font::Small);
  mode_button("##drum", y += 12, "DRUMNAME", &S.drums, "Pitch names from the destination track's note names", Font::Small);
  if (S.step) {
    st::text_box(kRightX + 3, y + 20, "STEP AT", Font::Small, false);
    st::text_box(kRightX + 3, y + 28, format_pos(S.steps.pos, f.g, f.bar_offset), Font::Small, false);
  }
}

void header(Frame& f) {
  ImGui_Context* ctx = f.c.ctx;
  st::fill(kLeftW + 1, 0, kRightX - kLeftW - 2, kHead, kPaper);
  st::fill(kLeftW + 1, kHead - 1, kRightX - kLeftW - 2, 1, kInk);
  auto cap = [](double x, double w, const std::string& s, bool right = true) {
    st::text(right ? x + w - st::text_w(s, Font::Small) : x, 2, s, Font::Small);
  };
  if (S.graphic)
    for (int b = 0; b < f.g.num; ++b) cap(kGfxX + 2 + b * kGfxW / f.g.num, 6, std::to_string(b + 1), false);
  cap(col_x(Col::Bar).x, col_x(Col::Bar).w, "BAR");
  cap(col_x(Col::Beat).x, col_x(Col::Beat).w, "/" + std::to_string(f.g.denom));
  ColX u = col_x(Col::Unit);
  st::text_box(u.x + u.w - st::text_w(std::to_string(f.g.format), Font::Small), 2, std::to_string(f.g.format), Font::Small, true);
  if (st::hit("##fmt", u.x, 0, u.w, kHead)) g_format = (g_format + 1) % 8;
  if (st::right_clicked()) g_format = (g_format + 7) % 8;
  G::SetItemTooltip(ctx, "Format (unit length): click +, right click -");
  cap(col_x(Col::Pulse).x, col_x(Col::Pulse).w, std::to_string(4 * kPPQ));
  cap(col_x(Col::Status).x, col_x(Col::Status).w, "STATUS", false);
  cap(col_x(Col::Chan).x, col_x(Col::Chan).w, "CHANNEL");
  cap(col_x(Col::D1).x, col_x(Col::D1).w, "-1-");
  cap(col_x(Col::D2).x, col_x(Col::D2).w, "-2-");
  cap(col_x(Col::Len).x + 8, col_x(Col::Len).w, "Length/Info", false);
}

void list_canvas(Frame& f, double h) {
  st::fill(kLeftW + 1, kHead, kRightX - kLeftW - 2, h, kPaper);
  if (!S.graphic) return;
  for (int u = 0; u * f.g.unit_p() <= f.g.bar_p(); ++u) {
    double x = kGfxX + std::round(double(u) * f.g.unit_p() / f.g.bar_p() * kGfxW);
    if (u * f.g.unit_p() % f.g.beat_p() == 0) st::fill(x, kHead, 1, h, kInk);
    else if (kGfxW * f.g.unit_p() / f.g.bar_p() >= 3) st::dotted_v(x, kHead + 1, h - 1);
  }
}

}  // namespace

void event_editor(EditCtx& c) {
  ImGui_Context* ctx = c.ctx;
  if (c.pattern != S.pattern || c.track != S.track) {
    S.pattern = c.pattern;
    S.track = c.track;
    S.cur = 0;
    S.lasso = -1;
    S.steps = {};
  }
  Frame f{c, c.trk.events, {}};
  f.now = G::GetTime(ctx);
  notes_off(f.now, false);
  if (S.cur >= int(f.ev.size())) S.cur = std::max(0, int(f.ev.size()) - 1);
  if (!G::IsAnyItemActive(ctx) && S.lasso < 0 && !std::is_sorted(f.ev.begin(), f.ev.end(), before))
    S.cur = sort_events(f.ev, S.cur);

  f.playing = GetPlayStateEx(c.proj) & 1;
  double play_qn = TimeMap2_timeToQN(c.proj, f.playing ? GetPlayPositionEx(c.proj) : GetCursorPositionEx(c.proj));
  auto pl = placements(c.song, c.pattern, bar_to_qn);
  f.ref = placement_at(pl, play_qn);
  if (!f.ref && !pl.empty()) f.ref = &pl.front();
  if (f.ref) {
    int num = 4, denom = 4;
    double tempo = 0;
    TimeMap_GetTimeSigAtTime(c.proj, TimeMap2_QNToTime(c.proj, f.ref->start), &num, &denom, &tempo);
    if (num > 0 && denom > 0 && denom <= 64) f.g.num = num, f.g.denom = denom;
    if (S.global) f.bar_offset = f.ref->bar - 1;
  }
  f.g.format = kFormats[g_format];

  if (f.playing && !S.was_playing) S.catch_on = true;
  S.was_playing = f.playing;
  auto vis = [&](int i) { return shown(i, f.ev); };
  if (f.playing && S.catch_on && f.ref && !f.ev.empty() && placement_at(pl, play_qn)) {
    int i = event_at(f.ev, rel_pos(f, play_qn), vis);
    if (i != S.cur) S.scroll = true;
    S.cur = i;
  }
  poll_midi(f);
  S.ctx = ctx;

  std::vector<int> rows;
  for (int i = 0; i < int(f.ev.size()); ++i)
    if (vis(i)) rows.push_back(i);

  double aw = 0, ah = 0;
  G::GetContentRegionAvail(ctx, &aw, &ah);
  double H = std::max(120.0, std::floor(ah / st::scale()));
  st::begin(ctx, st::scale());
  int page = std::max(1, int((H - kHead) / kRowH));
  int cur_row = int(std::find(rows.begin(), rows.end(), S.cur) - rows.begin());
  if (S.scroll && cur_row < int(rows.size()) && (cur_row < S.top || cur_row >= S.top + page)) S.top = cur_row - page / 2;
  S.scroll = false;
  bool focused = G::IsWindowFocused(ctx, G::FocusedFlags_RootAndChildWindows);
  double mx, my;
  st::mouse(&mx, &my);
  if (G::IsWindowHovered(ctx) && mx > kLeftW && mx < kRightX) {
    double wheel = 0, hw = 0;
    G::GetMouseWheel(ctx, &wheel, &hw);
    S.top -= int(std::lround(wheel * 3));
  }
  S.top = std::clamp(S.top, 0, std::max(0, int(rows.size()) - page));

  st::fill(0, 0, 640, H, kInk);
  list_canvas(f, H - kHead);
  header(f);
  MediaTrack* drums = S.drums ? dest_track(c.trk.dest) : nullptr;
  S.rows_y.clear();
  {
    st::DrawClip clip(kLeftW + 1, kHead, kRightX - kLeftW - 2, H - kHead);
    if (f.ev.empty()) st::text(kGfxX + kGfxW + 8, kHead + 8, "NO EVENTS: INSERT / SHIFT-INSERT, DRAG A TYPE, OR STEP INPUT", Font::Small);
    for (int r = S.top; r < int(rows.size()) && r < S.top + page + 1; ++r) row(f, rows[r], kHead + (r - S.top) * kRowH, drums);
    clip.pop();
    double y_end = kHead + std::max(0, int(rows.size()) - S.top) * kRowH;
    if (y_end < H - 1) {
      st::hit("##rest", kLeftW + 1, y_end, kRightX - kLeftW - 2, H - y_end);
      if (G::BeginDragDropTarget(ctx)) {
        char buf[4] = "";
        if (G::AcceptDragDropPayload(ctx, "NTR_EVTYPE", buf, sizeof buf)) S.drop_type = buf[0] - '0', S.drop_pos = new_pos(f), S.drop_chan = new_chan(f);
        G::EndDragDropTarget(ctx);
      }
    }
  }
  left_column(f);
  right_column(f, page);
  st::end(640, H);

  if (S.drop_type >= 0) {
    insert(f, new_event(kTypes[S.drop_type], S.drop_pos, S.drop_chan), "Rotator: insert event");
    S.drop_type = -1;
  }
  if (S.pending) {
    finish(f, S.pending);
    S.pending = nullptr;
  }
  if (S.open_req && S.cur < int(f.ev.size())) open_text(f);
  S.open_req = false;
  lasso_update(f);
  text_popup(f);
  if (focused && !G::IsAnyItemActive(ctx) && !G::IsPopupOpen(ctx, "##evtext") && S.lasso < 0) keys(f, rows, play_qn);
}
