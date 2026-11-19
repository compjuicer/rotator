#include "reaper_core_api.h"
#include "reaper_out.h"

#include <algorithm>
#include <optional>
#include <unordered_map>

namespace {

const char* kTag = "P_EXT:rotator";

void delete_generated() {
  for (int i = CountMediaItems(nullptr) - 1; i >= 0; --i) {
    MediaItem* item = GetMediaItem(nullptr, i);
    char tag[64] = "";
    if (GetSetMediaItemInfo_String(item, kTag, tag, false) && tag[0])
      DeleteTrackMediaItem(GetMediaItem_Track(item), item);
  }
}

void insert_cc(MediaItem_Take* take, double ppq, const rotator::Event& e) {
  using T = rotator::EventType;
  int c = e.chan - 1;
  switch (e.type) {
    case T::CC: MIDI_InsertCC(take, false, false, ppq, 0xB0, c, e.data1, e.data2); break;
    case T::Program: MIDI_InsertCC(take, false, false, ppq, 0xC0, c, e.data1, 0); break;
    case T::PitchBend: {
      int v = e.data2 + 8192;
      MIDI_InsertCC(take, false, false, ppq, 0xE0, c, v & 0x7F, v >> 7);
      break;
    }
    case T::ChanPressure: MIDI_InsertCC(take, false, false, ppq, 0xD0, c, e.data2, 0); break;
    case T::PolyPressure: MIDI_InsertCC(take, false, false, ppq, 0xA0, c, e.data1, e.data2); break;
    case T::Note: break;
  }
}

std::optional<rotator::Event> from_cc(int chanmsg, int chan, int msg2, int msg3) {
  using T = rotator::EventType;
  switch (chanmsg) {
    case 0xB0: return rotator::Event{T::CC, 0, chan + 1, msg2, msg3};
    case 0xC0: return rotator::Event{T::Program, 0, chan + 1, msg2, 0};
    case 0xE0: return rotator::Event{T::PitchBend, 0, chan + 1, 0, (msg3 << 7 | msg2) - 8192};
    case 0xD0: return rotator::Event{T::ChanPressure, 0, chan + 1, 0, msg2};
    case 0xA0: return rotator::Event{T::PolyPressure, 0, chan + 1, msg2, msg3};
  }
  return std::nullopt;
}

}  // namespace

double bar_to_qn(int bar) {
  double qn = 0;
  TimeMap_GetMeasureInfo(nullptr, bar - 1, &qn, nullptr, nullptr, nullptr, nullptr);
  return qn;
}

std::string track_guid(MediaTrack* tr) {
  char buf[64];
  guidToString(GetTrackGUID(tr), buf);
  return buf;
}

void render_to_project(const rotator::Song& song) {
  Undo_BeginBlock();
  PreventUIRefresh(1);
  delete_generated();

  std::unordered_map<std::string, MediaTrack*> tracks;
  for (int i = 0; i < CountTracks(nullptr); ++i) {
    MediaTrack* tr = GetTrack(nullptr, i);
    tracks[track_guid(tr)] = tr;
  }

  const bool yes = true;
  for (const auto& it : rotator::render(song, bar_to_qn)) {
    auto found = tracks.find(it.dest);
    if (found == tracks.end()) continue;
    MediaItem* item = CreateNewMIDIItemInProj(found->second, it.start, it.finish, &yes);
    MediaItem_Take* take = GetActiveTake(item);

    std::string name = it.entry.name;
    if (name.empty()) {
      auto p = song.patterns.find(it.entry.pattern);
      if (p != song.patterns.end()) name = p->second.name;
    }
    if (name.empty()) name = "P" + std::to_string(it.entry.pattern);
    std::string take_name = std::string(1, it.chain) + ": " + name;
    GetSetMediaItemTakeInfo_String(take, "P_NAME", take_name.data(), true);
    char tag[2] = {it.chain, 0};
    GetSetMediaItemInfo_String(item, kTag, tag, true);

    for (const auto& e : it.events) {
      double s = MIDI_GetPPQPosFromProjQN(take, e.pos);
      if (e.type == rotator::EventType::Note)
        MIDI_InsertNote(take, false, false, s, MIDI_GetPPQPosFromProjQN(take, e.pos + e.len), e.chan - 1, e.data1,
                        e.data2, &yes);
      else
        insert_cc(take, s, e);
    }
    MIDI_Sort(take);
  }

  PreventUIRefresh(-1);
  UpdateArrange();
  Undo_EndBlock("Rotator: render arrange", -1);
}

void commit(ReaProject* proj, const rotator::Song& song, const char* undo_desc) {
  Undo_BeginBlock2(proj);
  render_to_project(song);
  Undo_EndBlock2(proj, undo_desc, UNDO_STATE_ALL);
}

bool capture_selected_item(ReaProject* proj, rotator::Track& track) {
  for (int i = 0; i < CountSelectedMediaItems(proj); ++i) {
    MediaItem* item = GetSelectedMediaItem(proj, i);
    MediaItem_Take* take = GetActiveTake(item);
    if (!take || !TakeIsMIDI(take)) continue;
    double pos = GetMediaItemInfo_Value(item, "D_POSITION");
    double start = TimeMap2_timeToQN(proj, pos);
    double end = TimeMap2_timeToQN(proj, pos + GetMediaItemInfo_Value(item, "D_LENGTH")) - start;
    int notes = 0, ccs = 0;
    MIDI_CountEvts(take, &notes, &ccs, nullptr);
    track.events.clear();
    for (int n = 0; n < notes; ++n) {
      double s = 0, e = 0;
      int chan = 0, pitch = 0, vel = 0;
      MIDI_GetNote(take, n, nullptr, nullptr, &s, &e, &chan, &pitch, &vel);
      double sq = MIDI_GetProjQNFromPPQPos(take, s) - start, eq = MIDI_GetProjQNFromPPQPos(take, e) - start;
      if (sq < 0 || sq >= end) continue;
      track.events.push_back({rotator::EventType::Note, sq, chan + 1, pitch, vel, eq - sq});
    }
    for (int n = 0; n < ccs; ++n) {
      double s = 0;
      int chanmsg = 0, chan = 0, msg2 = 0, msg3 = 0;
      MIDI_GetCC(take, n, nullptr, nullptr, &s, &chanmsg, &chan, &msg2, &msg3);
      double sq = MIDI_GetProjQNFromPPQPos(take, s) - start;
      auto ev = from_cc(chanmsg, chan, msg2, msg3);
      if (sq < 0 || sq >= end || !ev) continue;
      ev->pos = sq;
      track.events.push_back(*ev);
    }
    std::stable_sort(track.events.begin(), track.events.end(),
                     [](const rotator::Event& a, const rotator::Event& b) { return a.pos < b.pos; });
    return true;
  }
  return false;
}

MediaTrack* dest_track(const std::string& guid) {
  if (guid.empty()) return nullptr;
  for (int i = 0; i < CountTracks(nullptr); ++i)
    if (track_guid(GetTrack(nullptr, i)) == guid) return GetTrack(nullptr, i);
  return nullptr;
}
