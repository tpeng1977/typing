#pragma once
#include <QWidget>
#include "article.h"

class QLineEdit;
class QPlainTextEdit;
class QListWidget;
class QPushButton;

// 导入新文章（从文件或粘贴文本），并管理用户导入的文章
class ImportWidget : public QWidget {
    Q_OBJECT
public:
    explicit ImportWidget(ArticleStore* store, QWidget* parent = nullptr);
    void refresh();
signals:
    void requestOpen(int index);
private slots:
    void onFile();
    void onSave();
    void onDelete();
    void onOpen();
private:
    ArticleStore* store_;
    QLineEdit*    title_;
    QPlainTextEdit* content_;
    QListWidget*  list_;
    QPushButton *fileBtn_, *saveBtn_, *delBtn_, *openBtn_;
};
