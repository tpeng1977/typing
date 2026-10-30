#include "article.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

static QString readResource(const char* path)
{
    QFile f(QString::fromLatin1(path));
    if (!f.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(f.readAll());
}

void ArticleStore::load()
{
    items_.clear();

    // 内置文章
    const QString js = readResource(":/data/articles.json");
    if (!js.isEmpty()) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(js.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isArray()) {
            const QJsonArray arr = doc.array();
            for (const QJsonValue& v : arr) {
                QJsonObject o = v.toObject();
                Article a;
                a.title = o.value("title").toString();
                a.body  = o.value("body").toString();
                a.builtin = true;
                if (!a.body.isEmpty())
                    items_.append(a);
            }
        }
    }

    // 用户导入文章
    QFile f(dataFile());
    if (f.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isArray()) {
            const QJsonArray arr = doc.array();
            for (const QJsonValue& v : arr) {
                QJsonObject o = v.toObject();
                Article a;
                a.title = o.value("title").toString();
                a.body  = o.value("body").toString();
                a.builtin = false;
                if (!a.body.isEmpty())
                    items_.append(a);
            }
        }
    }
}

int ArticleStore::addImported(const QString& title, const QString& body)
{
    QString t = title.trimmed();
    QString b = body.trimmed();
    if (b.isEmpty()) return -1;
    if (t.isEmpty()) t = QStringLiteral("未命名文章");
    Article a;
    a.title = t;
    a.body  = b;
    a.builtin = false;
    items_.append(a);
    persist();
    return items_.size() - 1;
}

bool ArticleStore::removeAt(int i)
{
    if (i < 0 || i >= items_.size()) return false;
    if (items_[i].builtin) return false;
    items_.removeAt(i);
    persist();
    return true;
}

QString ArticleStore::dataFile() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/wubi_practice_articles.json");
}

void ArticleStore::persist() const
{
    QJsonArray arr;
    for (const Article& a : items_) {
        if (a.builtin) continue;
        QJsonObject o;
        o.insert("title", a.title);
        o.insert("body", a.body);
        arr.append(o);
    }
    const QString path = dataFile();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }
}
