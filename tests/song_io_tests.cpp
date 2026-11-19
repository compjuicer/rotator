#include "doctest.h"
#include "song_io.h"

using namespace rotator;

static void check_equal(const Song& a, const Song& b) {
  REQUIRE(a.patterns.size() == b.patterns.size());
  for (const auto& [pn, pa] : a.patterns) {
    const Pattern& pb = b.patterns.at(pn);
    CHECK(pa.name == pb.name);
    REQUIRE(pa.tracks.size() == pb.tracks.size());
    for (const auto& [tn, ta] : pa.tracks) {
      const Track& tb = pb.tracks.at(tn);
      CHECK(ta.name == tb.name);
      CHECK(ta.dest == tb.dest);
      CHECK(ta.chan == tb.chan);
      CHECK(ta.transpose == tb.transpose);
      CHECK(ta.velocity == tb.velocity);
      CHECK(ta.loop == tb.loop);
      CHECK(ta.delay == tb.delay);
      CHECK(ta.lowest == tb.lowest);
      CHECK(ta.highest == tb.highest);
      CHECK(ta.mute == tb.mute);
      CHECK(ta.hide == tb.hide);
      REQUIRE(ta.events.size() == tb.events.size());
      for (size_t i = 0; i < ta.events.size(); ++i) {
        CHECK(ta.events[i].type == tb.events[i].type);
        CHECK(ta.events[i].pos == tb.events[i].pos);
        CHECK(ta.events[i].len == tb.events[i].len);
        CHECK(ta.events[i].data1 == tb.events[i].data1);
        CHECK(ta.events[i].data2 == tb.events[i].data2);
        CHECK(ta.events[i].chan == tb.events[i].chan);
      }
    }
  }
  REQUIRE(a.chains.size() == b.chains.size());
  for (const auto& [c, ea] : a.chains) {
    const auto& eb = b.chains.at(c);
    REQUIRE(ea.size() == eb.size());
    for (size_t i = 0; i < ea.size(); ++i) {
      CHECK(ea[i].bar == eb[i].bar);
      CHECK(ea[i].pattern == eb[i].pattern);
      CHECK(ea[i].name == eb[i].name);
      CHECK(ea[i].transpose == eb[i].transpose);
      CHECK(ea[i].mutes == eb[i].mutes);
    }
  }
}

TEST_CASE("song round-trips through lines") {
  Song s;
  Pattern& p1 = s.patterns[1];
  p1.name = "Verse \"A\" it's `odd`";
  Track& t1 = p1.tracks[1];
  t1.name = "Kick drum";
  t1.dest = "{11111111-2222-3333-4444-555555555555}";
  t1.chan = 10;
  t1.transpose = -12;
  t1.velocity = 7;
  t1.loop = 1.0 / 3;
  t1.delay = -0.1;
  t1.lowest = 20;
  t1.highest = 100;
  t1.mute = true;
  t1.hide = true;
  t1.quantize = 16;
  t1.groove = 7;
  t1.compress = -2;
  t1.ghost_pattern = 7;
  t1.ghost_track = 2;
  t1.filter = 5;
  t1.chan_filter = true;
  t1.events = {{EventType::Note, 0, 10, 36, 127, 0.1},
               {EventType::Note, 1.0 / 3, 16, 38, 1, 2.0 / 7},
               {EventType::CC, 0.5, 2, 74, 127},
               {EventType::Program, 0.75, 3, 42, 0},
               {EventType::PitchBend, 1, 4, 0, -8192},
               {EventType::PitchBend, 1.25, 4, 0, 8191},
               {EventType::ChanPressure, 1.5, 5, 0, 99},
               {EventType::PolyPressure, 2.0 / 3, 6, 61, 33}};
  Track& t3 = p1.tracks[3];
  t3.name = "it's";
  t3.events = {{EventType::Note, 0.5, 1, 60, 100, 0.25}};
  s.patterns[7].name = "";
  s.patterns[7].tracks[2].name = "say \"hi\"";

  Entry e1;
  e1.bar = 1;
  e1.pattern = 1;
  e1.name = "Intro part";
  e1.transpose = 3;
  e1.mutes = {{1, true}, {3, true}};
  e1.shift = -48;
  Entry e2;
  e2.bar = 9;
  e2.pattern = 7;
  s.chains['a'] = {e1, e2};
  s.chains['c'] = {e2};
  s.solo_chain = 'b';
  s.quant_mode = QuantMode::NoteOn;
  s.quant_range = 24;
  s.quant_strength = 70;
  s.grooves[3] = {2, 5, 12, 80};
  s.mute_groups[9] = {{2, true}, {16, true}};
  s.solo_track = 3;
  s.no_arrange_transpose = {{"{11111111-2222-3333-4444-555555555555}", 10}, {"x", 1}};

  std::vector<std::string> lines = to_lines(s);
  Song r = from_lines(lines);
  CHECK(r.patterns[1].name == "Verse \"A\" it's 'odd'");
  r.patterns[1].name = s.patterns[1].name;
  check_equal(s, r);
  CHECK(r.no_arrange_transpose == s.no_arrange_transpose);
  CHECK(r.solo_chain == 'b');
  CHECK(r.quant_mode == QuantMode::NoteOn);
  CHECK(r.quant_range == 24);
  CHECK(r.quant_strength == 70);
  CHECK(r.grooves[3].track == 5);
  CHECK(r.grooves[3].strength == 80);
  CHECK(r.mute_groups[9] == s.mute_groups[9]);
  CHECK(r.patterns[1].tracks[1].filter == 5);
  CHECK(r.patterns[1].tracks[1].chan_filter);
  CHECK(r.solo_track == 3);
  CHECK(r.patterns[1].tracks[1].quantize == 16);
  CHECK(r.patterns[1].tracks[1].groove == 7);
  CHECK(r.patterns[1].tracks[1].compress == -2);
  CHECK(r.patterns[1].tracks[1].ghost_pattern == 7);
  CHECK(r.patterns[1].tracks[1].ghost_track == 2);
  CHECK(r.chains['a'][0].shift == -48);
  CHECK(to_lines(from_lines(lines)) == lines);
}

TEST_CASE("unknown records ignored") {
  Song r = from_lines({"FUTURE 1 2 \"x\"", "PATTERN 2 \"P\"", "NEWTHING", "TRACK 1 \"T\" \"\" 0 0 0 0 0 0 127 0 0",
                       "E 0 1 60 100 1", "", "ENTRY a 1 2 0 \"\" \"\""});
  CHECK(r.patterns.size() == 1);
  CHECK(r.patterns[2].name == "P");
  CHECK(r.patterns[2].tracks[1].events.size() == 1);
  CHECK(!r.patterns[2].tracks[1].chan);
  CHECK(!r.patterns[2].tracks[1].loop);
  CHECK(r.chains['a'].size() == 1);
  CHECK(r.chains['a'][0].mutes.empty());
}

TEST_CASE("legacy E note lines load as notes") {
  Song r = from_lines({"PATTERN 1 \"P\"", "TRACK 1 \"T\" \"\" 0 0 0 0 0 0 127 0 0", "E 0.5 0.25 64 90 3"});
  const auto& ev = r.patterns[1].tracks[1].events;
  REQUIRE(ev.size() == 1);
  CHECK(ev[0].type == EventType::Note);
  CHECK(ev[0].pos == 0.5);
  CHECK(ev[0].len == 0.25);
  CHECK(ev[0].data1 == 64);
  CHECK(ev[0].data2 == 90);
  CHECK(ev[0].chan == 3);
  CHECK(to_lines(r)[2] == "N 0.5 0.25 64 90 3");
}

TEST_CASE("event record formats") {
  Song s;
  s.patterns[1].tracks[1].events = {{EventType::CC, 1, 2, 7, 100},   {EventType::Program, 1, 2, 5, 0},
                                    {EventType::PitchBend, 1, 2, 0, -1}, {EventType::ChanPressure, 1, 2, 0, 9},
                                    {EventType::PolyPressure, 1, 2, 60, 8}};
  auto l = to_lines(s);
  REQUIRE(l.size() == 7);
  CHECK(l[2] == "C 1 2 7 100");
  CHECK(l[3] == "P 1 2 5");
  CHECK(l[4] == "B 1 2 -1");
  CHECK(l[5] == "A 1 2 9");
  CHECK(l[6] == "K 1 2 60 8");
}

TEST_CASE("empty input gives empty song") {
  Song r = from_lines({});
  CHECK(r.patterns.empty());
  CHECK(r.chains.empty());
  CHECK(to_lines(r).empty());
}

TEST_CASE("hyper sets and track hyper set round-trip") {
  Song s;
  s.patterns[1].tracks[1].hyper_set = 1;
  s.patterns[1].tracks[2];
  HyperLane a;
  a.name = "Hihat \"CL\"";
  a.data1 = 42;
  a.chan = 10;
  a.grid = 1.0 / 3;
  a.len = 0.0625;
  a.delay = -0.01;
  a.hhat = true;
  HyperLane b;
  b.name = "Bend";
  b.type = EventType::PitchBend;
  s.hyper_sets = {{"Kit A", {a, b}}, {"Empty", {}}};
  Song r = from_lines(to_lines(s));
  CHECK(r.patterns[1].tracks[1].hyper_set == 1);
  CHECK(r.patterns[1].tracks[2].hyper_set == -1);
  REQUIRE(r.hyper_sets.size() == 2);
  CHECK(r.hyper_sets[0].name == "Kit A");
  CHECK(r.hyper_sets[1].name == "Empty");
  CHECK(r.hyper_sets[1].lanes.empty());
  REQUIRE(r.hyper_sets[0].lanes.size() == 2);
  const HyperLane &ra = r.hyper_sets[0].lanes[0], &rb = r.hyper_sets[0].lanes[1];
  CHECK(ra.name == a.name);
  CHECK(ra.type == EventType::Note);
  CHECK(ra.data1 == 42);
  CHECK(ra.chan == 10);
  CHECK(ra.grid == a.grid);
  CHECK(ra.len == a.len);
  CHECK(ra.delay == a.delay);
  CHECK(ra.hhat);
  CHECK(rb.type == EventType::PitchBend);
  CHECK(!rb.data1);
  CHECK(!rb.chan);
  CHECK(!rb.hhat);
  CHECK(to_lines(r) == to_lines(s));
}

TEST_CASE("bad hyper lane type and orphan lanes are ignored") {
  Song r = from_lines({"HLANE \"x\" N -1 0 0.25 0.1 0 0", "HSET \"S\"", "HLANE \"y\" Z -1 0 0.25 0.1 0 0"});
  REQUIRE(r.hyper_sets.size() == 1);
  CHECK(r.hyper_sets[0].lanes.empty());
}
