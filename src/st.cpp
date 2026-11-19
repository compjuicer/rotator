#include "st.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

#undef min
#undef max

namespace G = ImGui;

namespace st {

namespace {

ImGui_Context* g_ctx = nullptr;
ImGui_DrawList* g_dl = nullptr;
ImGui_Font *g_sys = nullptr, *g_small = nullptr;
ImGui_Image* g_tex[2] = {};
int g_tex_k = 0;
constexpr int kTile = 32;  // ST pixels
double g_s = 2, g_ox = 0, g_oy = 0;

struct Drag {
  unsigned id = 0;
  int orig = 0;      // value before the gesture
  double acc = 0;    // current value, fractional while sliding
  bool ctrl = false; // Ctrl + mouse gesture
};
Drag g_drag;
unsigned g_edit = 0;
char g_edit_buf[32];

ImGui_Font* font_of(Font f) { return f == Font::Small ? g_small : g_sys; }
double size_of(Font f) { return (f == Font::Small ? 6 : 16) * g_s; }
double char_w(Font f) { return f == Font::Small ? 6 : 8; }

unsigned hash(const char* s) {
  unsigned h = 2166136261u;
  for (; *s; ++s) h = (h ^ (unsigned char)*s) * 16777619u;
  return h;
}

}  // namespace

void attach_fonts(ImGui_Context* ctx) {
  g_sys = G::CreateFontFromFile(ROTATOR_FONT_DIR "/AtariST_8x16.ttf");
  g_small = G::CreateFontFromFile(ROTATOR_FONT_DIR "/Rotator_6x6.ttf");
  G::Attach(ctx, (ImGui_Resource*)g_sys);
  G::Attach(ctx, (ImGui_Resource*)g_small);
}

void begin(ImGui_Context* ctx, double scale) {
  g_ctx = ctx;
  g_s = scale;
  g_dl = G::GetWindowDrawList(ctx);
  G::GetCursorScreenPos(ctx, &g_ox, &g_oy);
}

DrawClip::DrawClip(double x, double y, double w, double h) {
  G::DrawList_PushClipRect(g_dl, sx(x), sy(y), sx(x + w), sy(y + h), true);
}

void DrawClip::pop() {
  if (active) G::DrawList_PopClipRect(g_dl);
  active = false;
}

void end(double w, double h) {
  G::SetCursorScreenPos(g_ctx, g_ox, g_oy);
  G::Dummy(g_ctx, w * g_s, h * g_s);
}

double scale() { return g_s; }
double sx(double x) { return g_ox + x * g_s; }
double sy(double y) { return g_oy + y * g_s; }
bool fonts_ok() { return g_sys && g_small; }

void fill(double x, double y, double w, double h, int col) {
  G::DrawList_AddRectFilled(g_dl, sx(x), sy(y), sx(x + w), sy(y + h), col);
}

void frame(double x, double y, double w, double h, int col, double t) {
  fill(x, y, w, t, col);
  fill(x, y + h - t, w, t, col);
  fill(x, y, t, h, col);
  fill(x + w - t, y, t, h, col);
}

void stripes(double x, double y, double w, double h, int col) {
  for (double r = y + 1; r < y + h; r += 2) fill(x, r, w, 1, col);
}

void dots(double x, double y, double w, double h) {
  for (double r = y; r < y + h; r += 2)
    for (double c = x + (int(r - y) / 2 % 2); c < x + w; c += 2) fill(c, r, 1, 1, kInk);
}

double text_w(const std::string& s, Font f) { return s.size() * char_w(f); }

void text(double x, double y, const std::string& s, Font f, int col) {
  G::DrawList_AddTextEx(g_dl, font_of(f), size_of(f), sx(x), sy(y), col, s.c_str());
  if (f == Font::Bold) G::DrawList_AddTextEx(g_dl, font_of(f), size_of(f), sx(x) + std::max(1.0, g_s * 0.5), sy(y), col, s.c_str());
}

ImGui_Font* sys_font() { return g_sys; }
std::string debug_state() { return std::to_string(g_drag.id) + "/" + std::to_string(g_drag.acc) + (g_drag.ctrl ? "c" : ""); }

void text_fit(double x, double y, double w, const std::string& s, int col) {
  double tw = text_w(s, Font::Small);
  if (tw <= w) {
    text(x + (w - tw) / 2, y, s, Font::Small, col);
    return;
  }
  double adv = std::max(4.0, w / s.size()), x0 = x + (w - s.size() * adv) / 2;
  for (size_t i = 0; i < s.size(); ++i) text(x0 + i * adv, y, std::string(1, s[i]), Font::Small, col);
}
double line_h(Font f) { return f == Font::Small ? 6 : 16; }

void dotted_v(double x, double y, double h) {
  for (double r = y; r < y + h; r += 2) fill(x, r, 1, 1, kInk);
}

void dotted_h(double x, double y, double w) {
  for (double c = x; c < x + w; c += 2) fill(c, y, 1, 1, kInk);
}

namespace {
// Minimal RGBA PNG (stored deflate), for CreateImageFromMem.
std::string png(int w, int h, const std::vector<uint32_t>& rgba) {
  std::string raw;
  for (int y = 0; y < h; ++y) {
    raw += '\0';
    for (int x = 0; x < w; ++x)
      for (int b = 3; b >= 0; --b) raw += char(rgba[y * w + x] >> (b * 8));
  }
  auto be = [](std::string& s, uint32_t v) {
    for (int b = 3; b >= 0; --b) s += char(v >> (b * 8));
  };
  std::string z = "\x78\x01";
  for (size_t i = 0; i < raw.size(); i += 65535) {
    size_t n = std::min<size_t>(65535, raw.size() - i);
    z += char(i + n == raw.size());
    z += char(n), z += char(n >> 8), z += char(~n), z += char(~n >> 8);
    z += raw.substr(i, n);
  }
  uint32_t a = 1, b = 0;
  for (unsigned char c : raw) a = (a + c) % 65521, b = (b + a) % 65521;
  be(z, b << 16 | a);
  std::string out = "\x89PNG\r\n\x1a\n";
  auto chunk = [&](const char* type, const std::string& data) {
    be(out, uint32_t(data.size()));
    std::string body = type + data;
    uint32_t crc = 0xFFFFFFFF;
    for (unsigned char c : body) {
      crc ^= c;
      for (int k = 0; k < 8; ++k) crc = crc >> 1 ^ (0xEDB88320 & (0u - (crc & 1)));
    }
    out += body;
    be(out, ~crc);
  };
  std::string hdr;
  be(hdr, w), be(hdr, h);
  hdr += std::string("\x08\x06\x00\x00\x00", 5);
  chunk("IHDR", hdr);
  chunk("IDAT", z);
  chunk("IEND", "");
  return out;
}

// Pattern tiles at device resolution, so 1 ST pixel stays crisp. 0: 50% checker, 1: 25% dots.
ImGui_Image* tile(int which) {
  int k = std::max(1, int(std::lround(g_s * G::GetWindowDpiScale(g_ctx))));
  if (k != g_tex_k) g_tex[0] = g_tex[1] = nullptr, g_tex_k = k;
  ImGui_Image*& t = g_tex[which];
  if (t && G::ValidatePtr(t, "ImGui_Image*")) return t;
  int n = kTile * k;
  std::vector<uint32_t> px(n * n);
  for (int y = 0; y < n; ++y)
    for (int x = 0; x < n; ++x) {
      int X = x / k, Y = y / k;
      bool on = which == 0 ? (X + Y) % 2 == 0 : Y % 2 == 0 && (X + Y / 2) % 2 == 0;
      px[y * n + x] = on ? 0xFFFFFFFF : 0;
    }
  std::string data = png(n, n, px);
  t = G::CreateImageFromMem(data.data(), int(data.size()));
  return t;
}

void pattern(double x, double y, double w, double h, int which, int col) {
  if (w <= 0 || h <= 0) return;
  ImGui_Image* t = tile(which);
  G::DrawList_PushClipRect(g_dl, sx(x), sy(y), sx(x + w), sy(y + h), true);
  for (double r = std::floor(y / kTile) * kTile; r < y + h; r += kTile)
    for (double c = std::floor(x / kTile) * kTile; c < x + w; c += kTile)
      G::DrawList_AddImage(g_dl, t, sx(c), sy(r), sx(c + kTile), sy(r + kTile), 0, 0, 1, 1, col);
  G::DrawList_PopClipRect(g_dl);
}
}  // namespace

void dither(double x, double y, double w, double h, int col) { pattern(x, y, w, h, 0, col); }

void dark(double x, double y, double w, double h) {
  fill(x, y, w, h, kInk);
  pattern(x, y, w, h, 1, kPaper);
}

void mouse(double* x, double* y) {
  G::GetMousePos(g_ctx, x, y);
  *x = (*x - g_ox) / g_s;
  *y = (*y - g_oy) / g_s;
}

void text_box(double x, double y, const std::string& s, Font f, bool inverse) {
  double h = line_h(f);
  fill(x - 1, y - (f == Font::Small ? 1 : 0), text_w(s, f) + 2 + (f == Font::Bold ? 1 : 0), h + (f == Font::Small ? 2 : 0), inverse ? kInk : kPaper);
  text(x, y, s, f, inverse ? kPaper : kInk);
}

bool hit(const char* id, double x, double y, double w, double h) {
  G::SetCursorScreenPos(g_ctx, sx(x), sy(y));
  G::InvisibleButton(g_ctx, id, std::max(1.0, w * g_s), std::max(1.0, h * g_s),
                     G::ButtonFlags_MouseButtonLeft | G::ButtonFlags_MouseButtonRight);
  return G::IsItemClicked(g_ctx, G::MouseButton_Left);
}

bool hovered() { return G::IsItemHovered(g_ctx); }
bool right_clicked() { return G::IsItemClicked(g_ctx, G::MouseButton_Right); }
bool double_clicked() { return G::IsItemHovered(g_ctx) && G::IsMouseDoubleClicked(g_ctx, G::MouseButton_Left); }

bool button(const char* id, double x, double y, double w, double h, const std::string& label, const std::string& hint,
            bool on, Font f) {
  bool clicked = hit(id, x, y, w, h);
  bool down = G::IsItemActive(g_ctx) && G::IsMouseDown(g_ctx, G::MouseButton_Left);
  double d = down ? 1 : 0;
  fill(x + 2, y + 2, w, h, kInk);
  fill(x + d, y + d, w, h, on ? kInk : kPaper);
  if (!on) stripes(x + d + 2, y + d + 2, w - 4, h - 4);
  frame(x + d, y + d, w, h, on ? kPaper : kInk);
  double lh = line_h(f);
  double tx = x + d + (w - text_w(label, f)) / 2, ty = y + d + (h - lh) / 2 + (hint.empty() ? 0 : 2);
  if (on) text(tx, ty, label, f, kPaper);
  else text_box(tx, ty, label, f, false);
  if (!hint.empty()) text_box(x + d + 3, y + d + 2, hint, Font::Small, on);
  return clicked;
}

void header(double x, double y, double w, double h, const std::string& label, Font f) {
  fill(x, y, w, h, kPaper);
  stripes(x + 1, y + 1, w - 2, h - 2);
  frame(x, y, w, h);
  double lh = line_h(f);
  double tw = text_w(label, f);
  if (f == Font::Small && tw > w - 2) {
    fill(x + (w - std::min(tw, label.size() * 5.0)) / 2 - 1, y + (h - lh) / 2 - 1, std::min(tw, label.size() * 5.0) + 1, lh + 2, kPaper);
    text_fit(x, y + (h - lh) / 2, w, label);
    return;
  }
  text_box(x + (w - tw) / 2, y + (h - lh) / 2, label, f, false);
}

bool value(const char* id, double x, double y, double w, double h, Value v, Font f, bool right_align) {
  unsigned key = hash(id) ^ unsigned(std::lround(x * 7919 + y * 104729));
  bool changed = false;
  if (g_edit == key) {
    G::SetCursorScreenPos(g_ctx, sx(x), sy(y));
    G::SetNextItemWidth(g_ctx, w * g_s);
    G::SetKeyboardFocusHere(g_ctx);
    if (G::InputText(g_ctx, (std::string("##e") + id).c_str(), g_edit_buf, sizeof g_edit_buf,
                     G::InputTextFlags_EnterReturnsTrue | G::InputTextFlags_AutoSelectAll)) {
      *v.v = std::clamp(std::atoi(g_edit_buf), v.lo, v.hi);
      changed = true;
      g_edit = 0;
    } else if (G::IsKeyPressed(g_ctx, G::Key_Escape) || (!G::IsItemActive(g_ctx) && !G::IsItemActivated(g_ctx) &&
                                                          G::IsMouseClicked(g_ctx, G::MouseButton_Left))) {
      g_edit = 0;
    }
    return changed;
  }
  hit(id, x, y, w, h);
  bool hov = G::IsItemHovered(g_ctx), ctrl = G::GetKeyMods(g_ctx) & G::Mod_Ctrl;
  bool l = G::IsMouseDown(g_ctx, G::MouseButton_Left), r = G::IsMouseDown(g_ctx, G::MouseButton_Right);
  if (!g_drag.id && hov && ctrl && !l && !r) g_drag = {key, *v.v, double(*v.v), true};
  if (G::IsItemActivated(g_ctx) && !ctrl) {
    int step = G::IsMouseClicked(g_ctx, G::MouseButton_Right) ? -1 : 1;
    g_drag = {key, *v.v, double(std::clamp(*v.v + step, v.lo, v.hi)), false};
  }
  if (g_drag.id == key) {
    if (g_drag.ctrl && G::IsMouseClicked(g_ctx, G::MouseButton_Left)) g_drag.acc = std::clamp(v.def, v.lo, v.hi);
    if (g_drag.ctrl || (slider_mode && (l || r))) {
      double dx = 0, dy = 0;
      G::GetMouseDelta(g_ctx, &dx, &dy);
      g_drag.acc = std::clamp(g_drag.acc - dy / (4 * g_s) * (l && r ? 6 : 1), double(v.lo), double(v.hi));
    }
    *v.v = int(std::floor(g_drag.acc + 0.5));
    if (g_drag.ctrl ? !ctrl : !(l || r)) {
      changed = *v.v != g_drag.orig;
      g_drag = {};
    }
  }
  if (double_clicked()) {
    g_edit = key;
    snprintf(g_edit_buf, sizeof g_edit_buf, "%d", *v.v);
  }
  std::string s = v.fmt ? v.fmt(*v.v) : std::to_string(*v.v);
  double lh = line_h(f);
  text(right_align ? x + w - text_w(s, f) : x, y + (h - lh) / 2, s, f);
  return changed;
}

}  // namespace st
