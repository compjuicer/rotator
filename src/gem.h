#pragma once
// GEM desktop dialogs drawn with the st toolkit: alerts, file selector, pattern overview, notepad.
// One dialog at a time. Open with the functions below; frame() draws it over the page, which the caller
// disables while active().
#include <functional>
#include <string>
#include <vector>

#include "model.h"
#include "reaper_imgui_functions.h"

namespace gem {

enum class Icon { None, Question, Stop, Note };

// Return presses the default button, Esc the first one. done gets the button index.
void alert(Icon, const std::string& msg, std::vector<std::string> buttons, int def, std::function<void(int)> done);

// TOS item selector. op: menu-bar caption ("Load SONG"); ext: ".ROT". done gets the chosen path.
void file_select(const std::string& op, const std::string& dir, const std::string& ext, bool must_exist,
                 std::vector<std::pair<std::string, std::string>> places, std::function<void(const std::string&)> done);
const std::string& file_op();  // the file selector's op while it is open, else empty

void overview(int current, std::function<void(int)> pick);
void notepad(std::function<void()> changed);

bool active();
void frame(ImGui_Context*, rotator::Song&);  // after the page, inside its window, with st::begin at the page origin

}  // namespace gem
