#pragma once
// Interface between the Rotator window (ui.cpp) and the pattern editors (ed_*.cpp).
#include <functional>

#include "model.h"
#include "reaper_plugin.h"
#include "reaper_imgui_functions.h"

struct EditCtx {
  ImGui_Context* ctx;
  ReaProject* proj;
  rotator::Song& song;
  int pattern, track;
  rotator::Track& trk;                         // the edited track; mutate it directly, then call commit
  std::function<void(const char*)> commit;     // one undo point + re-render; call once per finished gesture
};

inline int g_format = 3;  // display format index into 1/4 1/8 1/12 1/16 1/24 1/32 1/48 1/64 (box row FORMAT)

void event_editor(EditCtx&);
void matrix_editor(EditCtx&);
void hyper_editor(EditCtx&);
