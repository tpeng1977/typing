#pragma once
#include <QString>
#include <QVector>

struct Article {
    QString title;
    QString body;
    bool    builtin = false; // 内置文章不可删除
};

// 文章库：内置文章（嵌入资源）+ 用户导入文章（持久化到用户数据目录）
class ArticleStore {
public:
    void load();           // 加载内置 + 导入
    int  count() const { return items_.size(); }
    Article at(int i) const { return items_.at(i); }
    bool  isBuiltin(int i) const { return i >= 0 && i < items_.size() && items_[i].builtin; }

    // 返回新导入文章的索引；失败返回 -1
    int  addImported(const QString& title, const QString& body);
    bool removeAt(int i);  // 仅能删除导入文章
    void persist() const;  // 保存导入文章到磁盘

private:
    QString dataFile() const;
    QVector<Article> items_;
};
