"""Pick distinct, sharp Atari-screen frames from the 1fps dump.

usage: frames.py <tiny.rgb> <frames1s dir> <out dir>
tiny.rgb: 160x120 rgb24 frames at 1 fps, same order as frames1s/tNNNNN.jpg.
"""
import os
import shutil
import sys

import numpy as np

W, H = 160, 120
raw, src, out = sys.argv[1:4]
v = np.fromfile(raw, np.uint8).reshape(-1, H, W, 3).astype(np.float32)
luma = v @ np.array([0.299, 0.587, 0.114], np.float32)
sat = v.max(-1) - v.min(-1)

c = (slice(15, 105), slice(20, 140))
bright = ((luma[:, c[0], c[1]] > 165) & (sat[:, c[0], c[1]] < 70)).mean((1, 2))
dark = (luma[:, c[0], c[1]] < 80).mean((1, 2))
gx = np.abs(np.diff(luma[:, c[0], c[1]], axis=2)).mean((1, 2))
screen = (bright > 0.45) & (dark > 0.04) & (gx > 9)

sharp = np.abs(np.diff(luma, axis=1)).mean((1, 2)) + np.abs(np.diff(luma, axis=2)).mean((1, 2))

os.makedirs(out, exist_ok=True)
picked, group = [], []


def flush():
    if group:
        best = max(group, key=lambda i: sharp[i])
        picked.append(best)
        group.clear()


for i in range(len(v)):
    if not screen[i]:
        flush()
        continue
    if group and np.abs(luma[i] - luma[group[0]]).mean() > 6:
        flush()
    group.append(i)
flush()

with open(os.path.join(out, "index.txt"), "w") as f:
    for i in picked:
        name = f"{i // 3600}-{i % 3600 // 60:02d}-{i % 60:02d}.jpg"
        shutil.copy(os.path.join(src, f"t{i + 1:05d}.jpg"), os.path.join(out, name))
        f.write(f"{name}\n")
print(f"frames {len(v)} screen {int(screen.sum())} picked {len(picked)}")
