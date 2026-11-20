// REAPER entry point. Dev builds (ROTATOR_DEV) load the core from a copy of rotator_core.dylib so it can be
// hot-reloaded, and poll a file bridge for tools/reaper. Release builds link the core in.
#ifdef ROTATOR_DEV
#define REAPERAPI_IMPLEMENT
#include "reaper_api.h"
#else
#include "reaper_core_api.h"
#endif
#include "core_abi.h"

#ifndef ROTATOR_DEV
extern "C" const RotatorCore* rotator_core_init(void* (*get_func)(const char*));
#endif

#include <dlfcn.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

void* (*g_get_func)(const char*) = nullptr;
const RotatorCore* g_core = nullptr;
void* g_handle = nullptr;
int g_loads = 0;

struct Action {
  std::string id, desc;
  custom_action_register_t reg;
  int cmd = 0;
};
std::deque<Action> g_actions;
int g_reload_cmd = -1;

struct Result {
  bool ok;
  std::string text;
};

void log(const std::string& s) { ShowConsoleMsg(("Rotator: " + s + "\n").c_str()); }

int action_index(const Action& a) {
  for (int i = 0; i < g_core->num_actions; ++i)
    if (a.id == g_core->actions[i].id) return i;
  return -1;
}

#ifdef ROTATOR_DEV
custom_action_register_t g_reload_reg = {0, "ROTATOR_RELOAD", "Rotator: Reload core", nullptr};

Result load_core(const RotatorCore*& core, void*& handle) {
  fs::path dir = fs::path(ROTATOR_CORE_PATH).parent_path() / "reload";
  fs::path copy = dir / ("core-" + std::to_string(++g_loads) + "-" + std::to_string(getpid()) + ".dylib");
  std::error_code ec;
  fs::create_directories(dir, ec);
  if (!fs::copy_file(ROTATOR_CORE_PATH, copy, fs::copy_options::overwrite_existing, ec))
    return {false, "copy " ROTATOR_CORE_PATH ": " + ec.message()};
  handle = dlopen(copy.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!handle) return {false, dlerror()};
  auto init = reinterpret_cast<RotatorCoreInit>(dlsym(handle, "rotator_core_init"));
  core = init ? init(g_get_func) : nullptr;
  if (!core) {
    dlclose(handle);
    return {false, "rotator_core_init missing or failed"};
  }
  return {true, ""};
}

Result reload() {
  const RotatorCore* core = nullptr;
  void* handle = nullptr;
  std::string state = g_core->snapshot();
  Result r = load_core(core, handle);
  if (!r.ok) return r;
  core->restore(state.c_str());
  const RotatorCore* old = g_core;
  void* old_handle = g_handle;
  g_core = core;
  g_handle = handle;
  old->shutdown();
  dlclose(old_handle);
  for (auto& a : g_actions)
    if (action_index(a) < 0) r.text += "action " + a.id + " no longer in core\n";
  for (int i = 0; i < core->num_actions; ++i)
    if (std::none_of(g_actions.begin(), g_actions.end(), [&](const Action& a) { return a.id == core->actions[i].id; }))
      r.text += std::string("new action ") + core->actions[i].id + " needs a REAPER restart\n";
  if (!r.text.empty()) log(r.text);
  return r;
}

#endif

bool on_action(KbdSectionInfo*, int command, int, int, int, HWND) {
#ifdef ROTATOR_DEV
  if (command == g_reload_cmd) {
    Result r = reload();
    log(r.ok ? "core reloaded" : "reload failed: " + r.text);
    return true;
  }
#endif
  for (auto& a : g_actions)
    if (a.cmd == command) {
      int i = action_index(a);
      if (i >= 0) g_core->run_action(i);
      return true;
    }
  return false;
}

bool process_line(const char* line, ProjectStateContext* ctx, bool, project_config_extension_t*) {
  return g_core->config_line(line, ctx);
}
void save_config(ProjectStateContext* ctx, bool, project_config_extension_t*) { g_core->config_save(ctx); }
void begin_load(bool, project_config_extension_t*) { g_core->config_begin_load(); }
project_config_extension_t g_config = {process_line, save_config, begin_load, nullptr};

#ifdef ROTATOR_DEV
const fs::path kBridge = ROTATOR_BRIDGE_DIR;

std::string read_file(const fs::path& p) {
  std::ifstream f(p, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

void write_file(const fs::path& p, const std::string& s) { std::ofstream(p, std::ios::binary) << s; }

std::string lua_quote(const std::string& s) {
  std::string q = "\"";
  for (char c : s) q += (c == '"' || c == '\\') ? std::string("\\") + c : std::string(1, c);
  return q + "\"";
}

Result run_lua(const std::string& seq, const std::string& body) {
  fs::path script = kBridge / ("run-" + seq + ".lua"), user = kBridge / ("run-" + seq + ".body.lua");
  fs::path out = kBridge / ("res-" + seq + ".lua.out"), err = kBridge / ("res-" + seq + ".lua.err");
  write_file(user, body);
  write_file(script,
             "local OUT, ERR = " + lua_quote(out.string()) + ", " + lua_quote(err.string()) + "\n"
             "function out(...)\n"
             "  local t = {}\n"
             "  for i = 1, select('#', ...) do t[#t + 1] = tostring(select(i, ...)) end\n"
             "  local f = io.open(OUT, 'a') f:write(table.concat(t, '\\t'), '\\n') f:close()\n"
             "end\n"
             "local fn, e = loadfile(" + lua_quote(user.string()) + ")\n"
             "local ok = fn ~= nil\n"
             "if ok then ok, e = xpcall(fn, debug.traceback) end\n"
             "if not ok then local f = io.open(ERR, 'w') f:write(tostring(e)) f:close() end\n");
  int cmd = AddRemoveReaScript(true, 0, script.c_str(), true);
  if (cmd) Main_OnCommand(cmd, 0);
  AddRemoveReaScript(false, 0, script.c_str(), true);
  Result r{cmd != 0, cmd ? read_file(out) : "AddRemoveReaScript failed"};
  std::error_code ec;
  if (fs::exists(err, ec)) r = {false, "lua: " + read_file(err) + "\n" + r.text};
  for (auto& p : {script, user, out, err}) fs::remove(p, ec);
  return r;
}

Result run_command(const std::string& seq, const std::string& text) {
  std::string first = text.substr(0, text.find('\n'));
  std::string rest = first.size() < text.size() ? text.substr(first.size() + 1) : "";
  while (!first.empty() && isspace((unsigned char)first.back())) first.pop_back();
  std::string cmd = first.substr(0, first.find(' '));
  std::string arg = cmd.size() < first.size() ? first.substr(cmd.size() + 1) : "";

  if (cmd == "ping")
    return {true, "loader " __DATE__ " " __TIME__ "\ncore " + std::string(g_core->build_time) + "\n"};
  if (cmd == "reload") return reload();
  if (cmd == "dump") return {true, g_core->dump()};
  if (cmd == "lua") return run_lua(seq, rest);
  if (cmd == "action") {
    char* end = nullptr;
    long n = strtol(arg.c_str(), &end, 10);
    int id = !arg.empty() && *end == 0 ? int(n) : NamedCommandLookup(arg.c_str());
    if (!id) return {false, "unknown action " + arg};
    if (!on_action(nullptr, id, 0, 0, 0, nullptr)) Main_OnCommand(id, 0);
    return {true, ""};
  }
  return {false, "unknown command " + first};
}

void poll_bridge() {
  std::error_code ec;
  std::vector<std::pair<unsigned long long, std::string>> cmds;
  for (auto& e : fs::directory_iterator(kBridge, ec)) {
    std::string name = e.path().filename().string();
    if (name.size() > 8 && !name.compare(0, 4, "cmd-") && !name.compare(name.size() - 4, 4, ".txt")) {
      std::string seq = name.substr(4, name.size() - 8);
      if (!seq.empty() && seq.size() <= 19 && seq.find_first_not_of("0123456789") == std::string::npos) cmds.push_back({std::stoull(seq), seq});
    }
  }
  std::sort(cmds.begin(), cmds.end());
  for (auto& [n, seq] : cmds) {
    fs::path cmd = kBridge / ("cmd-" + seq + ".txt");
    std::string text = read_file(cmd);
    fs::remove(cmd, ec);
    Result r = run_command(seq, text);
    fs::path tmp = kBridge / ("res-" + seq + ".tmp");
    write_file(tmp, (r.ok ? "ok\n" : "error: ") + r.text);
    fs::rename(tmp, kBridge / ("res-" + seq + ".txt"), ec);
  }
}

void on_timer() {
  static int tick = 0;
  static bool busy = false;
  if (busy || ++tick % 8) return;
  busy = true;
  poll_bridge();
  busy = false;
}
#endif

}  // namespace

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE, reaper_plugin_info_t* rec) {
  if (!rec) {
    if (g_core) g_core->shutdown();
    return 0;
  }
  if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc) return 0;
  g_get_func = rec->GetFunc;
#ifdef ROTATOR_DEV
  if (REAPERAPI_LoadAPI(rec->GetFunc)) return 0;
  std::error_code ec;
  fs::remove_all(fs::path(ROTATOR_CORE_PATH).parent_path() / "reload", ec);
  Result r = load_core(g_core, g_handle);
  if (!r.ok) {
    log("core load failed: " + r.text);
    return 0;
  }
#else
  g_core = rotator_core_init(rec->GetFunc);
  if (!g_core) return 0;
#endif
  for (int i = 0; i < g_core->num_actions; ++i) {
    Action& a = g_actions.emplace_back();
    a.id = g_core->actions[i].id;
    a.desc = g_core->actions[i].desc;
    a.reg = {0, a.id.c_str(), a.desc.c_str(), nullptr};
    a.cmd = rec->Register("custom_action", &a.reg);
  }
  rec->Register("hookcommand2", (void*)on_action);
  rec->Register("projectconfig", &g_config);
#ifdef ROTATOR_DEV
  g_reload_cmd = rec->Register("custom_action", &g_reload_reg);
  fs::create_directories(kBridge, ec);
  rec->Register("timer", (void*)on_timer);
#endif
  return 1;
}
