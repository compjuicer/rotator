#pragma once
// Atari ST look of the original. Layout is in ST screen pixels (640x400 mono), scaled by s.
#include <functional>
#include <string>

#include "reaper_imgui_functions.h"

namespace st {

inline int kInk = 0x000000FF, kPaper = 0xFFFFFFFF;  // swapped by set_inverse

inline void set_inverse(bool on) {
  kInk = on ? 0xFFFFFFFF : 0x000000FF;
  kPaper = on ? 0x000000FF : 0xFFFFFFFF;
}

enum class Font { Sys, Small, Bold };  // 8x16 lists, thin ~6x6 captions, GEM-bold 8x16 values

struct Value {
  int* v;
  int lo, hi;
  std::function<std::string(int)> fmt;  // default: number
  int def = 0;                          // Ctrl+click resets to this (clamped to lo..hi)
};

inline bool slider_mode = true;  // Flags > Mouse as Slider: press and slide changes values

void attach_fonts(ImGui_Context*);         // once per context, before its first frame
void begin(ImGui_Context*, double scale);  // origin = cursor screen pos
void end(double w, double h);              // reserves w x h ST pixels in the layout
double scale();
double sx(double x);
double sy(double y);
bool fonts_ok();

void fill(double x, double y, double w, double h, int col);
void frame(double x, double y, double w, double h, int col = kInk, double t = 1);
void stripes(double x, double y, double w, double h, int col = kInk);
void dots(double x, double y, double w, double h);
void text(double x, double y, const std::string&, Font = Font::Sys, int col = kInk);
double text_w(const std::string&, Font = Font::Sys);
void text_box(double x, double y, const std::string&, Font, bool inverse);  // text on its own paper box

// Interactive. Hit areas are invisible buttons placed at ST coordinates.
bool hit(const char* id, double x, double y, double w, double h);  // true on left click
bool hovered();
bool right_clicked();
bool double_clicked();

bool button(const char* id, double x, double y, double w, double h, const std::string& label,
            const std::string& hint = "", bool on = false, Font = Font::Sys);
void header(double x, double y, double w, double h, const std::string& label, Font = Font::Sys);

// Left click +1, right click -1. Mouse as Slider: hold either button and slide up/down (both buttons = faster).
// Ctrl + mouse: hover, hold Ctrl and slide; commits on Ctrl release; Ctrl+click resets. Double-click types a value.
// Returns true on every change: while sliding (final() false, preview only) and when the gesture ends (final() true).
bool final();
bool value(const char* id, double x, double y, double w, double h, Value, Font = Font::Bold, bool right_align = true);

ImGui_Font* sys_font();
std::string debug_state();
void text_fit(double x, double y, double w, const std::string&, int col = kInk);  // small font, squeezed to w, centred

struct DrawClip {
  DrawClip(double x, double y, double w, double h);
  void pop();
  bool active = true;
};
double line_h(Font);
void dotted_v(double x, double y, double h);
void dotted_h(double x, double y, double w);
void dither(double x, double y, double w, double h, int col = kInk);  // 50% checker in col
void dark(double x, double y, double w, double h);                    // black with 25% white dots
void mouse(double* x, double* y);                                     // pointer in ST pixels

}  // namespace st
