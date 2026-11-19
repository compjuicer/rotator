#pragma once
// Turns timestamped MIDI input into track events. No Reaper dependencies.
#include <map>
#include <utility>
#include <vector>

#include "model.h"

namespace rotator {

struct Recorder {
  double origin = 0;  // QN of the pattern start in the project
  double len = 0;     // QN; positions wrap at len when cycling, are dropped past it otherwise
  bool cycle = false;

  // qn = project position of the message. Returns true when a finished event was added to out.
  bool feed(int status, int d1, int d2, double qn, std::vector<Event>& out);
  // Ends notes still held at qn.
  void finish(double qn, std::vector<Event>& out);

 private:
  double rel(double qn) const;
  std::map<std::pair<int, int>, std::pair<size_t, double>> held_;  // (chan, pitch) -> (index in out, start rel)
};

}  // namespace rotator
