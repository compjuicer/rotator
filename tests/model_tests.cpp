#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "model.h"

#include <algorithm>

using namespace rotator;

static double bar4(int bar) { return (bar - 1) * 4.0; }

static Event note(double pos, int pitch = 60) { return {EventType::Note, pos, 1, pitch, 100, 0.5}; }

static Track track(std::string dest, std::vector<Event> events) {
  Track t;
  t.dest = std::move(dest);
  t.events = std::move(events);
  return t;
}

static Entry entry(int bar, int pattern) {
  Entry e;
  e.bar = bar;
  e.pattern = pattern;
  return e;
}

TEST_CASE("entry length from next entry; loop fills; truncation at entry end") {
  Song song;
  Track kick = track("A", {note(0, 36), note(1.5, 38)});
  kick.loop = 1;
  song.patterns[1].tracks[1] = kick;
  song.patterns[1].tracks[2] = track("A", {note(0, 60), note(7.75, 62)});
  song.chains['a'] = {entry(3, 1), entry(1, 1)};

  auto items = render(song, bar4);
  REQUIRE(items.size() == 2);
  const auto& first = items[0];
  CHECK(first.start == 0);
  CHECK(first.finish == 8);
  int kicks = 0;
  const Event* last = nullptr;
  for (const auto& n : first.events) {
    if (n.data1 == 36) ++kicks;
    CHECK_MESSAGE(n.data1 != 38, "event past loop length must not play");
    if (n.data1 == 62) last = &n;
  }
  CHECK(kicks == 8);
  REQUIRE(last);
  CHECK(last->len == 0.25);
  CHECK(items[1].finish - items[1].start == 8.25);
}

TEST_CASE("params: transpose, velocity clamp, range, channel, delay, mutes, no dest") {
  Song song;
  Track t1 = track("A", {note(0, 60), note(1, 70), note(2, 50)});
  t1.chan = 10;
  t1.transpose = 12;
  t1.velocity = 50;
  t1.highest = 80;
  t1.delay = -0.5;
  auto& tracks = song.patterns[1].tracks;
  tracks[1] = t1;
  tracks[2] = track("B", {note(0)});
  tracks[3] = track("", {note(0)});
  Entry e = entry(1, 1);
  e.transpose = 2;
  e.mutes[2] = true;
  song.chains['b'] = {e};

  auto items = render(song, bar4);
  REQUIRE(items.size() == 1);
  CHECK(items[0].dest == "A");
  REQUIRE(items[0].events.size() == 1);
  const auto& n = items[0].events[0];
  CHECK(n.data1 == 64);
  CHECK(n.data2 == 127);
  CHECK(n.chan == 10);
  CHECK(n.pos == 1.5);
}

TEST_CASE("same start bar in one chain -> zero length, skipped") {
  Song song;
  song.patterns[1].tracks[1] = track("A", {note(0)});
  song.chains['a'] = {entry(1, 1), entry(1, 1)};
  CHECK(render(song, bar4).size() == 1);
}

TEST_CASE("looped note crossing the loop boundary is truncated at entry end") {
  Song song;
  Track t = track("A", {{EventType::Note, 0.75, 1, 60, 100, 0.5}});
  t.loop = 1;
  song.patterns[1].tracks[1] = t;
  song.chains['a'] = {entry(1, 1)};
  auto items = render(song, bar4);
  REQUIRE(items.size() == 1);
  REQUIRE(items[0].events.size() == 1);
  CHECK(items[0].events[0].len == 0.25);
}

TEST_CASE("per-type params: transpose/range on notes and poly pressure only, velocity on notes only") {
  Song song;
  Track t = track("A", {{EventType::Note, 0, 3, 60, 100, 1},
                        {EventType::CC, 0, 3, 60, 100},
                        {EventType::Program, 0, 3, 60, 0},
                        {EventType::PitchBend, 0, 3, 0, -8192},
                        {EventType::ChanPressure, 0, 3, 0, 100},
                        {EventType::PolyPressure, 0, 3, 60, 100},
                        {EventType::Note, 1, 3, 100, 100, 1},
                        {EventType::PolyPressure, 1, 3, 100, 50},
                        {EventType::CC, 1, 3, 100, 50, 2}});
  t.transpose = 5;
  t.velocity = -20;
  t.highest = 90;
  t.chan = 7;
  song.patterns[1].tracks[1] = t;
  song.chains['a'] = {entry(1, 1)};
  auto items = render(song, bar4);
  REQUIRE(items.size() == 1);
  const auto& ev = items[0].events;
  REQUIRE(ev.size() == 7);
  for (const auto& e : ev) CHECK(e.chan == 7);
  CHECK((ev[0].type == EventType::Note && ev[0].data1 == 65 && ev[0].data2 == 80 && ev[0].len == 1));
  CHECK((ev[1].type == EventType::CC && ev[1].data1 == 60 && ev[1].data2 == 100 && ev[1].len == 0));
  CHECK((ev[2].type == EventType::Program && ev[2].data1 == 60));
  CHECK((ev[3].type == EventType::PitchBend && ev[3].data2 == -8192));
  CHECK((ev[4].type == EventType::ChanPressure && ev[4].data2 == 100));
  CHECK((ev[5].type == EventType::PolyPressure && ev[5].data1 == 65 && ev[5].data2 == 100));
  CHECK((ev[6].type == EventType::CC && ev[6].data1 == 100 && ev[6].len == 0));
}

TEST_CASE("loop, delay and mutes apply to CCs") {
  Song song;
  Track t = track("A", {{EventType::CC, 0, 1, 1, 64}, {EventType::PitchBend, 1.5, 1, 0, 100}});
  t.loop = 1;
  t.delay = 0.25;
  song.patterns[1].tracks[1] = t;
  song.patterns[1].tracks[2] = track("B", {{EventType::CC, 0, 1, 7, 100}, note(3)});
  Entry e = entry(1, 1);
  e.mutes[2] = true;
  song.chains['a'] = {e, entry(2, 1)};
  auto items = render(song, bar4);
  REQUIRE(items.size() == 3);
  CHECK(items[0].dest == "A");
  REQUIRE(items[0].events.size() == 4);
  for (int i = 0; i < 4; ++i) {
    CHECK(items[0].events[i].type == EventType::CC);
    CHECK(items[0].events[i].pos == i + 0.25);
  }
  CHECK(items[2].dest == "B");
  CHECK(items[2].events.size() == 2);
}

TEST_CASE("items ordered by chain, then start, then dest") {
  Song song;
  song.patterns[1].tracks[1] = track("B", {note(0)});
  song.patterns[1].tracks[2] = track("A", {note(0)});
  song.chains['b'] = {entry(1, 1)};
  song.chains['a'] = {entry(3, 1), entry(1, 1)};
  auto items = render(song, bar4);
  REQUIRE(items.size() == 6);
  CHECK(items[0].chain == 'a');
  CHECK(items[0].start == 0);
  CHECK(items[0].dest == "A");
  CHECK(items[1].dest == "B");
  CHECK(items[2].start == 8);
  CHECK(items[4].chain == 'b');
}

TEST_CASE("free pattern and track numbers") {
  Song song;
  CHECK(free_pattern(song) == 1);
  song.patterns[1];
  song.patterns[3];
  CHECK(free_pattern(song) == 2);
  Pattern& p = song.patterns[1];
  CHECK(free_track(p) == 1);
  p.tracks[1];
  p.tracks[2];
  CHECK(free_track(p) == 3);
}

static std::vector<int> bars(const Song& s, char c) {
  std::vector<int> b;
  for (const auto& e : s.chains.at(c)) b.push_back(e.bar);
  return b;
}

TEST_CASE("entry length and add without duplicates") {
  Song s;
  s.chains['a'] = {entry(9, 1), entry(1, 2), entry(5, 3)};
  CHECK(entry_length(s, 'a', 1) == 4);
  CHECK(entry_length(s, 'a', 2) == 4);
  CHECK(!entry_length(s, 'a', 0));
  CHECK(add_entry(s, 'a', entry(5, 7)) == -1);
  CHECK(add_entry(s, 'a', entry(7, 7)) == 3);
  CHECK(entry_length(s, 'a', 2) == 2);
}

TEST_CASE("move start keeps end, clamped by neighbours") {
  Song s;
  s.chains['a'] = {entry(1, 1), entry(5, 1), entry(9, 1)};
  move_start(s, 'a', 1, 3);
  CHECK(bars(s, 'a') == std::vector<int>{1, 3, 9});
  move_start(s, 'a', 1, -5);
  CHECK(bars(s, 'a') == std::vector<int>{1, 2, 9});
  move_start(s, 'a', 1, 20);
  CHECK(bars(s, 'a') == std::vector<int>{1, 8, 9});
  move_start(s, 'a', 2, 50);
  CHECK(bars(s, 'a') == std::vector<int>{1, 8, 50});
  move_start(s, 'a', 0, -20);
  CHECK(bars(s, 'a') == std::vector<int>{-9, 8, 50});
}

TEST_CASE("move to chain keeps bar") {
  Song s;
  s.chains['a'] = {entry(1, 1), entry(5, 2)};
  s.chains['c'] = {entry(3, 3)};
  CHECK(move_to_chain(s, 'a', 0, 'c') == 1);
  CHECK(bars(s, 'a') == std::vector<int>{5});
  CHECK(bars(s, 'c') == std::vector<int>{3, 1});
  CHECK(s.chains['c'][1].pattern == 1);
  CHECK(move_to_chain(s, 'c', 0, 'c') == 0);
}

TEST_CASE("set length shifts later entries; not on last") {
  Song s;
  s.chains['a'] = {entry(9, 3), entry(1, 1), entry(5, 2)};
  s.chains['b'] = {entry(9, 1)};
  CHECK(set_length(s, 'a', 1, 6));
  CHECK(bars(s, 'a') == std::vector<int>{11, 1, 7});
  CHECK(set_length(s, 'a', 2, 1));
  CHECK(bars(s, 'a') == std::vector<int>{8, 1, 7});
  CHECK(!set_length(s, 'a', 0, 4));
  CHECK(!set_length(s, 'a', 1, 0));
  CHECK(bars(s, 'b') == std::vector<int>{9});
}

TEST_CASE("delete closes the gap") {
  Song s;
  s.chains['a'] = {entry(1, 1), entry(5, 2), entry(9, 3), entry(12, 4)};
  delete_close_gap(s, 'a', 1);
  CHECK(bars(s, 'a') == std::vector<int>{1, 5, 8});
  delete_close_gap(s, 'a', 2);
  CHECK(bars(s, 'a') == std::vector<int>{1, 5});
}

TEST_CASE("hyper lane delay offsets matching events of tracks using the set") {
  Song song;
  HyperLane snare;
  snare.data1 = 38;
  snare.delay = 0.125;
  HyperLane any;
  any.delay = 0.5;
  song.hyper_sets = {{"S", {snare, any}}};
  Track t = track("A", {note(1, 38), note(2, 36), {EventType::CC, 1, 1, 7, 9, 0}});
  t.hyper_set = 0;
  t.delay = 0.25;
  song.patterns[1].tracks[1] = t;
  t.hyper_set = -1;
  song.patterns[1].tracks[2] = t;
  song.chains['a'] = {entry(1, 1), entry(3, 1)};
  auto items = render(song, bar4);
  REQUIRE(items.size() == 2);
  std::vector<double> pos;
  for (const auto& e : items[0].events) pos.push_back(e.pos);
  std::sort(pos.begin(), pos.end());
  CHECK(pos == std::vector<double>{1.25, 1.25, 1.25, 1.375, 2.25, 2.75});
}

TEST_CASE("arrange transpose skipped for excluded dest+channel") {
  Song s;
  Track drums = track("D", {note(0, 36)});
  drums.chan = 10;
  s.patterns[1].tracks[1] = drums;
  s.patterns[1].tracks[2] = track("D", {note(0, 60)});
  Entry e = entry(1, 1);
  e.transpose = 5;
  s.chains['a'] = {e};
  s.no_arrange_transpose = {{"D", 10}};
  auto items = render(s, bar4);
  REQUIRE(items.size() == 1);
  std::vector<int> pitches;
  for (auto& ev : items[0].events) pitches.push_back(ev.data1);
  std::sort(pitches.begin(), pitches.end());
  CHECK(pitches == std::vector<int>{36, 65});
}

TEST_CASE("track copy, exchange, merge") {
  Pattern p;
  p.tracks[1] = track("A", {note(1)});
  p.tracks[1].name = "one";
  p.tracks[2] = track("B", {note(0), note(2)});
  CHECK(copy_track(p, 1) == 3);
  CHECK(p.tracks[3].name == "one");
  CHECK(p.tracks[3].dest == "A");
  exchange_tracks(p, 1, 2);
  CHECK(p.tracks[1].dest == "B");
  CHECK(p.tracks[2].dest == "A");
  merge_tracks(p, 2, 1);
  CHECK(p.tracks[1].events.size() == 3);
  CHECK(p.tracks[1].events[1].pos == 1);
  CHECK(p.tracks[2].events.size() == 1);
}

TEST_CASE("playback quantize snaps note starts; do_quantize bakes it") {
  Song s;
  Track t = track("D", {note(0.1), note(0.9)});
  t.events.push_back({EventType::CC, 0.3, 1, 7, 100});
  t.quantize = 4;
  s.patterns[1].tracks[1] = t;
  s.chains['a'] = {entry(1, 1), entry(3, 1)};
  auto items = render(s, bar4);
  std::vector<double> pos;
  for (auto& e : items[0].events) pos.push_back(e.pos);
  std::sort(pos.begin(), pos.end());
  CHECK(pos == std::vector<double>{0, 0.3, 1});
  do_quantize(s, t);
  CHECK(t.quantize == 0);
  CHECK(t.events[0].pos == 0);
  CHECK(t.events.back().pos == 1);
}

TEST_CASE("upbeat starts the entry early and cuts the previous one; cut starts late") {
  Song s;
  s.patterns[1].tracks[1] = track("D", {note(0), note(3.5)});
  s.patterns[2].tracks[1] = track("D", {note(-0.5), note(0), note(1)});
  s.patterns[2].tracks[1].delay = 0;
  Entry a = entry(1, 1), b = entry(2, 2);
  b.shift = -96;  // half a beat upbeat
  s.chains['a'] = {a, b};
  auto items = render(s, bar4);
  REQUIRE(items.size() == 2);
  CHECK(items[0].finish == doctest::Approx(3.5));
  CHECK(items[0].events.size() == 1);  // note at 3.5 falls in the upbeat
  CHECK(items[1].start == doctest::Approx(3.5));
  CHECK(items[1].events.front().pos == doctest::Approx(3.5));
  s.chains['a'][1].shift = 192;  // cut one beat
  items = render(s, bar4);
  CHECK(items[0].finish == doctest::Approx(5));
  CHECK(items[1].start == doctest::Approx(5));
  CHECK(items[1].events.size() == 1);
}

TEST_CASE("solo plays only one track number in its chain") {
  Song s;
  s.patterns[1].tracks[1] = track("A", {note(0)});
  s.patterns[1].tracks[2] = track("B", {note(0)});
  s.chains['a'] = {entry(1, 1)};
  s.chains['b'] = {entry(1, 1)};
  s.solo_chain = 'a';
  s.solo_track = 2;
  int a = 0, b = 0;
  for (auto& it : render(s, bar4)) (it.chain == 'a' ? a : b)++;
  CHECK(a == 1);
  CHECK(b == 2);
}

TEST_CASE("groove swings off-beats; compress pulls velocity to 64") {
  CHECK(grooved(0.25, 1) == doctest::Approx(0.25));       // 16A = 50 %: straight
  CHECK(grooved(0.25, 6) == doctest::Approx(0.354));      // 16F = 70.8 % of the 1/8 pair
  CHECK(grooved(0.97, 6) == doctest::Approx(1));
  CHECK(grooved(0.6, 10) == doctest::Approx(0.625));      // 8D = 62.5 % of a beat
  CHECK(groove_name(1) == "16A");
  CHECK(groove_name(12) == "8F");
  CHECK(compressed(120, 1) == 92);
  CHECK(compressed(120, 4) == 64);
  CHECK(compressed(80, -1) == 96);
}

TEST_CASE("ghost plays the source's events through its own parameters") {
  Song s;
  s.patterns[1].tracks[1] = track("A", {note(0, 60), note(1, 62)});
  Track g = track("B", {note(2, 40)});
  g.ghost_pattern = 1;
  g.ghost_track = 1;
  g.transpose = 12;
  g.delay = -1;  // negative delay not allowed on ghosts
  s.patterns[1].tracks[2] = g;
  s.chains['a'] = {entry(1, 1)};
  auto items = render(s, bar4);
  std::vector<int> b;
  for (auto& it : items)
    if (it.dest == "B")
      for (auto& e : it.events) b.push_back(e.data1);
  CHECK(b == std::vector<int>{72, 74});
}

TEST_CASE("normalize bakes parameters into events") {
  Track t = track("A", {note(0, 60), note(1, 20)});
  t.transpose = 2;
  t.lowest = 40;
  t.delay = 0.5;
  t.compress = 4;
  t.velocity = 10;
  normalize(t);
  REQUIRE(t.events.size() == 1);
  CHECK(t.events[0].data1 == 62);
  CHECK(t.events[0].pos == 0.5);
  CHECK(t.events[0].data2 == 74);
  CHECK(t.transpose == 0);
  CHECK(t.delay == 0);
  CHECK(t.lowest == 0);
}

TEST_CASE("quantize modes, capture range and strength") {
  Song s;
  Track t = track("A", {{EventType::Note, 0.1, 1, 60, 100, 0.8}});
  t.quantize = 4;
  CHECK(placed(s, t, t.events[0]).len == doctest::Approx(0.8));  // musical keeps length
  s.quant_mode = QuantMode::NoteOn;
  CHECK(placed(s, t, t.events[0]).len == doctest::Approx(0.9));  // end stays
  s.quant_mode = QuantMode::NoteOnOff;
  CHECK(placed(s, t, t.events[0]).len == doctest::Approx(1));
  s.quant_mode = QuantMode::Musical;
  s.quant_strength = 50;
  CHECK(placed(s, t, t.events[0]).pos == doctest::Approx(0.05));
  s.quant_strength = 100;
  s.quant_range = 10;  // 0.1 QN = 19 ticks away: outside
  CHECK(placed(s, t, t.events[0]).pos == doctest::Approx(0.1));
  Event cc{EventType::CC, 0.1, 1, 7, 100};
  s.quant_range = 0;
  CHECK(placed(s, t, cc).pos == doctest::Approx(0.1));
  s.quant_mode = QuantMode::All;
  CHECK(placed(s, t, cc).pos == doctest::Approx(0));
}

TEST_CASE("user groove snaps to a looped template; dynamic groove copies velocities") {
  Song s;
  Track tpl = track("", {{EventType::Note, 0, 1, 60, 40, 0.1}, {EventType::Note, 0.6, 1, 60, 120, 0.1}});
  tpl.loop = 1;
  s.patterns[9].tracks[1] = tpl;
  s.grooves[0] = {9, 1, 0, 100};
  Track t = track("A", {note(2.55)});
  t.groove = 13;
  CHECK(groove_name(13) == "Us1");
  CHECK(placed(s, t, t.events[0]).pos == doctest::Approx(2.6));
  CHECK(dynamic_groove(s, t, 0, 15));
  CHECK(t.events[0].data2 == 120);
}

TEST_CASE("output and channel filters") {
  Track t;
  t.filter = 1 << int(EventType::CC);
  CHECK(filtered(t, {EventType::CC, 0, 1, 7, 1}));
  CHECK(!filtered(t, note(0)));
  t.chan_filter = true;
  t.chan = 2;
  CHECK(filtered(t, note(0)));
}

TEST_CASE("entry params copy forward; move keeping length") {
  Song s;
  Entry a = entry(1, 1);
  a.transpose = 5;
  a.shift = -48;
  a.mutes[3] = true;
  s.chains['a'] = {a, entry(5, 1), entry(9, 1)};
  copy_entry_params(s, 'a', 0, 1, false);
  CHECK(s.chains['a'][1].transpose == 5);
  CHECK(s.chains['a'][2].transpose == 0);
  copy_entry_params(s, 'a', 0, 6, true);
  CHECK(s.chains['a'][2].shift == -48);
  CHECK(s.chains['a'][2].mutes.at(3));
  move_keep_length(s, 'a', 1, 6);
  CHECK(bars(s, 'a') == std::vector<int>{1, 6, 10});
  move_keep_length(s, 'a', 1, 1);  // can't pass the previous entry
  CHECK(bars(s, 'a') == std::vector<int>{1, 6, 10});
}
