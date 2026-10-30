#include "dataloader.h"
#include <QFile>
#include <QTextStream>

static QString readResource(const char* path)
{
    QFile f(QString::fromLatin1(path));
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll());
}

bool Dict::loadAll()
{
    chars_.clear();
    words_.clear();

    // ---- 单字五笔码表: 字 \t 全码,简码1,简码2... ----
    const QString wubi = readResource(":/data/wubi_char.txt");
    if (wubi.isEmpty()) return false;
    const QStringList lines = wubi.split('\n', Qt::SkipEmptyParts);
    for (const QString& ln : lines) {
        const QStringList f = ln.split('\t');
        if (f.size() < 2) continue;
        const QString ch = f[0];
        if (ch.size() != 1) continue;
        const QStringList codes = f[1].split(',', Qt::SkipEmptyParts);
        if (codes.isEmpty()) continue;
        CharEntry e;
        for (const QString& c : codes) {
            const QString cc = c.trimmed().toLower();
            if (!cc.isEmpty()) e.wubi.append(cc);
        }
        if (!e.wubi.isEmpty())
            chars_.insert(ch, e);
    }

    // ---- 拼音: 字 \t p1,p2,... ----
    const QString py = readResource(":/data/pinyin.txt");
    if (py.isEmpty()) return false;
    const QStringList pylines = py.split('\n', Qt::SkipEmptyParts);
    for (const QString& ln : pylines) {
        const QStringList f = ln.split('\t');
        if (f.size() < 2) continue;
        const QString ch = f[0];
        if (ch.size() != 1) continue;
        QStringList pys;
        for (const QString& p : f[1].split(',', Qt::SkipEmptyParts)) {
            const QString pp = p.trimmed();
            if (!pp.isEmpty()) pys.append(pp);
        }
        if (pys.isEmpty()) continue;
        auto it = chars_.find(ch);
        if (it != chars_.end())
            it.value().pinyins = pys;
        else {
            CharEntry e; e.pinyins = pys; chars_.insert(ch, e);
        }
    }

    // ---- 词语编码: 词 \t 编码 ----
    const QString words = readResource(":/data/words.txt");
    if (words.isEmpty()) return false;
    const QStringList wlines = words.split('\n', Qt::SkipEmptyParts);
    for (const QString& ln : wlines) {
        const QStringList f = ln.split('\t');
        if (f.size() < 2) continue;
        const QString w = f[0].trimmed();
        if (w.size() < 2) continue;
        words_.insert(w, f[1].trimmed().toLower());
    }

    loaded_ = true;
    return true;
}

QString Dict::wubiFull(const QString& c) const
{
    auto it = chars_.constFind(c);
    if (it == chars_.constEnd() || it.value().wubi.isEmpty()) return QString();
    return it.value().wubi.first();
}

QStringList Dict::wubiBriefs(const QString& c) const
{
    auto it = chars_.constFind(c);
    if (it == chars_.constEnd()) return {};
    QStringList b = it.value().wubi.mid(1);
    return b;
}

QStringList Dict::allWubi(const QString& c) const
{
    auto it = chars_.constFind(c);
    if (it == chars_.constEnd()) return {};
    return it.value().wubi;
}

QString Dict::pinyin(const QString& c) const
{
    auto it = chars_.constFind(c);
    if (it == chars_.constEnd() || it.value().pinyins.isEmpty()) return QString();
    return it.value().pinyins.first();
}

QStringList Dict::pinyins(const QString& c) const
{
    auto it = chars_.constFind(c);
    if (it == chars_.constEnd()) return {};
    return it.value().pinyins;
}

QString Dict::wordCode(const QString& w) const
{
    auto it = words_.constFind(w);
    if (it != words_.constEnd())
        return it.value();
    return computeWordCode(w, *this);
}

// 五笔词语编码规则（前三个字取首码、末字取首码等）
QString Dict::computeWordCode(const QString& w, const Dict& d)
{
    if (w.size() == 1) return d.wubiFull(w);
    auto first = [&](const QString& c) { return d.wubiFull(c).left(1); };
    auto first2 = [&](const QString& c) { return d.wubiFull(c).left(2); };

    if (w.size() == 2)
        return first2(QString(w[0])) + first2(QString(w[1]));
    if (w.size() == 3)
        return first(QString(w[0])) + first(QString(w[1])) + first2(QString(w[2]));
    // 四字及以上
    return first(QString(w[0])) + first(QString(w[1]))
         + first(QString(w[2])) + first(QString(w[w.size() - 1]));
}

Dict& theDict()
{
    static Dict* d = [] {
        Dict* p = new Dict();
        p->loadAll();
        return p;
    }();
    return *d;
}
