#include "bilingual.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

static QString readResource(const char* path)
{
    QFile f(QString::fromLatin1(path));
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll());
}

bool BilingualStore::load()
{
    arts_.clear();
    enwords_.clear();

    // ---- 逐句中英对照：[{title, pairs:[{zh,start,en}]}] ----
    const QString sj = readResource(":/data/sentences.json");
    if (sj.isEmpty()) return false;
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(sj.toUtf8(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isArray())
        return false;
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject o = v.toObject();
        const QString title = o.value("title").toString();
        QVector<SentencePair> pairs;
        const QJsonArray arr = o.value("pairs").toArray();
        for (const QJsonValue& pv : arr) {
            const QJsonObject po = pv.toObject();
            SentencePair p;
            p.zh = po.value("zh").toString();
            p.start = po.value("start").toInt();
            p.en = po.value("en").toString();
            pairs.append(p);
        }
        if (!title.isEmpty() && !pairs.isEmpty())
            arts_.insert(title, pairs);
    }

    // ---- 英文单词释义：{word: 释义} ----
    const QString ew = readResource(":/data/enwords.json");
    if (ew.isEmpty()) return false;
    QJsonParseError eperr;
    const QJsonDocument edoc = QJsonDocument::fromJson(ew.toUtf8(), &eperr);
    if (eperr.error != QJsonParseError::NoError || !edoc.isObject())
        return false;
    const QJsonObject eo = edoc.object();
    for (auto it = eo.begin(); it != eo.end(); ++it)
        enwords_.insert(it.key(), it.value().toString());

    loaded_ = true;
    return true;
}

QVector<SentencePair> BilingualStore::sentences(const QString& title) const
{
    return arts_.value(title);
}

QString BilingualStore::meaning(const QString& word) const
{
    // 规范化：去首尾标点、转小写、去掉所有格 's
    QString w = word.trimmed();
    static const QRegularExpression punct(QStringLiteral("^[\\W_]+|[\\W_]+$"));
    w = w.remove(punct);
    w = w.toLower();
    if (w.isEmpty()) return QString();
    auto it = enwords_.constFind(w);
    if (it != enwords_.constEnd())
        return it.value();
    if (w.endsWith(QLatin1Char('\''))) w.chop(1);
    if (w.endsWith(QStringLiteral("'s"))) w.chop(2);
    it = enwords_.constFind(w);
    return it != enwords_.constEnd() ? it.value() : QString();
}

BilingualStore& theBilingual()
{
    static BilingualStore* b = [] {
        BilingualStore* p = new BilingualStore();
        p->load();
        return p;
    }();
    return *b;
}
