#pragma once
// Matrix editor rules. Notes are addressed by index into Track::events; other event types are never touched.
#include <vector>

#include "model.h"

namespace rotator {

// Nearest multiple of grid; grid <= 0 leaves x as is.
double snap(double x, double grid);

enum class Axis { Undecided, Time, Pitch, Free };
// Locks to whichever axis first moves past threshold (pixels); free unlocks both.
Axis lock_axis(Axis cur, double dpitch_px, double dtime_px, double threshold, bool free);

struct NoteHit {
  int index = -1;
  bool end = false;  // grabbed the end edge (resize)
};
// Beams are at least min_hit long for hit testing; the last `edge` of a beam (and anything past a short one) is the
// end zone. Later-starting notes win.
NoteHit hit_note(const std::vector<Event>&, double pos, int pitch, double min_hit, double edge);

bool same_key(const Event& a, const Event& b);  // both notes, same pitch and channel
bool overlaps(const Event& a, const Event& b);

// Shifts sel by (dpos, dpitch), clamped to pos >= 0 and pitch 0..127. Refuses (returns false, events unchanged) if a
// moved note would overlap a same-key note outside sel.
bool move_notes(std::vector<Event>&, const std::vector<int>& sel, double dpos, int dpitch);
// len += dlen, at least min_len, clipped at the next same-key note.
void resize_notes(std::vector<Event>&, const std::vector<int>& sel, double dlen, double min_len);
void add_velocity(std::vector<Event>&, const std::vector<int>& sel, int dv);
// Refused (-1) if the start falls inside a same-key note; length is clipped at the next one. Returns the new index.
int insert_note(std::vector<Event>&, Event);
void delete_events(std::vector<Event>&, std::vector<int> sel);
// Notes touching the time range [p0, p1] and pitch range [k0, k1] (either order).
std::vector<int> notes_in(const std::vector<Event>&, double p0, double p1, int k0, int k1);

}  // namespace rotator
