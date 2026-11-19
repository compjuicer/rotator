#pragma once
#include "model.h"
#include "reaper_plugin.h"

#include <string>

std::string track_guid(MediaTrack*);
MediaTrack* dest_track(const std::string& guid);  // nullptr if empty or missing
double bar_to_qn(int bar);
void render_to_project(const rotator::Song&);
void preview(const rotator::Song&);  // re-render without an undo point (live while a value slides)
void commit(ReaProject*, const rotator::Song&, const char* undo_desc);
bool capture_selected_item(ReaProject*, rotator::Track&);
