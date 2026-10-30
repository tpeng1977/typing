# -*- coding: utf-8 -*-
"""把子代理合并的 20 篇文章拆分为独立文件，并重新生成 res/articles.json。"""
import os, json, re

BASE = os.path.dirname(os.path.abspath(__file__))
ART  = os.path.join(BASE, "..", "articles")
RES  = os.path.join(BASE, "..", "res")
SRC  = os.path.join(BASE, "..", "..",
    ".sessions", "38446481629243650", "agents", "o_000ERNAg64o", "artifacts", "五笔练习文章20篇.txt")

# 用规范路径；若相对路径定位不到，直接采用绝对路径
ABS_SRC = r"C:\Users\hypen\AppData\Local\Doubao\User Data\Default\.doubao\agent_mode\workspace\.sessions\38446481629243650\agents\o_000ERNAg64o\artifacts\五笔练习文章20篇.txt"
if not os.path.exists(SRC):
    SRC = ABS_SRC

with open(SRC, encoding='utf-8') as f:
    content = f.read()

# 按空行分块
blocks = [b.strip() for b in re.split(r'\n\s*\n', content) if b.strip()]
print("blocks:", len(blocks))

os.makedirs(ART, exist_ok=True)
# 先清理旧的 art*.txt
for fn in os.listdir(ART):
    if fn.startswith("art") and fn.endswith(".txt"):
        os.remove(os.path.join(ART, fn))

articles = []
for i, blk in enumerate(blocks):
    lines = blk.split('\n')
    title = lines[0].strip()
    body = ''.join(l.strip() for l in lines[1:])
    body = body.strip()
    if not body:
        continue
    articles.append({"title": title, "body": body})
    fn = os.path.join(ART, "art%02d.txt" % (i + 1))
    with open(fn, "w", encoding='utf-8', newline='\n') as f:
        f.write(title + "\n" + body + "\n")
    print("wrote", fn, "len=", len(body))

print("articles parsed:", len(articles))

# 写 res/articles.json
with open(os.path.join(RES, "articles.json"), "w", encoding='utf-8') as f:
    json.dump(articles, f, ensure_ascii=False, indent=0)

# 统计
lens = [len(a['body']) for a in articles]
print("total chars:", sum(lens), "min:", min(lens), "max:", max(lens))
print("DONE")
