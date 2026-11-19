#include "doctest.h"
#include "recorder.h"

using namespace rotator;

TEST_CASE("recorder: notes get lengths, positions are pattern-relative") {
  Recorder r;
  r.origin = 8;
  r.len = 4;
  std::vector<Event> ev;
  r.feed(0x90, 60, 100, 8.5, ev);
  r.feed(0xB1, 7, 90, 9, ev);
  r.feed(0x80, 60, 0, 9.5, ev);
  r.feed(0x90, 62, 80, 7.5, ev);  // count-in: dropped
  r.feed(0x90, 64, 80, 12.5, ev);  // past the end: dropped
  REQUIRE(ev.size() == 2);
  CHECK(ev[0].pos == 0.5);
  CHECK(ev[0].len == 1);
  CHECK(ev[0].data2 == 100);
  CHECK(ev[1].type == EventType::CC);
  CHECK(ev[1].chan == 2);
}

TEST_CASE("recorder: cycling wraps positions and held notes end at stop") {
  Recorder r;
  r.origin = 0;
  r.len = 4;
  r.cycle = true;
  std::vector<Event> ev;
  r.feed(0x90, 60, 100, 5, ev);
  r.feed(0x90, 61, 100, 7.5, ev);
  r.feed(0x80, 61, 0, 8.5, ev);  // crosses the cycle end
  r.finish(6, ev);
  REQUIRE(ev.size() == 2);
  CHECK(ev[0].pos == 1);
  CHECK(ev[0].len == 1);
  CHECK(ev[1].pos == 3.5);
  CHECK(ev[1].len == 1);
}

TEST_CASE("recorder: positions and lengths land on 1/192 QN ticks") {
  Recorder r;
  r.len = 16;
  std::vector<Event> ev;
  r.feed(0x90, 60, 100, 5.744000000000057, ev);
  r.feed(0x80, 60, 0, 7.184000000000138, ev);
  CHECK(ev[0].pos * 192 == doctest::Approx(1103));
  CHECK(ev[0].len * 192 == doctest::Approx(276));
}
