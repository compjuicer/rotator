#include "editor.h"
#include "reaper_out.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "ed_event_ops.h"
#include "ed_hyper_ops.h"
#include "st.h"

#undef min
#undef max

namespace G = ImGui;
using namespace rotator;
using st::Font, st::kInk, st::kPaper;

namespace {

enum class Mode { None, Paint, InsertOnly, Erase, EraseGrid, Scale };

struct Gesture {
  Mode mode = Mode::None;
  int lane = -1;
  double t = 0, v = 0, y0 = 0;
  int applied = 0;
  std::vector<Event> before;
};

Gesture g_g;
int g_set = 0;
double g_start = 0;  // bars
int g_bars = 0;      // 0 = fit pattern
int g_lane_h = 28;   // ST pixels
int g_top = 0;       // first visible lane
bool g_params = false, g_autoins = true, g_fixvel = false;
int g_vel = 100;
int g_rename = -2;  // lane being renamed, -1 = set name
char g_name[64];
std::string g_info;

constexpr double kL = 5, kQnPerBar = 4, kPen = 3, kBarH = 16, kRow2 = 18, kHeadY = 32, kLanesY = 43;
const double kGrids[] = {1, 0.5, 1.0 / 3, 0.25, 1.0 / 6, 0.125, 1.0 / 12, 0.0625, 1.0 / 24};
const int kGridDen[] = {4, 8, 12, 16, 24, 32, 48, 64, 96};
const EventType kTypes[] = {EventType::Note, EventType::CC, EventType::Program, EventType::PitchBend, EventType::ChanPressure, EventType::PolyPressure};
const char* const kCheck = "\xE2\x88\x9A";

struct Col {
  const char* head;
  double w;
};
const Col kCols[] = {{"N A M E", 74}, {"QUA", 24}, {"DLY", 32}, {"HHAT", 22}, {"STATUS", 60}, {"CHA", 24}, {"-1-", 40}, {"LEN", 34}};

double params_width() {
  double w = 0;
  for (int i = 0; i < (g_params ? 8 : 2); ++i) w += kCols[i].w;
  return kL + w + 1;
}

bool same(const std::vector<Event>& a, const std::vector<Event>& b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](const Event& x, const Event& y) {
    return x.type == y.type && x.pos == y.pos && x.chan == y.chan && x.data1 == y.data1 && x.data2 == y.data2 &&
           x.len == y.len;
  });
}

std::string bar_pos(double qn) {
  int bar = int(std::floor(qn / kQnPerBar)), beat = int(std::floor(qn - bar * kQnPerBar));
  int tick = int(std::lround((qn - bar * kQnPerBar - beat) * 192));
  return std::to_string(bar + 1) + " " + std::to_string(beat + 1) + " " + std::to_string(tick / 48 + 1) + " " +
         std::to_string(tick % 48 + 1);
}

void box(double x, double y, double w, double h) {
  st::fill(x, y, w, h, kPaper);
  st::frame(x, y, w, h);
}

// Toolbar cell: small caption, bold value (click +, right click -, drag, double-click types).
bool value_cell(const char* id, double x, double w, const char* cap, st::Value v) {
  box(x, 0, w, kBarH);
  st::text(x + 2, 6, cap, Font::Small);
  double cw = st::text_w(cap, Font::Small) + 3;
  return st::value(id, x + cw, 0, w - cw - 2, kBarH, v);
}

void name_edit(ImGui_Context* ctx, double x, double y, double w, std::string& target, const std::function<void()>& done) {
  G::SetCursorScreenPos(ctx, st::sx(x), st::sy(y));
  G::SetNextItemWidth(ctx, w * st::scale());
  if (G::IsWindowAppearing(ctx) || !G::IsAnyItemActive(ctx)) G::SetKeyboardFocusHere(ctx);
  bool enter = G::InputText(ctx, "##rename", g_name, sizeof g_name, G::InputTextFlags_EnterReturnsTrue | G::InputTextFlags_AutoSelectAll);
  if (enter) {
    target = g_name;
    g_rename = -2;
    done();
  } else if (G::IsKeyPressed(ctx, G::Key_Escape) || (G::IsItemDeactivated(ctx) && !enter)) {
    g_rename = -2;
  }
}

}  // namespace

void hyper_editor(EditCtx& ec) {
  ImGui_Context* ctx = ec.ctx;
  Song& song = ec.song;
  Track& trk = ec.trk;
  if (song.hyper_sets.empty()) song.hyper_sets.push_back(default_hyper_set());
  int nsets = int(song.hyper_sets.size());
  if (trk.hyper_set >= 0 && trk.hyper_set < nsets) g_set = trk.hyper_set;
  g_set = std::clamp(g_set, 0, nsets - 1);
  auto done = [&](const char* desc) {
    trk.hyper_set = g_set;
    ec.commit(desc);
  };

  double aw = 0, ah = 0;
  G::GetContentRegionAvail(ctx, &aw, &ah);
  double H = std::max(120.0, std::floor(ah / st::scale()));
  st::begin(ctx, st::scale());
  st::fill(0, 0, 640, H, kInk);
  HyperSet* set = &song.hyper_sets[g_set];
  int nl = int(set->lanes.size());

  // Toolbar
  double plen = pattern_length(song.patterns[ec.pattern]);
  int fit = std::max(1, int(std::ceil(plen / kQnPerBar - 1e-9)));
  int bars = g_bars > 0 ? g_bars : fit;
  int set_no = g_set;
  if (value_cell("##set", 0, 46, "SET", {&set_no, 0, nsets - 1, [](int v) { return std::to_string(v + 1); }})) {
    g_set = set_no;
    if (st::final()) return done("Rotator: hyper set");
  }
  G::SetItemTooltip(ctx, "Hyper set: click +, right click -");
  if (value_cell("##bars", 46, 62, "BARS", {&bars, 1, 64, {}})) g_bars = bars;
  G::SetItemTooltip(ctx, "Bars shown (Ctrl-wheel)");
  int start = int(std::lround(g_start)) + 1;
  if (value_cell("##start", 108, 54, "FROM", {&start, 1, 999, {}})) g_start = start - 1;
  G::SetItemTooltip(ctx, "First bar shown (horizontal wheel)");
  value_cell("##lh", 162, 62, "HEIGHT", {&g_lane_h, 16, 120, {}});
  G::SetItemTooltip(ctx, "Lane height");
  value_cell("##pen", 224, 50, "PEN", {&g_vel, 1, 127, {}});
  G::SetItemTooltip(ctx, "Velocity for FixVel");
  auto check = [&](const char* id, double x, double w, const char* cap, bool* v, const char* tip) {
    box(x, 0, w, kBarH);
    st::text(x + 3, 6, cap, Font::Small);
    if (*v) st::text(x + w - 10, 0, kCheck);
    if (st::hit(id, x, 0, w, kBarH)) *v = !*v;
    G::SetItemTooltip(ctx, tip);
  };
  check("##ai", 274, 58, "AutoIns", &g_autoins, "Drawing on empty grid steps inserts events (Shift also inserts)");
  check("##fv", 332, 52, "FixVel", &g_fixvel, "Drawn notes take the PEN velocity");
  check("##par", 384, 58, "Params", &g_params, "Show all lane parameter columns");
  box(442, 0, 198, kBarH);
  st::DrawClip iclip(444, 0, 194, kBarH);
  st::text(446, 6, g_info, Font::Small);
  iclip.pop();

  // Set row
  box(0, kRow2, 150, 12);
  st::text(3, kRow2 + 3, "SET NAME", Font::Small);
  if (g_rename == -1) {
    name_edit(ctx, 52, kRow2, 98, set->name, [&] { done("Rotator: hyper set name"); });
  } else {
    st::text(54, kRow2 + 3, set->name.substr(0, 16), Font::Small);
    st::hit("##setname", 0, kRow2, 150, 12);
    G::SetItemTooltip(ctx, "Double-click to rename the set");
    if (st::double_clicked()) {
      g_rename = -1;
      snprintf(g_name, sizeof g_name, "%s", set->name.c_str());
    }
  }
  double bx = 154;
  auto button = [&](const char* id, const char* label, double w) {
    bool hit = st::button(id, bx, kRow2, w, 11, label, "", false, Font::Small);
    bx += w + 4;
    return hit;
  };
  if (button("##new", "NEW SET", 48)) {
    song.hyper_sets.push_back({"Set " + std::to_string(nsets + 1), {}});
    g_set = nsets;
    return done("Rotator: new hyper set");
  }
  if (button("##dup", "COPY SET", 52)) {
    song.hyper_sets.push_back(*set);
    song.hyper_sets.back().name += " copy";
    g_set = nsets;
    return done("Rotator: duplicate hyper set");
  }
  if (button("##del", "DELETE SET", 62)) {
    delete_hyper_set(song, g_set);
    g_set = std::max(0, g_set - 1);
    if (song.hyper_sets.empty()) song.hyper_sets.push_back(default_hyper_set());
    return done("Rotator: delete hyper set");
  }
  if (button("##add", "ADD LANE", 50)) {
    HyperLane l;
    l.name = "Lane";
    set->lanes.push_back(l);
    return done("Rotator: add hyper lane");
  }
  if (button("##from", "FROM TRACK", 62) && add_unmatched_lanes(*set, trk)) return done("Rotator: hyper lanes from track");
  G::SetItemTooltip(ctx, "Add a lane for every event the set doesn't show yet");
  if (button("##conv", "CONVERT TO SET", 80)) G::OpenPopup(ctx, "convert_set");
  if (G::BeginPopup(ctx, "convert_set")) {
    G::TextDisabled(ctx, "Rewrite events lane by lane to:");
    for (int i = 0; i < nsets; ++i) {
      if (i == g_set) continue;
      if (G::Selectable(ctx, (std::to_string(i + 1) + " " + song.hyper_sets[i].name + "##" + std::to_string(i)).c_str())) {
        convert_set(trk, *set, song.hyper_sets[i]);
        g_set = i;
        G::EndPopup(ctx);
        return done("Rotator: convert to hyper set");
      }
    }
    G::EndPopup(ctx);
  }

  // Header row: column captions and ruler
  double pw = params_width(), cx = pw + 3, cw = 640 - cx;
  double t0 = g_start * kQnPerBar, span = bars * kQnPerBar, pxq = cw / span;
  auto x_of = [&](double qn) { return cx + (qn - t0) * pxq; };
  box(kL, kHeadY, pw - kL, 10);
  for (int i = 0, x = int(kL); i < (g_params ? 8 : 2); x += int(kCols[i].w), ++i) {
    st::fill(x, kHeadY, 1, 10, kInk);
    st::text_fit(x, kHeadY + 2, kCols[i].w, kCols[i].head);
  }
  box(cx, kHeadY, cw, 10);
  for (double q = std::ceil(t0 - 1e-9); q <= t0 + span + 1e-9; q += 1) {
    bool bar = std::fmod(q, kQnPerBar) == 0;
    double x = std::min(x_of(q), 639.0);
    st::fill(x, kHeadY + (bar ? 1 : 6), 1, bar ? 9 : 4, kInk);
    if (bar) st::text(x + 2, kHeadY + 2, std::to_string(int(q / kQnPerBar) + 1), Font::Small);
  }

  // Lanes
  double area = H - kLanesY;
  int vis = std::max(1, int(area / g_lane_h));
  double mx = 0, my = 0;
  st::mouse(&mx, &my);
  int mods = G::GetKeyMods(ctx);
  bool alt = mods & G::Mod_Alt, shift = mods & G::Mod_Shift, ctrl = mods & G::Mod_Ctrl;
  if (G::IsWindowHovered(ctx) && my >= kLanesY) {
    double wheel = 0, hw = 0;
    G::GetMouseWheel(ctx, &wheel, &hw);
    if (ctrl && wheel) g_bars = std::clamp(bars + (wheel > 0 ? -1 : 1), 1, 64);
    else if (wheel) g_top -= int(std::lround(wheel));
    if (hw) g_start = std::max(0.0, g_start - hw * 0.25);
  }
  g_top = std::clamp(g_top, 0, std::max(0, nl - vis));
  std::string info;
  int op_lane = -1, op_kind = 0, op_target = -1;  // 1 up, 2 down, 3 delete, 4 copy, 5 convert, 6 hhat dups, 7 clear
  st::DrawClip lanes_clip(0, kLanesY, 640, area);

  for (int i = g_top; i < nl && i < g_top + vis + 1; ++i) {
    HyperLane& l = set->lanes[i];
    int vlo = value_range(l.type).first, vhi = value_range(l.type).second;
    double ry = kLanesY + (i - g_top) * g_lane_h, lh = g_lane_h - 1, vy = ry + lh - 16;
    G::PushID(ctx, std::to_string(i).c_str());
    st::fill(kL, ry, pw - kL, lh, kPaper);
    double x = kL;
    for (int c = 0; c < (g_params ? 8 : 2); x += kCols[c].w, ++c) st::fill(x, ry, 1, lh, kInk);
    st::fill(x, ry, 1, lh, kInk);

    // NAME: white box, bold; right click for lane functions, double-click to rename
    if (g_rename == i) {
      name_edit(ctx, kL + 2, vy, kCols[0].w - 4, l.name, [&] { done("Rotator: hyper lane name"); });
    } else {
      st::DrawClip nclip(kL + 1, ry, kCols[0].w - 2, lh);
      st::text(kL + 3, vy, l.name, Font::Bold);
      nclip.pop();
      st::hit("##name", kL + 1, ry, kCols[0].w - 1, lh);
      G::SetItemTooltip(ctx, "Right click: lane functions. Double-click: rename");
      if (st::double_clicked()) {
        g_rename = i;
        snprintf(g_name, sizeof g_name, "%s", l.name.c_str());
      }
    }
    if (G::BeginPopupContextItem(ctx, "lane_menu")) {
      if (G::MenuItem(ctx, "Move up", nullptr, nullptr, i > 0)) op_lane = i, op_kind = 1;
      if (G::MenuItem(ctx, "Move down", nullptr, nullptr, i + 1 < nl)) op_lane = i, op_kind = 2;
      if (G::BeginMenu(ctx, "Copy events to")) {
        for (int j = 0; j < nl; ++j)
          if (j != i && G::MenuItem(ctx, (set->lanes[j].name + "##" + std::to_string(j)).c_str())) op_lane = i, op_kind = 4, op_target = j;
        G::EndMenu(ctx);
      }
      if (G::BeginMenu(ctx, "Convert events to")) {
        for (int j = 0; j < nl; ++j)
          if (j != i && G::MenuItem(ctx, (set->lanes[j].name + "##" + std::to_string(j)).c_str())) op_lane = i, op_kind = 5, op_target = j;
        G::EndMenu(ctx);
      }
      if (G::MenuItem(ctx, "Delete duplicated hihat events", nullptr, nullptr, l.hhat)) op_lane = i, op_kind = 6;
      if (G::MenuItem(ctx, "Clear events")) op_lane = i, op_kind = 7;
      if (G::MenuItem(ctx, "Delete lane")) op_lane = i, op_kind = 3;
      G::EndPopup(ctx);
    }

    x = kL + kCols[0].w;
    // Live while dragging; one undo point when the gesture commits.
    auto field = [&](int c, const char* id, int& v, int lo, int hi, std::function<std::string(int)> fmt, const char* desc,
                     const std::function<void(int)>& apply) {
      int v0 = v;
      bool commit = st::value(id, x + 2, vy, kCols[c].w - 4, 16, {&v, lo, hi, std::move(fmt)});
      x += kCols[c].w;
      if (v != v0) apply(v);
      if (commit) {
        if (st::final()) done(desc);
        else preview(ec.song);
      }
    };
    int gi = int(std::min_element(std::begin(kGrids), std::end(kGrids),
                                  [&](double a, double b) { return std::abs(a - l.grid) < std::abs(b - l.grid); }) -
                 std::begin(kGrids));
    field(1, "##qua", gi, 0, 8, [](int v) { return std::to_string(kGridDen[v]); }, "Rotator: hyper lane grid",
          [&](int v) { l.grid = kGrids[v]; });
    if (g_params) {
      int dly = int(std::lround(l.delay * kPPQ));
      field(2, "##dly", dly, -kPPQ, kPPQ, {}, "Rotator: hyper lane delay", [&](int v) { l.delay = v / double(kPPQ); });
      if (l.hhat) st::text(x + (kCols[3].w - 8) / 2, vy, kCheck);
      if (st::hit("##hh", x, ry, kCols[3].w, lh)) {
        l.hhat = !l.hhat;
        done("Rotator: hyper lane hihat");
      }
      G::SetItemTooltip(ctx, "Hihat group: adjacent HHAT lanes are mutually exclusive");
      x += kCols[3].w;
      int ty = int(l.type);
      field(4, "##type", ty, 0, 5, [](int v) { return v ? std::string(status_name(kTypes[v])) : std::string("NOTE"); },
            "Rotator: hyper lane status", [&](int v) { l.type = kTypes[v]; });
      int ch = l.chan.value_or(0);
      field(5, "##cha", ch, 0, 16, [](int v) { return v ? std::to_string(v) : std::string("-"); }, "Rotator: hyper lane channel",
            [&](int v) { l.chan = v ? std::optional<int>(v) : std::nullopt; });
      if (has_data1(l.type)) {
        int d1 = l.data1.value_or(-1);
        EventType t = l.type;
        field(6, "##d1", d1, -1, 127,
              [t](int v) { return v < 0 ? std::string("ALL") : t == EventType::CC ? std::to_string(v) : note_name(v); },
              "Rotator: hyper lane data1", [&](int v) { l.data1 = v < 0 ? std::nullopt : std::optional<int>(v); });
      } else {
        x += kCols[6].w;
      }
      if (l.type == EventType::Note) {
        int len = std::max(1, int(std::lround(l.len * kPPQ)));
        field(7, "##len", len, 1, 16 * kPPQ, {}, "Rotator: hyper lane length", [&](int v) { l.len = v / double(kPPQ); });
      }
    }

    // Grid: dark field, step lines from the bottom, beat and bar lines full height
    double top = ry, h = lh;
    st::dark(cx, top, cw, h);
    double end_x = std::max(cx, x_of(plen));
    if (end_x < 640) st::fill(end_x, top, 640 - end_x, h, kInk);
    st::DrawClip gclip(cx, top, cw, h);
    if (l.grid > 0 && l.grid * pxq >= 3)
      for (double q = std::ceil(t0 / l.grid - 1e-9) * l.grid; q <= t0 + span; q += l.grid)
        st::fill(x_of(q), top + h * 2 / 3, 1, h / 3, kPaper);
    for (double q = std::ceil(t0 - 1e-9); q <= t0 + span; q += 1) {
      bool bar = std::fmod(q, kQnPerBar) == 0;
      st::fill(x_of(q) - bar, top, 1 + bar, h, kPaper);
    }
    auto y_of = [&](double v) { return top + h - (v - vlo) / double(vhi - vlo) * (h - 2); };
    double base = l.type == EventType::PitchBend ? y_of(0) : top + h;
    for (const Event& e : trk.events) {
      if (!lane_matches(l, e) || e.pos < t0 - 1e-9 || e.pos >= t0 + span) continue;
      double ex = x_of(e.pos) + 1, vy2 = y_of(event_value(e));
      st::fill(ex, top, kPen, h, kPaper);
      st::fill(ex, std::min(base, vy2), kPen, std::max(1.0, std::abs(base - vy2)), kInk);
    }
    gclip.pop();
    st::fill(cx, ry + lh, cw, 1, kPaper);

    G::PushID(ctx, "c");
    st::hit("##canvas", cx, top, cw, h);
    bool hov = st::hovered();
    G::PopID(ctx);
    double mt = std::clamp(t0 + (mx - cx) / pxq, std::max(0.0, t0), t0 + span);
    double mv = std::clamp(vlo + (top + h - my) / (h - 2) * (vhi - vlo), double(vlo), double(vhi));
    if (hov) info = l.name + "  " + bar_pos(mt) + "  " + std::to_string(int(std::lround(mv)));
    if (hov && g_g.mode == Mode::None) {
      bool left = G::IsMouseClicked(ctx, G::MouseButton_Left), right = G::IsMouseClicked(ctx, G::MouseButton_Right);
      if (left || right) {
        g_g = {left ? (ctrl ? Mode::Scale : alt ? Mode::InsertOnly : Mode::Paint) : (alt ? Mode::EraseGrid : Mode::Erase),
               i, mt, mv, my, 0, trk.events};
      }
    }
    if (g_g.lane == i && g_g.mode != Mode::None) {
      double tol = kPen / pxq;
      switch (g_g.mode) {
        case Mode::Paint:
          paint(trk, *set, i, {g_g.t, mt, g_g.v, mv, tol}, true, g_autoins || shift,
                g_fixvel ? std::optional<int>(g_vel) : std::nullopt);
          break;
        case Mode::InsertOnly:
          paint(trk, *set, i, {g_g.t, mt, g_g.v, mv, tol}, false, true, g_fixvel ? std::optional<int>(g_vel) : std::nullopt);
          break;
        case Mode::Erase:
        case Mode::EraseGrid:
          erase(trk, l, std::min(g_g.t, mt) - tol, std::max(g_g.t, mt) + tol, g_g.mode == Mode::EraseGrid);
          break;
        case Mode::Scale: {
          int total = int(std::lround((g_g.y0 - my) * (vhi - vlo) / 150.0));
          offset_lane(trk, l, total - g_g.applied);
          g_g.applied = total;
          break;
        }
        case Mode::None: break;
      }
      g_g.t = mt;
      g_g.v = mv;
      int button = g_g.mode == Mode::Erase || g_g.mode == Mode::EraseGrid ? G::MouseButton_Right : G::MouseButton_Left;
      if (!G::IsMouseDown(ctx, button)) {
        static const char* descs[] = {"", "Rotator: hyper draw", "Rotator: hyper insert", "Rotator: hyper erase",
                                      "Rotator: hyper erase on grid", "Rotator: hyper scale lane"};
        if (!same(g_g.before, trk.events)) done(descs[int(g_g.mode)]);
        g_g = {};
      }
    }
    G::PopID(ctx);
  }
  lanes_clip.pop();
  if (g_g.mode != Mode::None && !G::IsMouseDown(ctx, G::MouseButton_Left) && !G::IsMouseDown(ctx, G::MouseButton_Right)) {
    if (!same(g_g.before, trk.events)) done("Rotator: hyper edit");  // lane scrolled out of view mid-gesture
    g_g = {};
  }
  if (g_g.lane >= nl) g_g = {};
  if (g_rename >= nl) g_rename = -2;
  if (nl > vis) {
    double th = std::max(6.0, area * vis / nl), ty = kLanesY + (area - th) * g_top / std::max(1, nl - vis);
    st::fill(0, kLanesY, 4, area, kPaper);
    st::fill(1, ty, 2, th, kInk);
  }
  st::end(640, H);
  g_info = info;

  if (op_kind) {
    auto& lanes = set->lanes;
    switch (op_kind) {
      case 1: std::swap(lanes[op_lane], lanes[op_lane - 1]); break;
      case 2: std::swap(lanes[op_lane], lanes[op_lane + 1]); break;
      case 3: lanes.erase(lanes.begin() + op_lane); break;
      case 4: convert_lane(trk, lanes[op_lane], lanes[op_target], true); break;
      case 5: convert_lane(trk, lanes[op_lane], lanes[op_target], false); break;
      case 6: delete_hhat_duplicates(trk, *set, op_lane); break;
      case 7: erase(trk, lanes[op_lane], -1e18, 1e18); break;
    }
    static const char* descs[] = {"", "Rotator: move hyper lane", "Rotator: move hyper lane", "Rotator: delete hyper lane",
                                  "Rotator: copy hyper lane", "Rotator: convert hyper lane",
                                  "Rotator: delete duplicated hihats", "Rotator: clear hyper lane"};
    done(descs[op_kind]);
  }
}
