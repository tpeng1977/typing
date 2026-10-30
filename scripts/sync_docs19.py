# -*- coding: utf-8 -*-
"""更新日志加 0.1.9 条目；README 加修复说明。"""
import io

# 更新日志
p1 = r"C:\Users\hypen\Doubao\chats\2026-10-10\new-chat-1\wubi-practice\Docs\更新日志.md"
s1 = io.open(p1, encoding='utf-8').read()
anchor = "# 更新日志\n\n## 0.1.8 — 2026-10-16"
entry = """# 更新日志

## 0.1.9 — 2026-10-16

**修复（逐字输入被吞 / 输入一两个字后输入区不再接收新字符）**
- 0.1.8 为支持跨行词组，把输入匹配起点从“当前行行首”改为“当前位置（absPos）”，
  但输入框内容是从**当前行行首累积**的（含已匹配前缀），二者错位导致：
  真实输入法逐字输入时，第二个字起前缀比对失败并被截断回原样，
  表现即“输入一个字/两个字后输入区不再接收新字符”。
- 0.1.9 将匹配起点改回**当前行行首** `lineStarts_[lineIdx_]`，与输入框“行首累积”语义一致；
  跨行词组能力不变（输入框与“行首到文末的连续正文”前缀匹配，仍可一次输入跨行词组）。
- 同时保留：单行内连续正确输入时不再重写输入框（避免干扰系统输入法），
  仅在跨行截断 / 出错截断时改写输入框。
- 新增 `--ime-test` 自检模式：用与真实输入法相同的 `QInputMethodEvent` 提交路径
  逐字模拟输入，验证连续输入、出错截断、截断后继续输入三条链路。

## 0.1.8 — 2026-10-16"""
assert anchor in s1, "更新日志锚点未找到"
s1 = s1.replace(anchor, entry, 1)
io.open(p1, 'w', encoding='utf-8', newline='\n').write(s1)
print("更新日志 OK")

# README
p2 = r"C:\Users\hypen\Doubao\chats\2026-10-10\new-chat-1\wubi-practice\README.md"
s2 = io.open(p2, encoding='utf-8').read()
anchor2 = "  连续 5 秒无输入自动暂停，暂停期间不计时、不计速度。"
add = """  连续 5 秒无输入自动暂停，暂停期间不计时、不计速度。
  **0.1.9 修复**：真实输入法逐字输入时不再出现“输入一两个字后输入区不接收新字符”
  （匹配起点与输入框行首累积语义对齐；跨行词组整词输入能力保持不变）。"""
assert anchor2 in s2, "README 锚点未找到"
s2 = s2.replace(anchor2, add, 1)
io.open(p2, 'w', encoding='utf-8', newline='\n').write(s2)
print("README OK")
