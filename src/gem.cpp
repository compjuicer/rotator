#include "gem.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <system_error>

#include "st.h"

#undef min
#undef max

namespace G = ImGui;
namespace fs = std::filesystem;
using st::Font;
using st::kInk;
using st::kPaper;

namespace gem {

namespace {

enum class Kind { None, Alert, Files, Overview, Notepad };
Kind g_kind = Kind::None;
ImGui_Context* g_ctx = nullptr;

struct {
  Icon icon = Icon::None;
  std::vector<std::string> lines, buttons;
  int def = 0;
  std::function<void(int)> done;
} g_alert;

struct Item {
  std::string name;
  bool dir;
};

struct {
  std::string op, dir, ext;
  bool must_exist = false, dir_active = false;
  std::vector<std::pair<std::string, std::string>> places;
  std::function<void(const std::string&)> done;
  std::vector<Item> items;
  int top = 0;
  char sel[256] = "", path[1024] = "";
} g_fs;

struct {
  int current = 0;
  std::function<void(int)> pick;
} g_ov;

struct {
  std::vector<char> buf;
  bool changed = false;
  std::function<void()> done;
} g_np;

const std::string kNone;

int g_focus = 0;  // frames left to keep focusing a newly opened dialog (a closing menu takes focus back)

// Child window over (x, y, w, h) of the page, which is disabled meanwhile; st keeps page coordinates inside it.
bool begin_box(const char* id, double x, double y, double w, double h) {
  double ox = st::sx(0), oy = st::sy(0), s = st::scale();
  G::SetCursorScreenPos(g_ctx, st::sx(x), st::sy(y));
  if (g_focus > 0) G::SetNextWindowFocus(g_ctx), --g_focus;
  G::PushStyleVar(g_ctx, G::StyleVar_WindowPadding, 0, 0);
  bool vis = G::BeginChild(g_ctx, id, w * s, h * s, 0, G::WindowFlags_NoScrollbar | G::WindowFlags_NoScrollWithMouse);
  G::PopStyleVar(g_ctx);
  if (!vis) {
    G::EndChild(g_ctx);
    return false;
  }
  G::SetCursorScreenPos(g_ctx, ox, oy);
  st::begin(g_ctx, s);
  return true;
}

void close() { g_kind = Kind::None; }

bool key(int k) { return G::IsKeyPressed(g_ctx, k, false); }
bool enter() { return key(G::Key_Enter) || key(G::Key_KeypadEnter); }

void box(double x, double y, double w, double h) {
  st::fill(x, y, w, h, kPaper);
  st::frame(x, y, w, h);
  st::frame(x + 3, y + 3, w - 6, h - 6);
}

// GEM button: 1 px border, the default one 3 px; inverts while pressed.
bool button(const char* id, double x, double y, double w, double h, const std::string& label, bool def = false) {
  bool clicked = st::hit(id, x, y, w, h);
  bool down = G::IsItemActive(g_ctx) && G::IsItemHovered(g_ctx);
  st::fill(x, y, w, h, down ? kInk : kPaper);
  st::frame(x, y, w, h);
  if (def) st::frame(x - 2, y - 2, w + 4, h + 4, kInk, 3);
  st::text(x + (w - st::text_w(label)) / 2, y + (h - 16) / 2, label, Font::Sys, down ? kPaper : kInk);
  return clicked;
}

void shape(double x, double y, double (*inset)(int), int d, int col) {
  for (int r = d; r < 32 - d; ++r) {
    double i = inset(r) + d;
    if (32 - 2 * i > 0) st::fill(x + i, y + r, 32 - 2 * i, 1, col);
  }
}

void icon(Icon k, double x, double y) {
  if (k == Icon::None) return;
  double (*f)(int) = k == Icon::Question ? [](int r) { return std::floor(r / 2.0); }
                     : k == Icon::Note   ? [](int r) { return std::floor((31 - r) / 2.0); }
                                         : [](int r) { return double(std::max({0, 9 - r, r - 22})); };
  shape(x, y, f, 0, kInk);
  shape(x, y, f, 2, kPaper);
  shape(x, y, f, 3, kInk);
  if (k == Icon::Question) st::text(x + 12, y + 2, "?", Font::Bold, kPaper);
  if (k == Icon::Note) st::text(x + 12, y + 14, "!", Font::Bold, kPaper);
  if (k == Icon::Stop) st::text(x + 4, y + 13, "STOP", Font::Small, kPaper);
}

void alert_frame() {
  auto& a = g_alert;
  size_t cols = 0, bw = 0;
  for (auto& l : a.lines) cols = std::max(cols, l.size());
  for (auto& b : a.buttons) bw = std::max(bw, b.size() * 8 + 16);
  double row_w = a.buttons.size() * (bw + 16) - 16;
  double w = std::max(56 + cols * 8.0 + 20, row_w + 32), h = 16 + std::max(32.0, a.lines.size() * 16.0) + 36 + 20 + 14;
  double x = std::round(320 - w / 2), y = std::round(200 - h / 2);
  if (!begin_box("##gem_alert", x, y, w, h)) return;
  int pressed = enter() ? a.def : key(G::Key_Escape) ? 0 : -1;
  box(x, y, w, h);
  icon(a.icon, x + 14, y + 14);
  for (size_t i = 0; i < a.lines.size(); ++i) st::text(x + 56, y + 14 + i * 16.0, a.lines[i]);
  double bx = x + w - 16 - row_w, by = y + h - 14 - 20;
  for (size_t i = 0; i < a.buttons.size(); ++i) {
    G::PushID(g_ctx, std::to_string(i).c_str());
    if (button("##b", bx + i * (bw + 16.0), by, bw, 20, a.buttons[i], int(i) == a.def)) pressed = int(i);
    G::PopID(g_ctx);
  }
  if (pressed >= 0) {
    auto done = std::move(a.done);
    close();
    if (done) done(pressed);
  }
  G::EndChild(g_ctx);
}

// ---------- file selector ----------

std::string lower(std::string s) {
  for (char& c : s) c = char(std::tolower((unsigned char)c));
  return s;
}

void set_dir(const fs::path& p) {
  std::error_code ec;
  fs::path d = fs::weakly_canonical(p, ec);
  g_fs.dir = (ec ? p : d).string();
  snprintf(g_fs.path, sizeof g_fs.path, "%s/*%s", g_fs.dir == "/" ? "" : g_fs.dir.c_str(), g_fs.ext.c_str());
  g_fs.items.clear();
  g_fs.top = 0;
  for (auto it = fs::directory_iterator(g_fs.dir, fs::directory_options::skip_permission_denied, ec);
       !ec && it != fs::directory_iterator(); it.increment(ec)) {
    std::string n = it->path().filename().string();
    if (n.empty() || n[0] == '.') continue;
    std::error_code e2;
    bool d = it->is_directory(e2);
    if (d || lower(it->path().extension().string()) == lower(g_fs.ext)) g_fs.items.push_back({n, d});
  }
  std::sort(g_fs.items.begin(), g_fs.items.end(), [](const Item& a, const Item& b) {
    return a.dir != b.dir ? a.dir : lower(a.name) < lower(b.name);
  });
}

void finish_files() {
  std::string name = g_fs.sel;
  if (name.empty()) return;
  fs::path p = name[0] == '/' ? fs::path(name) : fs::path(g_fs.dir) / name;
  std::error_code ec;
  if (fs::is_directory(p, ec)) {
    set_dir(p);
    g_fs.sel[0] = 0;
    return;
  }
  if (p.extension().empty()) p += g_fs.ext;
  if (g_fs.must_exist && !fs::exists(p, ec)) return;
  auto done = std::move(g_fs.done);
  close();
  if (done) done(p.string());
}

void edit_line(const char* id, double x, double y, double w, char* buf, int sz) {
  double s = st::scale();
  st::dotted_h(x, y + 16, w);
  G::SetCursorScreenPos(g_ctx, st::sx(x), st::sy(y));
  G::PushFont(g_ctx, st::sys_font(), 16 * s);
  G::PushStyleVar(g_ctx, G::StyleVar_FramePadding, 0, 0);
  G::PushStyleVar(g_ctx, G::StyleVar_FrameBorderSize, 0);
  G::SetNextItemWidth(g_ctx, w * s);
  G::InputText(g_ctx, id, buf, sz);
  G::PopStyleVar(g_ctx, 2);
  G::PopFont(g_ctx);
}

void diamond(double x, double y) {
  for (int r = 0; r < 7; ++r) {
    int hw = 3 - std::abs(r - 3);
    st::fill(x + 3 - hw, y + 5 + r, 2 * hw + 1, 1, kInk);
  }
}

void files_frame() {
  constexpr double X = 160, Y = 48, W = 320, H = 278, LX = X + 20, LY = Y + 98, LW = 176, RX = X + 216;
  constexpr int kRows = 9;
  if (!begin_box("##gem_files", X, Y, W, H)) return;
  auto& f = g_fs;
  if (key(G::Key_Escape)) {
    close();
    G::EndChild(g_ctx);
    return;
  }
  bool ok = enter() && !f.dir_active;
  box(X, Y, W, H);
  st::text(X + (W - st::text_w("FILE SELECTOR")) / 2, Y + 6, "FILE SELECTOR");
  st::fill(X + 3, Y + 23, W - 6, 1, kInk);
  st::text(X + 12, Y + 28, "Directory:");
  edit_line("##dir", X + 12, Y + 46, W - 24, f.path, sizeof f.path);
  f.dir_active = G::IsItemActive(g_ctx);
  if (G::IsItemDeactivated(g_ctx)) {
    fs::path p = f.path;
    if (p.filename().string().find('*') != std::string::npos) p = p.parent_path();
    std::error_code ec;
    set_dir(fs::is_directory(p, ec) ? p : fs::path(f.dir));
  }
  st::text(X + 12, Y + 72, "Selection:");
  if (g_focus) G::SetKeyboardFocusHere(g_ctx);
  edit_line("##sel", X + 100, Y + 72, W - 112, f.sel, sizeof f.sel);

  // list: header with closer and mask, rows, scroll bar
  st::fill(LX, LY, LW, 18 + kRows * 16 + 4, kPaper);
  st::frame(LX, LY, LW, 18 + kRows * 16 + 4);
  st::header(LX + 17, LY, LW - 17, 18, "*" + f.ext);
  st::frame(LX, LY, 18, 18);
  if (st::hit("##closer", LX, LY, 18, 18)) set_dir(fs::path(f.dir).parent_path());
  st::fill(LX + 5, LY + 5, 8, 8, kPaper);
  st::frame(LX + 5, LY + 5, 8, 8);
  st::fill(LX + 4, LY + 4, 10, 2, kInk);
  int n = int(f.items.size()), max_top = std::max(0, n - kRows);
  double wy = 0, wx = 0;
  G::GetMouseWheel(g_ctx, &wy, &wx);
  if (wy && G::IsWindowHovered(g_ctx)) f.top -= int(wy > 0 ? std::ceil(wy) : std::floor(wy));
  f.top = std::clamp(f.top, 0, max_top);
  double tx = LX + 4, tw = LW - 16 - 4;
  for (int r = 0; r < kRows; ++r) {
    double ry = LY + 20 + r * 16;
    int i = f.top + r;
    if (i >= n) {
      st::dotted_h(tx + 16, ry + 13, tw - 24);
      continue;
    }
    const Item& it = f.items[i];
    G::PushID(g_ctx, std::to_string(r).c_str());
    bool click = st::hit("##row", LX + 1, ry, LW - 18, 16), dbl = st::double_clicked();
    G::PopID(g_ctx);
    bool on = !it.dir && it.name == f.sel;
    if (on) st::fill(tx - 2, ry, tw, 16, kInk);
    if (it.dir) {
      diamond(tx + 1, ry);
      st::text(tx + 16, ry, it.name.substr(0, 17));
    } else {
      size_t dot = it.name.rfind('.');
      std::string stem = it.name.substr(0, dot), ext = it.name.substr(dot);
      stem = stem.size() > 13 ? stem.substr(0, 12) + "~" : stem;
      st::text(tx + 8, ry, stem, Font::Sys, on ? kPaper : kInk);
      st::text(tx + 8 + 13 * 8, ry, ext.substr(0, 4), Font::Sys, on ? kPaper : kInk);
    }
    if (click && it.dir) {
      set_dir(fs::path(f.dir) / it.name);
      break;
    }
    if (click) snprintf(f.sel, sizeof f.sel, "%s", it.name.c_str());
    if (dbl && !it.dir) ok = true;
  }
  double sbx = LX + LW - 17, sby = LY + 17, sbh = kRows * 16 + 5;
  st::fill(sbx, sby, 1, sbh, kInk);
  auto arrow = [&](const char* id, double y, bool up) {
    st::frame(sbx, y, 17, 17);
    double cx = sbx + 8;
    for (int r = 0; r < 5; ++r) st::fill(cx - r, y + (up ? 4 + r : 12 - r), 2 * r + 1, 1, kInk);
    st::fill(cx - 2, y + (up ? 9 : 4), 5, 4, kInk);
    return st::hit(id, sbx, y, 17, 17);
  };
  if (arrow("##up", sby, true)) --f.top;
  if (arrow("##down", sby + sbh - 17, false)) ++f.top;
  double ty = sby + 17, th = sbh - 34;
  double kh = n > kRows ? std::max(8.0, th * kRows / n) : th, ky = ty + (max_top ? (th - kh) * f.top / max_top : 0);
  st::dither(sbx + 1, ky, 15, kh);
  st::fill(sbx, ky, 17, 1, kInk);
  st::fill(sbx, ky + kh - 1, 17, 1, kInk);
  if (st::hit("##track", sbx, ty, 17, th)) {
    double mx, my;
    st::mouse(&mx, &my);
    f.top += my < ky ? -kRows : my > ky + kh ? kRows : 0;
  }
  f.top = std::clamp(f.top, 0, max_top);

  st::text(RX + 20, Y + 98, "PLACE:", Font::Bold);
  for (size_t i = 0; i < f.places.size(); ++i) {
    auto& [label, path] = f.places[i];
    double py = Y + 116 + i * 20.0;
    bool cur = path == f.dir;
    G::PushID(g_ctx, std::to_string(i).c_str());
    if (st::hit("##place", RX, py, 84, 18) && !path.empty()) set_dir(path);
    G::PopID(g_ctx);
    st::fill(RX, py, 84, 18, cur ? kInk : kPaper);
    st::frame(RX, py, 84, 18);
    st::text(RX + (84 - st::text_w(label)) / 2, py + 1, label, Font::Sys, cur ? kPaper : kInk);
    if (path.empty()) st::dither(RX + 1, py + 1, 82, 16, kPaper);
  }
  if (button("##ok", RX + 2, Y + 210, 80, 20, "OK", true)) ok = true;
  bool cancel = button("##cancel", RX + 2, Y + 242, 80, 20, "Cancel");
  if (cancel) close();
  else if (ok) finish_files();
  G::EndChild(g_ctx);
}

// ---------- pattern overview, notepad ----------

void overview_frame(const rotator::Song& song) {
  constexpr double X = 0, Y = 19, W = 640, H = 361, GY = 46;
  if (!begin_box("##gem_overview", X, Y, W, H)) return;
  int pick = 0;
  bool exit = key(G::Key_Escape) || enter();
  box(X, Y, W, H);
  st::header(X + 8, Y + 6, 540, 18, "PATTERN OVERVIEW", Font::Bold);
  if (button("##exit", 562, Y + 6, 68, 18, "EXIT", true)) exit = true;
  st::fill(X + 3, GY - 2, W - 6, 1, kInk);
  for (int c = 0; c < 5; ++c) {
    double cx = X + 4 + c * 126.4;
    if (c) st::fill(cx - 1, GY - 2, 1, 20 * 16 + 2, kInk);
    for (int r = 0; r < 20; ++r) {
      int p = c * 20 + r + 1;
      if (p > 99) break;
      double cy = GY + r * 16;
      G::PushID(g_ctx, std::to_string(p).c_str());
      if (st::hit("##p", cx, cy, 124, 16)) pick = p;
      G::PopID(g_ctx);
      bool cur = p == g_ov.current;
      if (cur) st::fill(cx, cy, 124, 16, kInk);
      int col = cur ? kPaper : kInk;
      std::string num = (p < 10 ? " " : "") + std::to_string(p);
      st::text(cx + 5, cy, num, Font::Bold, col);
      auto it = song.patterns.find(p);
      if (it == song.patterns.end()) {
        st::text(cx + 30, cy, "empty", Font::Sys, cur ? kPaper : kInk);
        if (!cur) st::dither(cx + 30, cy, 40, 16, kPaper);
        continue;
      }
      std::string name = it->second.name.substr(0, 9);
      std::string tracks = std::to_string(it->second.tracks.size());
      st::text(cx + 30, cy, name, Font::Sys, col);
      st::text(cx + 122 - st::text_w(tracks), cy, tracks, Font::Sys, col);
    }
  }
  if (pick || exit) {
    auto done = std::move(g_ov.pick);
    close();
    if (pick && done) done(pick);
  }
  G::EndChild(g_ctx);
}

void notepad_frame(rotator::Song& song) {
  constexpr double X = 80, Y = 40, W = 480, H = 320;
  if (!begin_box("##gem_notepad", X, Y, W, H)) return;
  bool exit = key(G::Key_Escape);  // before the editor sees Esc, which would revert its edit
  if (!exit) {
    box(X, Y, W, H);
    st::header(X + 8, Y + 6, W - 16, 18, "NOTEPAD", Font::Bold);
    st::frame(X + 8, Y + 30, W - 16, 248);
    double s = st::scale();
    G::SetCursorScreenPos(g_ctx, st::sx(X + 10), st::sy(Y + 32));
    G::PushFont(g_ctx, st::sys_font(), 16 * s);
    G::PushStyleVar(g_ctx, G::StyleVar_FramePadding, 2 * s, 0);
    G::PushStyleVar(g_ctx, G::StyleVar_FrameBorderSize, 0);
    if (g_focus) G::SetKeyboardFocusHere(g_ctx);
    if (G::InputTextMultiline(g_ctx, "##notes", g_np.buf.data(), int(g_np.buf.size()), (W - 20) * s, 244 * s)) {
      song.notes = g_np.buf.data();
      g_np.changed = true;
    }
    G::PopStyleVar(g_ctx, 2);
    G::PopFont(g_ctx);
    if (button("##exit", X + W - 92, Y + H - 32, 76, 20, "EXIT", true)) exit = true;
  }
  if (exit) {
    auto done = std::move(g_np.done);
    bool changed = g_np.changed;
    close();
    if (changed && done) done();
  }
  G::EndChild(g_ctx);
}

std::vector<std::string> wrap(const std::string& msg, size_t cols) {
  std::vector<std::string> out;
  size_t a = 0;
  while (a <= msg.size()) {
    size_t nl = std::min(msg.find('\n', a), msg.size());
    std::string para = msg.substr(a, nl - a);
    while (para.size() > cols) {
      size_t cut = para.rfind(' ', cols);
      if (cut == std::string::npos || cut == 0) cut = cols;
      out.push_back(para.substr(0, cut));
      para = para.substr(cut + (para[cut] == ' '));
    }
    out.push_back(para);
    a = nl + 1;
  }
  return out;
}

}  // namespace

void alert(Icon icon, const std::string& msg, std::vector<std::string> buttons, int def, std::function<void(int)> done) {
  g_alert = {icon, wrap(msg, 30), std::move(buttons), def, std::move(done)};
  g_kind = Kind::Alert;
  g_focus = 4;
}

void file_select(const std::string& op, const std::string& dir, const std::string& ext, bool must_exist,
                 std::vector<std::pair<std::string, std::string>> places, std::function<void(const std::string&)> done) {
  g_fs.op = op;
  g_fs.ext = ext;
  g_fs.must_exist = must_exist;
  g_fs.places = std::move(places);
  g_fs.done = std::move(done);
  g_fs.dir_active = false;
  g_fs.sel[0] = 0;
  set_dir(dir);
  g_kind = Kind::Files;
  g_focus = 4;
}

bool active() { return g_kind != Kind::None; }

const std::string& file_op() { return g_kind == Kind::Files ? g_fs.op : kNone; }

void overview(int current, std::function<void(int)> pick) {
  g_ov = {current, std::move(pick)};
  g_kind = Kind::Overview;
  g_focus = 4;
}

void notepad(std::function<void()> changed) {
  g_np.done = std::move(changed);
  g_np.changed = false;
  g_kind = Kind::Notepad;
  g_focus = 4;
}

void frame(ImGui_Context* ctx, rotator::Song& song) {
  g_ctx = ctx;
  switch (g_kind) {
    case Kind::Alert: alert_frame(); break;
    case Kind::Files: files_frame(); break;
    case Kind::Overview: overview_frame(song); break;
    case Kind::Notepad:
      if (g_focus == 4) {
        g_np.buf.assign(song.notes.begin(), song.notes.end());
        g_np.buf.resize(song.notes.size() + 16384, 0);
      }
      notepad_frame(song);
      break;
    case Kind::None: break;
  }
}

}  // namespace gem
