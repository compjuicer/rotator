#pragma once
#include "model.h"
#include "reaper_plugin.h"

void ui_init(void* (*get_func)(const char*));
void ui_set_open(bool);
bool ui_is_open();
void ui_frame(ReaProject*, rotator::Song&);
const char* ui_status();
void ui_set_gam(bool);
void ui_set_tab(int);
void ui_record();  // start recording into the selected track (next frame)
void ui_set_track(int);
