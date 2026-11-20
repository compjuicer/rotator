#pragma once
// Song <-> RPP-style text lines, stored inside a <ROTATOR ... > block in the project file.
// One record per line, tokens quoted RPP-style (WDL LineParser rules):
//   PATTERN <n> "<name>"
//   TRACK <n> "<name>" "<dest guid>" <chan|0=original> <transpose> <velocity> <loop|0=off> <delay> <lowest> <highest> <mute> <hide> [<quantize|0=off> [<groove> <compress> <ghost pattern> <ghost track> [<filter bits> <chan filter>]]]
//   events of the preceding TRACK (bend is signed -8192..8191):
//     N <pos> <len> <pitch> <vel> <chan>                     note
//     C <pos> <chan> <controller> <value>                    control change
//     P <pos> <chan> <program>                               program change
//     B <pos> <chan> <bend>                                  pitch bend
//     A <pos> <chan> <value>                                 channel pressure
//     K <pos> <chan> <pitch> <value>                         poly pressure
//     E <pos> <len> <pitch> <vel> <chan>                     legacy note, read only
//   ENTRY <chain a-d> <bar> <pattern> <transpose> "<name>" "<muted track numbers, comma-separated>" [<shift ticks: <0 upbeat, >0 cut>]
//   HTRACK <hyper set index|-1>                          hyper set of the preceding TRACK (lane delays)
//   HSET "<name>"                                        hyper edit set, in order
//   HLANE "<name>" <type N|C|P|B|A|K> <data1|-1=all> <chan|0=any> <grid> <len> <delay> <hhat>
//                                                        lane of the preceding HSET; times in QN
//   QMODE <0 musical|1 on+off|2 on|3 all> <range ticks> <strength %>
//   UGROOVE <1-16> <pattern> <track> <range ticks> <strength %>
//   MGROUP <1-10> "<muted track numbers>"
//   SOLO <chain> <track>                                 only that track number plays in that chain
//   NOXPOSE "<dest guid>" <chan 1-16>                    arrange transpose ignored for this destination+channel
//   NOTE "<text line>"                                   one line of the song notes, in order
// Unknown records are ignored so the format can grow.
#include <string>
#include <vector>

#include "model.h"

namespace rotator {

std::vector<std::string> to_lines(const Song&);
Song from_lines(const std::vector<std::string>&);

}  // namespace rotator
