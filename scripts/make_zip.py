# -*- coding: utf-8 -*-
"""重建 typing-源码.zip：排除构建产物与大体积原始数据。"""
import os
import zipfile

root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
out = os.path.join(root, "typing-源码.zip")
skip_dirs = {"build", "dist", "_shots", ".git", "__pycache__"}
skip_suffixes = (".exe", ".dll", ".o", ".obj", ".zip")

paths = []
for dirpath, dirnames, filenames in os.walk(root):
    dirnames[:] = [d for d in dirnames if d not in skip_dirs]
    for fn in filenames:
        if fn.endswith(skip_suffixes):
            continue
        full = os.path.join(dirpath, fn)
        rel = os.path.relpath(full, root)
        paths.append((full, rel))

paths.sort(key=lambda p: p[1])
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for full, rel in paths:
        z.write(full, os.path.join("typing", rel))

kb = os.path.getsize(out) // 1024
print(f"zipKB={kb} files={len(paths)}")
