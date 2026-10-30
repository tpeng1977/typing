"""生成打字练习软件图标：深蓝底 + 白色键盘 + 五字 + 拼音/英文字母"""
from PIL import Image, ImageDraw, ImageFont
import os

SIZES = [16, 24, 32, 48, 64, 128, 256]
OUT_DIR = r"C:\Users\hypen\Doubao\chats\2026-10-10\new-chat-1\wubi-practice\res"

def make_icon(size):
    scale = 4
    S = size * scale
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # 深蓝圆角背景
    radius = int(S * 0.22)
    d.rounded_rectangle([0, 0, S - 1, S - 1], radius=radius, fill=(30, 64, 175, 255))

    # 白色键盘（3行按键）
    key_color = (255, 255, 255, 230)
    key_h = int(S * 0.10)
    gap = int(S * 0.025)
    rows = 3
    total_h = rows * key_h + (rows - 1) * gap
    start_y = int(S * 0.28)

    # 每行按键数递减（模拟真实键盘）
    keys_per_row = [10, 9, 7]
    for r in range(rows):
        n = keys_per_row[r]
        key_w = int((S * 0.62 - (n - 1) * gap) / n)
        start_x = (S - n * key_w - (n - 1) * gap) // 2
        y = start_y + r * (key_h + gap)
        for k in range(n):
            x = start_x + k * (key_w + gap)
            d.rounded_rectangle(
                [x, y, x + key_w, y + key_h],
                radius=int(S * 0.015),
                fill=key_color,
            )

    # 中央"五"字（叠加在键盘上）
    try:
        font_paths = [
            r"C:\Windows\Fonts\msyhbd.ttc",
            r"C:\Windows\Fonts\simhei.ttf",
            r"C:\Windows\Fonts\simsun.ttc",
        ]
        font = None
        for fp in font_paths:
            if os.path.exists(fp):
                try:
                    font = ImageFont.truetype(fp, int(S * 0.22))
                    break
                except Exception:
                    continue
        if font is None:
            font = ImageFont.load_default()
    except Exception:
        font = ImageFont.load_default()

    text = "五"
    bbox = d.textbbox((0, 0), text, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    tx = (S - tw) // 2 - bbox[0]
    ty = int(S * 0.42) - bbox[1]
    d.text((tx, ty), text, fill=(30, 64, 175, 255), font=font)

    # 底部三个标签：五笔 / 拼音 / ABC
    label_font = None
    for fp in font_paths:
        if os.path.exists(fp):
            try:
                label_font = ImageFont.truetype(fp, int(S * 0.06))
                break
            except Exception:
                continue

    labels = ["五", "拼", "ABC"]
    label_w = int(S * 0.18)
    label_gap = int(S * 0.03)
    total_lw = len(labels) * label_w + (len(labels) - 1) * label_gap
    lx = (S - total_lw) // 2
    ly = int(S * 0.78)
    for i, lbl in enumerate(labels):
        x = lx + i * (label_w + label_gap)
        d.rounded_rectangle(
            [x, ly, x + label_w, ly + int(S * 0.07)],
            radius=int(S * 0.015),
            fill=(255, 255, 255, 200),
        )
        bbox2 = d.textbbox((0, 0), lbl, font=label_font)
        lw = bbox2[2] - bbox2[0]
        lh = bbox2[3] - bbox2[1]
        d.text((x + (label_w - lw) // 2 - bbox2[0],
                ly + (int(S * 0.07) - lh) // 2 - bbox2[1]),
               lbl, fill=(30, 64, 175, 255), font=label_font)

    img = img.resize((size, size), Image.LANCZOS)
    return img

icons = [make_icon(s) for s in SIZES]

png_path = os.path.join(OUT_DIR, "app_icon.png")
icons[-1].save(png_path, "PNG")
print(f"PNG saved: {png_path}")

ico_path = os.path.join(OUT_DIR, "app_icon.ico")
icons[0].save(ico_path, format="ICO",
              sizes=[(s, s) for s in SIZES], append_images=icons[1:])
print(f"ICO saved: {ico_path}")
