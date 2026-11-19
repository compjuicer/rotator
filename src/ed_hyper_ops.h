#pragma once
// Hyper Edit logic: lanes over one track's events. Times in QN.
#include <optional>
#include <string>
#include <utility>

#include "model.h"

namespace rotator {

std::pair<int, int> value_range(EventType);
int event_value(const Event&);           // the value drawn as beam height
void set_event_value(Event&, int);       // clamped to value_range

std::string lane_label(EventType, int data1);
HyperSet default_hyper_set();
HyperLane lane_from_event(const Event&);
int add_unmatched_lanes(HyperSet&, const Track&);  // one lane per distinct unmatched event; returns lanes added
bool can_insert(const HyperLane&);                 // false for ALL lanes of types that need data1
std::pair<int, int> hhat_group(const HyperSet&, int lane);  // inclusive lane range; {lane, lane} if not hhat

// One mouse step: from (t0, v0) to (t1, v1). Events within tol of the swept span are hit.
struct Stroke {
  double t0, t1, v0, v1, tol;
};

// change: hit events take the stroke value (or fixed, for notes). insert: empty grid steps
// (nearest step to the span) get a new event; inserted notes clear hhat group-mates at that position.
void paint(Track&, const HyperSet&, int lane, const Stroke&, bool change, bool insert, std::optional<int> fixed = {});
// grid_only: only events exactly on the lane grid. Returns number erased.
int erase(Track&, const HyperLane&, double lo, double hi, bool grid_only = false);
void offset_lane(Track&, const HyperLane&, int delta);  // clamped
// Rewrites (or copies, if copy) the lane's events to the destination definition.
void convert_lane(Track&, const HyperLane& from, const HyperLane& to, bool copy);
// Each event takes the definition of the same-numbered lane in `to` as the first lane of `from` it matches.
void convert_set(Track&, const HyperSet& from, const HyperSet& to);
int delete_hhat_duplicates(Track&, const HyperSet&, int lane);
void delete_hyper_set(Song&, int index);

}  // namespace rotator
