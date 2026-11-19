#pragma once
// C ABI between the loader (reaper_rotator.dylib) and the hot-reloadable core (rotator_core.dylib).
// Returned strings are owned by the core and valid until the next call into it.

class ProjectStateContext;

extern "C" {

struct RotatorAction {
  const char* id;
  const char* desc;
};

struct RotatorCore {
  int num_actions;
  const RotatorAction* actions;
  void (*run_action)(int index);
  bool (*config_line)(const char* line, ProjectStateContext*);
  void (*config_save)(ProjectStateContext*);
  void (*config_begin_load)();
  const char* (*snapshot)();
  void (*restore)(const char*);
  const char* (*dump)();
  const char* build_time;
  void (*shutdown)();
};

typedef const RotatorCore* (*RotatorCoreInit)(void* (*get_func)(const char*));
}
