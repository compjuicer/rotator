#include "doctest.h"
#include "ed_matrix_ops.h"

using namespace rotator;

static Event n(double pos, int pitch, double len = 1, int chan = 1) { return {EventType::Note, pos, chan, pitch, 100, len}; }

TEST_CASE("matrix: snap") {
  CHECK(snap(1.13, 0.25) == 1.25);
  CHECK(snap(1.12, 0.25) == 1.0);
  CHECK(snap(1.13, 0) == 1.13);
}

TEST_CASE("matrix: axis lock") {
  CHECK(lock_axis(Axis::Undecided, 2, 1, 4, false) == Axis::Undecided);
  CHECK(lock_axis(Axis::Undecided, 6, 1, 4, false) == Axis::Pitch);
  CHECK(lock_axis(Axis::Undecided, 1, -6, 4, false) == Axis::Time);
  CHECK(lock_axis(Axis::Time, 50, 0, 4, false) == Axis::Time);
  CHECK(lock_axis(Axis::Time, 0, 0, 4, true) == Axis::Free);
}

TEST_CASE("matrix: hit test, end zone, short notes, non-notes ignored") {
  std::vector<Event> ev = {n(0, 60, 1), n(2, 60, 0.01), {EventType::CC, 0, 1, 60, 1, 0}};
  CHECK(hit_note(ev, 0.1, 60, 0.1, 0.1).index == 0);
  CHECK_FALSE(hit_note(ev, 0.1, 60, 0.1, 0.1).end);
  CHECK(hit_note(ev, 0.95, 60, 0.1, 0.1).end);
  CHECK(hit_note(ev, 0.5, 61, 0.1, 0.1).index == -1);
  CHECK(hit_note(ev, 1.5, 60, 0.1, 0.1).index == -1);
  auto h = hit_note(ev, 2.05, 60, 0.1, 0.1);  // past the beam, inside the minimum hit area
  CHECK(h.index == 1);
  CHECK(h.end);
  CHECK(hit_note({n(0, 60, 4), n(1, 60, 1)}, 1.2, 60, 0, 0.1).index == 1);
}

TEST_CASE("matrix: move clamps to range and refuses same-key overlap") {
  std::vector<Event> ev = {n(0, 60), n(2, 60), n(2, 60, 1, 2), {EventType::CC, 0, 1, 7, 1, 0}};
  CHECK(move_notes(ev, {0}, -3, 0));
  CHECK(ev[0].pos == 0);
  CHECK_FALSE(move_notes(ev, {0}, 1.5, 0));
  CHECK(ev[0].pos == 0);
  CHECK(move_notes(ev, {0}, 1, 0));  // touching end-to-start is fine
  CHECK(ev[0].pos == 1);
  CHECK(move_notes(ev, {0, 1}, 0.5, 0));  // moving together never collides with itself
  CHECK(ev[1].pos == 2.5);
  CHECK(move_notes(ev, {0, 3}, 0, 100));
  CHECK(ev[0].data1 == 127);
  CHECK(ev[3].data1 == 7);
  CHECK(ev[3].pos == 0);
}

TEST_CASE("matrix: resize keeps minimum and clips at next same-key note") {
  std::vector<Event> ev = {n(0, 60, 1), n(2, 60, 1), n(1, 60, 1, 2)};
  resize_notes(ev, {0}, 5, 0.1);
  CHECK(ev[0].len == 2);
  resize_notes(ev, {0}, -5, 0.1);
  CHECK(ev[0].len == doctest::Approx(0.1));
  resize_notes(ev, {1}, 3, 0.1);
  CHECK(ev[1].len == 4);
}

TEST_CASE("matrix: insert refuses inside a note and clips at the next") {
  std::vector<Event> ev = {n(1, 60, 1)};
  CHECK(insert_note(ev, n(1.5, 60)) == -1);
  CHECK(insert_note(ev, n(1, 60)) == -1);
  CHECK(insert_note(ev, n(1.5, 60, 1, 2)) == 1);
  CHECK(insert_note(ev, n(0.5, 60, 2)) == 2);
  CHECK(ev[2].len == 0.5);
  CHECK(insert_note(ev, n(2, 60)) == 3);
}

TEST_CASE("matrix: velocity, delete, rubber band") {
  std::vector<Event> ev = {n(0, 60), n(1, 62), n(2, 64), {EventType::CC, 1, 1, 62, 5, 0}};
  add_velocity(ev, {0, 3}, 50);
  CHECK(ev[0].data2 == 127);
  CHECK(ev[3].data2 == 5);
  add_velocity(ev, {1}, -200);
  CHECK(ev[1].data2 == 1);
  CHECK(notes_in(ev, 1.5, 0.5, 63, 60) == std::vector<int>{0, 1});
  delete_events(ev, {0, 2, 0});
  REQUIRE(ev.size() == 2);
  CHECK(ev[0].data1 == 62);
  CHECK(ev[1].type == EventType::CC);
}
