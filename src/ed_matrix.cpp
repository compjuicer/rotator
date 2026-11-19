#include "reaper_core_api.h"
#include "editor.h"
#include "ed_event_ops.h"
#include "ed_matrix_ops.h"
#include "st.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#undef min
#undef max

namespace G = ImGui;
using namespace rotator;
using st::Font, st::kInk, st::kPaper;

namespace {

enum class Mode { None, Move, Resize, Velocity, Band };

struct Drag {
  Mode mode = Mode::None;
  double mx0 = 0, my0 = 0;
  std::vector<Event> orig;
  int anchor = -1;
  Axis axis = Axis::Undecided;
  double dpos = 0;
  int dpitch = 0;
};

struct View {
  int pattern = -1, track = -1;
  std::vector<int> sel;
  Event last{EventType::Note, 0, 1, 60, 100, 0.25};
  Drag drag;
  bool veloc = false, snap = true, scroll_to_notes = true;
  int division = 4;
  double zoom = 32, key_w = 8;  // ST pixels per QN, per key
  double top = 0;               // QN at the top edge
  int lowest = 36;              // pitch at the left edge
  int sounding = -1, sounding_chan = 1;
};
View g;

constexpr double kBeats = 4;  // QN per bar; patterns carry no signature
constexpr double kPanelW = 62, kGx = 64, kKeysH = 26;
const char* const kDivisions[] = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/4T", "1/8T", "1/16T", "1/32T"};

double grid_qn(int d) { return d < 7 ? 4.0 / (1 << d) : 4.0 / (1 << (d - 5)) * 2 / 3; }

bool black(int p) { return (0x54A >> (p % 12)) & 1; }

bool same(const std::vector<Event>& a, const std::vector<Event>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (a[i].type != b[i].type || a[i].pos != b[i].pos || a[i].chan != b[i].chan || a[i].data1 != b[i].data1 ||
        a[i].data2 != b[i].data2 || a[i].len != b[i].len)
      return false;
  return true;
}

void sound(const Track& t, int pitch, int chan, int vel) {
  if (pitch == g.sounding) return;
  if (g.sounding >= 0) StuffMIDIMessage(0, 0x80 | (g.sounding_chan - 1), g.sounding, 0);
  g.sounding = pitch;
  if (pitch < 0) return;
  g.sounding_chan = std::clamp(t.chan.value_or(chan), 1, 16);
  StuffMIDIMessage(0, 0x90 | (g.sounding_chan - 1), pitch, vel);
}

bool toggle_box(ImGui_Context* ctx, const char* id, double y, const char* label, bool* v, const char* tip) {
  bool hit = st::hit(id, 1, y, kPanelW - 2, 18);
  G::SetItemTooltip(ctx, tip);
  if (hit) *v = !*v;
  st::fill(1, y, kPanelW - 2, 18, *v ? kInk : kPaper);
  st::frame(1, y, kPanelW - 2, 18, *v ? kPaper : kInk);
  st::text(1 + (kPanelW - 2 - st::text_w(label, Font::Bold)) / 2, y + 1, label, Font::Bold, *v ? kPaper : kInk);
  return hit;
}

void field_box(double y, const char* cap) {
  st::fill(1, y, kPanelW - 2, 27, kPaper);
  st::frame(1, y, kPanelW - 2, 27);
  st::fill(1, y + 18, kPanelW - 2, 9, kPaper);
  st::stripes(2, y + 19, kPanelW - 4, 7);
  st::frame(1, y + 18, kPanelW - 2, 9);
  st::text_box(1 + (kPanelW - 2 - st::text_w(cap, Font::Small)) / 2, y + 20, cap, Font::Small, false);
}

}  // namespace

void matrix_editor(EditCtx& ec) {
  ImGui_Context* ctx = ec.ctx;
  Track& trk = ec.trk;
  auto& ev = trk.events;
  if (g.pattern != ec.pattern || g.track != ec.track) {
    g.pattern = ec.pattern;
    g.track = ec.track;
    g.sel.clear();
    g.drag = {};
    g.scroll_to_notes = true;
  }
  g.sel.erase(std::remove_if(g.sel.begin(), g.sel.end(),
                             [&](int i) { return i >= int(ev.size()) || ev[i].type != EventType::Note; }),
              g.sel.end());
  if (g.drag.mode != Mode::None && g.drag.mode != Mode::Band && g.drag.orig.size() != ev.size()) g.drag = {};

  double aw = 0, ah = 0;
  G::GetContentRegionAvail(ctx, &aw, &ah);
  double H = std::max(120.0, std::floor(ah / st::scale()));
  st::begin(ctx, st::scale());
  st::fill(0, 0, 640, H, kInk);
  double gx = kGx, gy = 0, gw = 640 - kGx, gh = H - kKeysH - 1;
  int nkeys = int(std::ceil(gw / g.key_w));

  if (g.scroll_to_notes) {
    int lo = 128;
    for (const Event& e : ev)
      if (e.type == EventType::Note) lo = std::min(lo, e.data1);
    g.lowest = std::max(0, (lo == 128 ? 48 : lo) - 4);
    g.top = 0;
    g.scroll_to_notes = false;
  }
  g.lowest = std::clamp(g.lowest, 0, std::max(0, 128 - nkeys));

  // Left panel
  st::fill(1, 1, kPanelW - 2, 18, kPaper);
  st::frame(1, 1, kPanelW - 2, 18);
  st::text(1 + (kPanelW - 2 - st::text_w("MATRIX", Font::Bold)) / 2, 2, "MATRIX", Font::Bold);
  toggle_box(ctx, "##snap", 21, "SNAP", &g.snap, "Snap moves, lengths and inserts to the grid");
  toggle_box(ctx, "##veloc", 41, "VELOC", &g.veloc, "Dragging a note changes its velocity (or hold Alt)");
  field_box(63, "GRID");
  st::value("##div", 3, 63, kPanelW - 6, 18, {&g.division, 0, 10, [](int v) { return std::string(kDivisions[v]); }});
  G::SetItemTooltip(ctx, "Grid and snap division: click +, right click -");
  field_box(93, "LOWEST");
  st::value("##low", 3, 93, kPanelW - 6, 18,
            {&g.lowest, 0, std::max(0, 128 - nkeys), [](int v) { return note_name(v); }});
  G::SetItemTooltip(ctx, "Lowest key shown (Shift-wheel scrolls pitch)");
  {
    st::fill(1, 124, kPanelW - 2, 34, kPaper);
    st::frame(1, 124, kPanelW - 2, 34);
    std::string a, b;
    if (g.sel.size() == 1) {
      const Event& e = ev[g.sel[0]];
      a = note_name(e.data1) + " CH" + std::to_string(e.chan);
      b = "VEL " + std::to_string(e.data2);
    } else {
      a = std::to_string(g.sel.size()) + " SEL";
    }
    st::text(4, 127, a, Font::Small);
    st::text(4, 137, b, Font::Small);
    st::text(4, 147, std::to_string(int(std::lround(g.zoom))) + " PX/Q", Font::Small);
  }

  double grid = grid_qn(g.division), snap_grid = g.snap ? grid : 0;
  double end = trk.loop.value_or(0);
  if (auto p = ec.song.patterns.find(ec.pattern); p != ec.song.patterns.end()) end = std::max(end, pattern_length(p->second));
  for (const Event& e : ev) end = std::max(end, e.pos + e.len);

  int mods = G::GetKeyMods(ctx);
  bool ctrl = mods & G::Mod_Ctrl, shift = mods & G::Mod_Shift, alt = mods & G::Mod_Alt;
  double mx = 0, my = 0;
  st::mouse(&mx, &my);
  auto X = [&](int p) { return gx + (p - g.lowest) * g.key_w; };
  auto Y = [&](double q) { return gy + (q - g.top) * g.zoom; };
  double q0 = g.top, q1 = g.top + gh / g.zoom;
  double total = std::max(std::ceil(end / kBeats) * kBeats + 2 * kBeats, q1);
  double mpos = g.top + (my - gy) / g.zoom;
  int mpitch = std::clamp(g.lowest + int(std::floor((mx - gx) / g.key_w)), 0, 127);

  st::hit("##notes", gx, gy, gw, gh);
  bool hovered = st::hovered();
  if (G::IsWindowHovered(ctx) && mx >= gx && my < gy + gh + kKeysH) {
    double wheel = 0, hw = 0;
    G::GetMouseWheel(ctx, &wheel, &hw);
    if (wheel && ctrl && shift) {
      g.key_w = std::clamp(g.key_w + (wheel > 0 ? 1 : -1), 3.0, 24.0);
    } else if (wheel && ctrl) {
      double nz = std::clamp(g.zoom * (wheel > 0 ? 1.25 : 0.8), 4.0, 400.0);
      g.top = std::max(0.0, mpos - (my - gy) / nz);
      g.zoom = nz;
    } else if (wheel && shift) {
      g.lowest -= int(std::lround(wheel * 2));
    } else if (wheel) {
      g.top = std::max(0.0, g.top - wheel * 32 / g.zoom);
    }
    if (hw) g.lowest += int(std::lround(hw * 2));
    g.lowest = std::clamp(g.lowest, 0, std::max(0, 128 - nkeys));
  }

  // Grid: white paper, dotted step lines, solid numbered bar lines
  st::fill(gx, gy, gw, gh, kPaper);
  st::DrawClip clip(gx, gy, gw, gh);
  if (grid * g.zoom >= 3)
    for (double q = std::ceil(q0 / grid - 1e-9) * grid; q < q1; q += grid) st::dotted_h(gx + 1, std::floor(Y(q)), gw);
  for (double q = std::ceil(q0 - 1e-9); q < q1; q += 1)
    if (std::fmod(q, kBeats) != 0) {
      double y = std::floor(Y(q));
      for (double x = gx; x < gx + gw; x += 4) st::fill(x, y, 3, 1, kInk);
    }
  if (trk.loop && *trk.loop < q1) {
    double y = std::max(gy, Y(*trk.loop));
    st::dither(gx, y, gw, gy + gh - y, kInk);
    st::fill(gx, y, gw, 2, kInk);
  }
  for (double q = std::ceil(q0 / kBeats - 1e-9) * kBeats; q < q1; q += kBeats) {
    double y = std::floor(Y(q));
    st::fill(gx, y, gw, 1, kInk);
    st::text_box(gx + 3, y + 2, std::to_string(int(std::lround(q / kBeats)) + 1), Font::Small, false);
  }

  std::vector<bool> selected(ev.size());
  for (int i : g.sel) selected[i] = true;
  double bw = std::max(1.0, g.key_w - 2);
  for (int i = 0; i < int(ev.size()); ++i) {
    const Event& e = ev[i];
    if (e.type != EventType::Note || e.pos + e.len < q0 || e.pos > q1 || e.data1 < g.lowest) continue;
    double x = X(e.data1) + (g.key_w - bw) / 2, y0 = Y(e.pos), h = std::max(2.0, e.len * g.zoom);
    if (selected[i]) {
      st::fill(x - 1, y0, bw + 2, h, kInk);
      st::fill(x, y0 + 1, bw, h - 2, kPaper);
      st::stripes(x, y0 + 1, bw, h - 2);
    } else {
      st::fill(x, y0, bw, h, kInk);
    }
  }

  Drag& d = g.drag;
  if (d.mode == Mode::Band) {
    double x0 = std::min(d.mx0, mx), y0 = std::min(d.my0, my), w = std::abs(mx - d.mx0), h = std::abs(my - d.my0);
    st::dotted_h(x0, y0, w);
    st::dotted_h(x0, y0 + h, w);
    st::dotted_v(x0, y0, h);
    st::dotted_v(x0 + w, y0, h);
  }
  clip.pop();

  double min_hit = 4 / g.zoom, edge = 2 / g.zoom;
  NoteHit hit = hovered ? hit_note(ev, mpos, mpitch, min_hit, edge) : NoteHit{};
  if (d.mode == Mode::None && hit.index >= 0 && hit.end && !g.veloc && !alt) G::SetMouseCursor(ctx, G::MouseCursor_ResizeNS);

  if (d.mode == Mode::None && hovered && G::IsMouseClicked(ctx, G::MouseButton_Left)) {
    if (hit.index >= 0) {
      const Event& e = ev[hit.index];
      bool in = std::find(g.sel.begin(), g.sel.end(), hit.index) != g.sel.end();
      if (ctrl) {
        if (in) g.sel.erase(std::find(g.sel.begin(), g.sel.end(), hit.index));
        else g.sel.push_back(hit.index);
      } else {
        if (!in) g.sel = {hit.index};
        g.last = e;
        d = {g.veloc || alt ? Mode::Velocity : hit.end ? Mode::Resize : Mode::Move, mx, my, ev, hit.index};
        sound(trk, e.data1, e.chan, e.data2);
      }
    } else {
      if (!ctrl) g.sel.clear();
      d = {Mode::Band, mx, my, {}};
      sound(trk, mpitch, g.last.chan, g.last.data2);
    }
  }
  if (d.mode == Mode::None && hovered && !g.veloc && G::IsMouseClicked(ctx, G::MouseButton_Right)) {
    if (hit.index >= 0) {
      delete_events(ev, {hit.index});
      g.sel.clear();
      ec.commit("Rotator: delete note");
    } else {
      Event n = g.last;
      n.pos = std::max(0.0, g.snap ? snap(mpos, grid) : mpos);
      n.data1 = mpitch;
      int i = insert_note(ev, n);
      if (i >= 0) {
        g.sel = {i};
        g.last = ev[i];
        sound(trk, n.data1, n.chan, n.data2);
        ec.commit("Rotator: insert note");
      }
    }
  }

  if (d.mode == Mode::Move || d.mode == Mode::Resize || d.mode == Mode::Velocity) {
    const Event& a = d.orig[d.anchor];
    double dx = mx - d.mx0, dy = my - d.my0;
    ev = d.orig;
    if (d.mode == Mode::Move) {
      d.axis = lock_axis(d.axis, dx, dy, 2, shift || G::IsMouseDown(ctx, G::MouseButton_Right));
      bool time = d.axis == Axis::Time || d.axis == Axis::Free, pitch = d.axis == Axis::Pitch || d.axis == Axis::Free;
      double dpos = time ? snap(a.pos + dy / g.zoom, snap_grid) - a.pos : 0;
      int dpitch = pitch ? int(std::lround(dx / g.key_w)) : 0;
      if (move_notes(ev, g.sel, dpos, dpitch)) {
        d.dpos = ev[d.anchor].pos - a.pos;
        d.dpitch = ev[d.anchor].data1 - a.data1;
      } else {
        move_notes(ev, g.sel, d.dpos, d.dpitch);
      }
      sound(trk, ev[d.anchor].data1, a.chan, a.data2);
    } else if (d.mode == Mode::Resize) {
      double e1 = snap(a.pos + a.len + dy / g.zoom, snap_grid);
      resize_notes(ev, g.sel, e1 - a.pos - a.len, g.snap ? std::min(grid, std::max(a.len, 1 / 96.0)) : 1 / 96.0);
    } else {
      add_velocity(ev, g.sel, int(std::lround(-dy)));
    }
    if (!G::IsMouseDown(ctx, G::MouseButton_Left)) {
      g.last = ev[d.anchor];
      if (!same(ev, d.orig))
        ec.commit(d.mode == Mode::Move ? "Rotator: move notes" : d.mode == Mode::Resize ? "Rotator: note length" : "Rotator: velocity");
      d = {};
    }
  } else if (d.mode == Mode::Band && !G::IsMouseDown(ctx, G::MouseButton_Left)) {
    if (std::abs(mx - d.mx0) + std::abs(my - d.my0) > 2)
      for (int i : notes_in(ev, g.top + (d.my0 - gy) / g.zoom, mpos,
                            g.lowest + int(std::floor((d.mx0 - gx) / g.key_w)), mpitch))
        if (std::find(g.sel.begin(), g.sel.end(), i) == g.sel.end()) g.sel.push_back(i);
    d = {};
  }
  if (G::IsWindowFocused(ctx, G::FocusedFlags_RootAndChildWindows) && d.mode == Mode::None && !G::IsAnyItemActive(ctx)) {
    if (!g.sel.empty() && (G::IsKeyPressed(ctx, G::Key_Delete) || G::IsKeyPressed(ctx, G::Key_Backspace))) {
      delete_events(ev, g.sel);
      g.sel.clear();
      ec.commit("Rotator: delete notes");
    }
    if (ctrl && G::IsKeyPressed(ctx, G::Key_A)) g.sel = notes_in(ev, 0, total, 0, 127);
  }

  // Keyboard along the bottom
  double ky = gy + gh + 1, kh = kKeysH;
  st::hit("##keys", gx, ky, gw, kh);
  bool keys_active = G::IsItemActive(ctx), on_keys = st::hovered();
  st::fill(gx, ky, gw, kh, kPaper);
  st::DrawClip kclip(gx, ky, gw, kh);
  for (int p = g.lowest; p < 128 && X(p) < gx + gw; ++p) {
    double a = X(p);
    if (black(p)) {
      st::fill(a + 1, ky, g.key_w - 2, kh * 0.6, kInk);
      st::fill(a + g.key_w / 2, ky + kh * 0.6, 1, kh * 0.4, kInk);
    } else if (p % 12 == 0 || p % 12 == 5) {
      st::fill(a, ky, 1, kh, kInk);
    }
    if (p == 60) st::text(a + (g.key_w - 12) / 2, ky + kh - 7, "C3", Font::Small);
  }
  int dot = hovered || on_keys ? mpitch : -1;
  if (d.mode == Mode::Move) dot = ev[d.anchor].data1;
  if (dot >= g.lowest && dot < 128) {
    double s = std::max(2.0, std::min(5.0, g.key_w - 3));
    double x = X(dot) + (g.key_w - s) / 2, y = ky + (black(dot) ? kh * 0.6 - s - 2 : kh - s - 3);
    st::fill(x - 1, y - 1, s + 2, s + 2, black(dot) ? kPaper : kInk);
    st::fill(x, y, s, s, black(dot) ? kInk : kPaper);
  }
  kclip.pop();
  st::end(640, H);
  if (keys_active) sound(trk, std::clamp(mpitch, 0, 127), g.last.chan, g.last.data2);
  if (!G::IsMouseDown(ctx, G::MouseButton_Left)) sound(trk, -1, 1, 0);
}
