#include "doctest.h"
#include "ed_event_ops.h"

using namespace rotator;

static Event ev(EventType t, double pos, int d1 = 60, int d2 = 100, double len = 0) { return {t, pos, 1, d1, d2, len}; }
static Event nt(double pos, int pitch, double len = 0.25) { return ev(EventType::Note, pos, pitch, 100, len); }

TEST_CASE("event: bar beat unit pulse in 4/4 and 6/8") {
  Grid g;
  CHECK(format_pos(0, g) == "1 1 1 1");
  CHECK(format_pos(4 + 1 + 0.25 + from_pulses(5), g) == "2 2 2 6");
  CHECK(format_pos(0, g, 4) == "5 1 1 1");
  Bbup b = to_bbup(7.75, g);
  CHECK(from_bbup(b, g) == doctest::Approx(7.75));
  CHECK(from_bbup({1, 5, 1, 1}, g) == doctest::Approx(4));  // carries

  Grid g68{6, 8, 16};
  CHECK(g68.beat_p() == 96);
  CHECK(g68.units_per_beat() == 2);
  CHECK(format_pos(3, g68) == "2 1 1 1");
  CHECK(format_pos(3.75, g68) == "2 2 2 1");

  Grid coarse{4, 4, 4};
  CHECK(coarse.unit_p() == kPPQ);
  Grid triplet{6, 8, 12};  // 1/12 unit (64) into a 96-pulse beat: last unit absorbs the rest
  CHECK(triplet.units_per_beat() == 1);
  CHECK(format_pos(from_pulses(80), triplet) == "1 1 1 81");
}

TEST_CASE("event: lengths and parsing") {
  Grid g;
  CHECK(format_len(2, g) == "2 0 0");
  CHECK(format_len(0.25, g) == "0 1 0");
  CHECK(format_len(5, g) == "1 1 0 0");
  CHECK(parse_len("48", g) == doctest::Approx(0.25));
  CHECK(parse_len("2 0 0", g) == doctest::Approx(2));
  CHECK(parse_len("1 0 0 0", g) == doctest::Approx(4));
  CHECK(!parse_len("0", g));
  CHECK(!parse_len("x", g));
  CHECK(parse_pos("2", g) == doctest::Approx(4));
  CHECK(parse_pos("1 3 2 1", g) == doctest::Approx(2.25));
  CHECK(!parse_pos("0", g));
  CHECK(!parse_pos("1 2 3 4 5", g));
}

TEST_CASE("event: note names, C3 = 60") {
  CHECK(note_name(60) == "C3");
  CHECK(note_name(64 + 12) == "E4");
  CHECK(note_name(0) == "C-2");
  CHECK(note_name(127) == "G8");
  CHECK(parse_note("E4") == 76);
  CHECK(parse_note("f#2") == 54);
  CHECK(parse_note("Bb-1") == 22);
  CHECK(parse_note("76") == 76);
  CHECK(!parse_note("H3"));
  CHECK(!parse_note("C9"));
}

TEST_CASE("event: fields per status; pitch wheel as LSB/MSB") {
  Event pb = ev(EventType::PitchBend, 0, 0, 0);
  CHECK(get_field(pb, Col::D1) == 0);
  CHECK(get_field(pb, Col::D2) == 64);
  set_field(pb, Col::D2, 127);
  CHECK(pb.data2 == 8192 - 128);
  set_field(pb, Col::D1, 127);
  CHECK(pb.data2 == 8191);

  Event cp = ev(EventType::ChanPressure, 0, 0, 33);
  CHECK(get_field(cp, Col::D1) == 33);
  CHECK(!has_field(EventType::ChanPressure, Col::D2));
  CHECK(!has_field(EventType::Program, Col::D2));
  CHECK(!has_field(EventType::CC, Col::Len));

  Event n = nt(0, 60);
  set_field(n, Col::D2, 0);
  CHECK(n.data2 == 1);
  set_field(n, Col::Chan, 20);
  CHECK(n.chan == 16);
}

TEST_CASE("event: status change only between non-note types") {
  Event e = ev(EventType::CC, 0, 7, 100);
  set_status(e, EventType::PitchBend);
  CHECK(e.type == EventType::PitchBend);
  CHECK(e.data2 == 0);
  set_status(e, EventType::Program);
  CHECK(e.type == EventType::Program);
  CHECK(e.data2 == 64);
  set_status(e, EventType::Note);
  CHECK(e.type == EventType::Program);
  Event n = nt(0, 60);
  set_status(n, EventType::CC);
  CHECK(n.type == EventType::Note);
}

TEST_CASE("event: send order, lowest pitch first in a chord") {
  std::vector<Event> v = {nt(1, 67), ev(EventType::CC, 0), nt(1, 60), nt(0.5, 72), ev(EventType::Program, 1)};
  int f = sort_events(v, 2);
  CHECK(v[f].data1 == 60);
  CHECK(v[0].type == EventType::CC);
  CHECK(v[1].pos == 0.5);
  CHECK(v[2].data1 == 60);
  CHECK(v[3].data1 == 67);
  CHECK(v[4].type == EventType::Program);
}

TEST_CASE("event: move and insert mode") {
  std::vector<Event> v = {nt(0, 60), nt(1, 62), nt(2, 64)};
  move_event(v, 1, 1.5, false);
  CHECK(v[1].pos == 1.5);
  CHECK(v[2].pos == 2);
  move_event(v, 0, 1, true);
  CHECK(v[0].pos == 1);
  CHECK(v[1].pos == 2.5);
  CHECK(v[2].pos == 3);
  move_event(v, 0, -3, true);
  CHECK(v[0].pos == 0);
  CHECK(v[1].pos == 1.5);
  CHECK(step_pos(0, Col::Beat, 5, Grid{}) == 5);
  CHECK(step_pos(0, Col::Bar, -1, Grid{}) == 0);
}

TEST_CASE("event: note length clamps at next note of same pitch and channel") {
  std::vector<Event> v = {nt(0, 60), nt(0.5, 62), nt(1, 60)};
  v[2].chan = 2;
  v.push_back(nt(2, 60));
  set_length(v, 0, 5);
  CHECK(v[0].len == 2);
  set_length(v, 0, 0);
  CHECK(to_pulses(v[0].len) == 1);
  set_lengths(v, 0, 3, 0.5);
  CHECK(v[0].len == 0.5);
  CHECK(v[1].len == 0.5);
  CHECK(v[3].len == 0.5);
}

TEST_CASE("event: insert keeps order; new events get defaults") {
  std::vector<Event> v = {nt(0, 60), nt(1, 60)};
  int i = insert_event(v, new_event(EventType::CC, 1, 3));
  CHECK(i == 2);
  CHECK(v[2].chan == 3);
  i = insert_event(v, new_event(EventType::Note, 0.5, 1));
  CHECK(i == 1);
  CHECK(v[1].len == 0.25);
}

TEST_CASE("event: fast transform T / Shift-T") {
  std::vector<Event> v = {nt(0, 60, 1), ev(EventType::CC, 0.25), nt(0, 64, 0.5), nt(1, 67, 0.5), nt(2, 60, 0.5)};
  v[0].data2 = 80;
  transform(v, 0, Col::Len, false);
  CHECK(v[2].len == 1);
  CHECK(v[3].len == 0.5);
  transform(v, 0, Col::D2, true);
  CHECK(v[2].data2 == 80);
  CHECK(v[3].data2 == 80);
  CHECK(v[4].data2 == 80);
  CHECK(v[1].data2 == 100);
}

TEST_CASE("event: fast delete / keep") {
  std::vector<Event> v = {nt(0, 36), nt(0, 38), ev(EventType::CC, 0, 7), nt(1, 36), ev(EventType::CC, 1, 10),
                          ev(EventType::Program, 2, 5)};
  auto w = v;
  int c = delete_matching(w, 3, false);
  CHECK(w.size() == 4);
  CHECK(w[c].type == EventType::CC);
  w = v;
  c = delete_matching(w, 3, true);
  CHECK(w.size() == 5);
  CHECK(w[c].pos == 1);
  CHECK(w[c].data1 == 36);
  w = v;
  c = delete_matching(w, 2, true);
  CHECK(w.size() == 5);
  CHECK(w[c].data1 == 7);
  w = v;
  CHECK(delete_matching(w, 5, true) == 5);
  CHECK(w.size() == 6);
  c = delete_matching(w, 5, false);
  CHECK(w.size() == 5);
  CHECK(c == 4);
}

TEST_CASE("event: MIDI IN value entry") {
  Event n = nt(0, 60);
  CHECK(midi_in(n, Col::D2, 0x90, 72, 33));
  CHECK(n.data1 == 60);
  CHECK(n.data2 == 33);
  CHECK(midi_in(n, Col::Status, 0x91, 72, 90));
  CHECK(n.data1 == 72);
  CHECK(n.data2 == 90);
  CHECK(!midi_in(n, Col::D1, 0x90, 50, 0));
  CHECK(!midi_in(n, Col::D1, 0xB0, 1, 2));
  Event cc = ev(EventType::CC, 0, 1, 0);
  CHECK(midi_in(cc, Col::D1, 0xB0, 7, 99));
  CHECK(cc.data1 == 7);
  CHECK(cc.data2 == 0);
}

TEST_CASE("event: step input advances after a chord is released") {
  std::vector<Event> v;
  StepInput s;
  step_note_on(s, v, 1, 60, 100, 0.2);
  step_note_on(s, v, 1, 64, 100, 0.2);
  step_note_off(s, 0.25);
  CHECK(s.pos == 0);
  step_note_off(s, 0.25);
  CHECK(s.pos == 0.25);
  step_note_off(s, 0.25);
  CHECK(s.pos == 0.25);
  step_note_on(s, v, 1, 60, 100, 0.2);
  step_note_off(s, 0.25);
  REQUIRE(v.size() == 3);
  CHECK(v[2].pos == 0.25);
  CHECK(v[0].len == doctest::Approx(0.2).epsilon(0.01));
}

TEST_CASE("event: placements and catch lookup") {
  Song song;
  song.patterns[1].tracks[1].events = {nt(0, 60, 8)};
  song.patterns[2].tracks[1].events = {nt(0, 60, 4)};
  auto add = [&](char c, int bar, int p) {
    Entry e;
    e.bar = bar;
    e.pattern = p;
    song.chains[c].push_back(e);
  };
  add('a', 5, 1);
  add('a', 1, 1);
  add('a', 2, 2);
  add('b', 3, 1);
  auto bar4 = [](int bar) { return (bar - 1) * 4.0; };
  auto p = placements(song, 1, bar4);
  REQUIRE(p.size() == 3);
  CHECK(p[0].bar == 1);
  CHECK(p[0].end == 4);
  CHECK(p[1].bar == 3);
  CHECK(p[1].end == 16);
  CHECK(p[2].start == 16);
  CHECK(p[2].end == 24);
  CHECK(placement_at(p, 9)->bar == 3);
  CHECK(!placement_at(p, 30));

  std::vector<Event> v = {nt(0, 60), nt(1, 60), ev(EventType::CC, 1.5), nt(2, 60)};
  auto all = [](int) { return true; };
  CHECK(event_at(v, 1.7, all) == 2);
  CHECK(event_at(v, 1.7, [](int i) { return i != 2; }) == 1);
  CHECK(event_at(v, 9, all) == 3);
}
