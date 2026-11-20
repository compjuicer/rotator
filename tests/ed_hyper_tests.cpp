#include "doctest.h"
#include "ed_hyper_ops.h"

#include <algorithm>

using namespace rotator;

static Event nt(double pos, int pitch, int vel = 100, int chan = 10) { return {EventType::Note, pos, chan, pitch, vel, 0.1}; }
static Event cc(double pos, int ctl, int val) { return {EventType::CC, pos, 1, ctl, val, 0}; }

static HyperLane lane(EventType type, std::optional<int> data1, double grid = 0.25, bool hhat = false) {
  HyperLane l;
  l.type = type;
  l.data1 = data1;
  l.grid = grid;
  l.hhat = hhat;
  return l;
}

static int count(const Track& t, const HyperLane& l) {
  return int(std::count_if(t.events.begin(), t.events.end(), [&](const Event& e) { return lane_matches(l, e); }));
}

TEST_CASE("lane matching: type, data1 or ALL, channel or any") {
  HyperLane kick = lane(EventType::Note, 36);
  CHECK(lane_matches(kick, nt(0, 36)));
  CHECK_FALSE(lane_matches(kick, nt(0, 38)));
  CHECK_FALSE(lane_matches(kick, cc(0, 36, 1)));
  kick.chan = 10;
  CHECK(lane_matches(kick, nt(0, 36, 100, 10)));
  CHECK_FALSE(lane_matches(kick, nt(0, 36, 100, 1)));
  CHECK(lane_matches(lane(EventType::Note, std::nullopt), nt(0, 99)));
  HyperLane bend = lane(EventType::PitchBend, 5);
  CHECK(lane_matches(bend, {EventType::PitchBend, 0, 1, 0, 100, 0}));
}

TEST_CASE("values and ranges") {
  Event p{EventType::Program, 0, 1, 5, 0, 0};
  CHECK(event_value(p) == 5);
  set_event_value(p, 200);
  CHECK(p.data1 == 127);
  Event n = nt(0, 36);
  set_event_value(n, 0);
  CHECK(n.data2 == 1);
  Event b{EventType::PitchBend, 0, 1, 0, 0, 0};
  set_event_value(b, -9000);
  CHECK(b.data2 == -8192);
}

TEST_CASE("paint: click inserts on nearest grid step with lane len and value") {
  HyperSet s{"", {lane(EventType::Note, 36)}};
  s.lanes[0].len = 0.05;
  Track t;
  paint(t, s, 0, {0.6, 0.6, 90, 90, 0.01}, true, true);
  REQUIRE(t.events.size() == 1);
  CHECK(t.events[0].pos == doctest::Approx(0.5));
  CHECK(t.events[0].data1 == 36);
  CHECK(t.events[0].data2 == 90);
  CHECK(t.events[0].len == doctest::Approx(0.05));
  CHECK(t.events[0].chan == 1);
  paint(t, s, 0, {0.55, 0.55, 40, 40, 0.1}, true, true);
  REQUIRE(t.events.size() == 1);
  CHECK(t.events[0].data2 == 40);
}

TEST_CASE("paint: sweep inserts at grid spacing, interpolating value; keeps events sorted") {
  HyperSet s{"", {lane(EventType::CC, 1, 0.5)}};
  Track t;
  t.events.push_back(nt(0.75, 60));
  paint(t, s, 0, {0, 2, 0, 127, 0.01}, true, true);
  CHECK(count(t, s.lanes[0]) == 5);
  CHECK(std::is_sorted(t.events.begin(), t.events.end(), [](auto& a, auto& b) { return a.pos < b.pos; }));
  CHECK(t.events[0].data2 == 0);
  CHECK(t.events.back().data2 == 127);
  CHECK(t.events.back().len == 0);
}

TEST_CASE("paint: change-only leaves gaps; insert-only leaves existing values") {
  HyperSet s{"", {lane(EventType::Note, 36)}};
  Track t;
  t.events = {nt(0, 36, 50), nt(0.5, 36, 50)};
  paint(t, s, 0, {0, 1, 100, 100, 0.01}, true, false);
  CHECK(t.events.size() == 2);
  CHECK(t.events[0].data2 == 100);
  t.events[0].data2 = 50;
  paint(t, s, 0, {0, 1, 120, 120, 0.01}, false, true);
  CHECK(t.events.size() == 5);
  CHECK(t.events[0].data2 == 50);
}

TEST_CASE("paint: ALL lane needing data1 can't insert; fixed velocity for notes") {
  HyperSet s{"", {lane(EventType::Note, std::nullopt)}};
  Track t;
  t.events = {nt(0, 40, 10)};
  paint(t, s, 0, {0, 1, 70, 70, 0.01}, true, true, 99);
  REQUIRE(t.events.size() == 1);
  CHECK(t.events[0].data2 == 99);
}

TEST_CASE("hihat group: inserting removes group-mates at the same position only") {
  HyperSet s{"", {lane(EventType::Note, 42, 0.25, true), lane(EventType::Note, 46, 0.25, true),
                  lane(EventType::Note, 36), lane(EventType::Note, 45, 0.25, true)}};
  CHECK(hhat_group(s, 0) == std::pair{0, 1});
  CHECK(hhat_group(s, 2) == std::pair{2, 2});
  CHECK(hhat_group(s, 3) == std::pair{3, 3});
  Track t;
  t.events = {nt(0, 42), nt(0.25, 42), nt(0.5, 36), nt(0.5, 45)};
  paint(t, s, 1, {0.5, 0.5, 100, 100, 0.01}, true, true);
  paint(t, s, 1, {0, 0, 100, 100, 0.01}, true, true);
  CHECK(count(t, s.lanes[0]) == 1);
  CHECK(count(t, s.lanes[1]) == 2);
  CHECK(count(t, s.lanes[2]) == 1);
  CHECK(count(t, s.lanes[3]) == 1);
}

TEST_CASE("delete hihat duplicates keeps the run lane") {
  HyperSet s{"", {lane(EventType::Note, 42, 0.25, true), lane(EventType::Note, 46, 0.25, true)}};
  Track t;
  t.events = {nt(0, 42), nt(0, 46), nt(0.5, 46), nt(1, 42)};
  CHECK(delete_hhat_duplicates(t, s, 1) == 1);
  CHECK(count(t, s.lanes[0]) == 1);
  CHECK(count(t, s.lanes[1]) == 2);
}

TEST_CASE("erase range and erase on grid only") {
  HyperLane l = lane(EventType::Note, 42, 0.5);
  Track t;
  for (int i = 0; i < 8; ++i) t.events.push_back(nt(i * 0.25, 42));
  t.events.push_back(nt(0.5, 36));
  CHECK(erase(t, l, 0, 1.75, true) == 4);
  CHECK(count(t, l) == 4);
  CHECK(erase(t, l, 0.2, 0.3) == 1);
  CHECK(t.events.size() == 4);
}

TEST_CASE("offset lane clamps") {
  HyperLane l = lane(EventType::Note, 36);
  Track t;
  t.events = {nt(0, 36, 120), nt(1, 36, 5), nt(1, 38, 50)};
  offset_lane(t, l, 20);
  CHECK(t.events[0].data2 == 127);
  CHECK(t.events[1].data2 == 25);
  CHECK(t.events[2].data2 == 50);
  offset_lane(t, l, -40);
  CHECK(t.events[1].data2 == 1);
}

TEST_CASE("copy and convert lane take destination definition") {
  HyperLane snare = lane(EventType::Note, 38), clap = lane(EventType::Note, 39);
  clap.chan = 2;
  Track t;
  t.events = {nt(0, 38, 80), nt(1, 38, 90), nt(0.5, 36)};
  convert_lane(t, snare, clap, true);
  CHECK(count(t, snare) == 2);
  REQUIRE(count(t, clap) == 2);
  convert_lane(t, snare, clap, false);
  CHECK(count(t, snare) == 0);
  CHECK(count(t, clap) == 4);
  for (auto& e : t.events)
    if (e.data1 == 39) CHECK(e.chan == 2);

  HyperLane mod = lane(EventType::CC, 1), bend = lane(EventType::PitchBend, std::nullopt);
  t.events = {cc(0, 1, 127), cc(1, 1, 0)};
  convert_lane(t, mod, bend, false);
  CHECK(t.events[0].type == EventType::PitchBend);
  CHECK(t.events[0].data2 == 8191);
  CHECK(t.events[1].data2 == -8192);
  convert_lane(t, bend, lane(EventType::Note, 60), false);
  CHECK(t.events[0].len > 0);
}

TEST_CASE("convert set maps lane i to lane i without chaining") {
  HyperSet a{"", {lane(EventType::Note, 36), lane(EventType::Note, 38)}};
  HyperSet b{"", {lane(EventType::Note, 38), lane(EventType::Note, 40)}};
  Track t;
  t.events = {nt(0, 36), nt(1, 38), nt(2, 50)};
  convert_set(t, a, b);
  CHECK(t.events[0].data1 == 38);
  CHECK(t.events[1].data1 == 40);
  CHECK(t.events[2].data1 == 50);
}

TEST_CASE("add unmatched lanes, default set, delete set fixes track refs") {
  HyperSet s = default_hyper_set();
  CHECK(s.lanes.size() >= 5);
  CHECK(hhat_group(s, 3).second > 3);
  Track t;
  t.events = {nt(0, 36), nt(0, 61), nt(1, 61), cc(0, 7, 1)};
  CHECK(add_unmatched_lanes(s, t) == 2);
  CHECK(s.lanes[s.lanes.size() - 2].name == "C#3");
  CHECK(s.lanes.back().name == "CC 7");

  Song song;
  song.hyper_sets = {s, s, s};
  song.patterns[1].tracks[1].hyper_set = 0;
  song.patterns[1].tracks[2].hyper_set = 2;
  delete_hyper_set(song, 0);
  CHECK(song.hyper_sets.size() == 2);
  CHECK(song.patterns[1].tracks[1].hyper_set == -1);
  CHECK(song.patterns[1].tracks[2].hyper_set == 1);
}
