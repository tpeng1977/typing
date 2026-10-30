#pragma once
#include <QString>
#include <QStringList>
#include <QHash>

// 单字信息
struct CharEntry {
    QStringList wubi;   // codes_[0] 为全码，其余为各级简码（按长度升序）
    QStringList pinyins; // 拼音列表，[0] 为常用读音
};

// 全局五笔 / 拼音 / 词语词典
class Dict {
public:
    bool loadAll();                       // 从嵌入资源加载，成功返回 true
    bool ok() const { return loaded_; }

    // 单字
    bool hasChar(const QString& c) const { return chars_.contains(c); }
    QString wubiFull(const QString& c) const;       // 全码
    QStringList wubiBriefs(const QString& c) const; // 各级简码
    QStringList allWubi(const QString& c) const;    // 全码 + 简码
    QString pinyin(const QString& c) const;         // 常用读音
    QStringList pinyins(const QString& c) const;

    // 词语
    bool hasWord(const QString& w) const { return words_.contains(w); }
    QString wordCode(const QString& w) const;       // 优先查表，其次按规则计算

    static QString computeWordCode(const QString& w, const Dict& d);

private:
    bool loaded_ = false;
    QHash<QString, CharEntry> chars_;
    QHash<QString, QString>   words_; // 词 -> 编码
};

// 全局词典实例（懒加载）
Dict& theDict();
