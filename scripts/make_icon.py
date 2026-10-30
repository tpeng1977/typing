#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""从 res/app_icon_512.png 生成各尺寸 app_icon.png / app_icon.ico。

用法（在工程根目录）:
    python3 scripts/make_icon.py
"""
from __future__ import annotations

import os
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
RES = os.path.join(ROOT, "res")
SRC = os.path.join(RES, "app_icon_512.png")
SIZES = [16, 24, 32, 48, 64, 128, 256]


def main() -> None:
    if not os.path.isfile(SRC):
        raise SystemExit(f"缺少源图: {SRC}")
    src = Image.open(SRC).convert("RGBA")
    icons = [src.resize((s, s), Image.Resampling.LANCZOS) for s in SIZES]

    png = os.path.join(RES, "app_icon.png")
    icons[-1].save(png, "PNG")
    print(f"PNG saved: {png} ({icons[-1].size[0]}x{icons[-1].size[1]})")

    ico = os.path.join(RES, "app_icon.ico")
    icons[-1].save(
        ico,
        format="ICO",
        sizes=[(s, s) for s in SIZES],
        append_images=icons[:-1],
    )
    print(f"ICO saved: {ico} ({os.path.getsize(ico)} bytes)")


if __name__ == "__main__":
    main()
