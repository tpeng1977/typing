#pragma once
#include <QString>
#include <QVector>
#include <QHash>

// 一句中英对照（zh 为中文原句，start 为中文句在正文中的起始偏移，en 为英文译文）
struct SentencePair {
    QString zh;
    int     start = 0;
    QString en;
};

// 双语数据：20 篇内置文章的逐句中英对照 + 英文单词释义词典
class BilingualStore {
public:
    bool load();                     // 从嵌入资源加载，成功返回 true
    bool ok() const { return loaded_; }

    // 按文章标题取逐句对照；无译文返回空
    QVector<SentencePair> sentences(const QString& title) const;

    // 英文单词释义（自动去标点/小写/去所有格's），查不到返回空
    QString meaning(const QString& word) const;

    int articleCount() const { return arts_.size(); }
    int wordCount() const { return enwords_.size(); }
private:
    bool loaded_ = false;
    QHash<QString, QVector<SentencePair>> arts_;
    QHash<QString, QString> enwords_;
};

// 全局双语数据实例（懒加载）
BilingualStore& theBilingual();
