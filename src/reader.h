#pragma once
#include <QWidget>
#include <QTextBrowser>
#include "segmentation.h"

class QLabel;
class QLineEdit;
class QCheckBox;
class QTextBrowser;

// 可感知鼠标悬停 / 点击字词的文本浏览器
class ReaderBrowser : public QTextBrowser {
    Q_OBJECT
public:
    explicit ReaderBrowser(QWidget* parent = nullptr);
signals:
    void tokenHovered(int idx);
    void tokenActivated(int idx);
protected:
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
private:
    int tokenAt(const QPoint& p) const;
};

// 阅读视图：词组分色显示 + 字词五笔/拼音提示
class ReaderView : public QWidget {
    Q_OBJECT
public:
    explicit ReaderView(QWidget* parent = nullptr);
    void setArticle(const QString& title, const QString& body);
    void setHintsEnabled(bool on);   // 关闭后悬停/点击不再显示编码提示
private slots:
    void onHovered(int idx);
    void onActivated(int idx);
    void onLookup();
    void rebuild();
private:
    void buildHtml();
    QString charDetailHtml(const QString& ch) const;
    QString wordDetailHtml(const QString& word) const;
    QString lookupResultHtml(const QString& text) const;

    ReaderBrowser* browser_;
    QLabel*        legend_;
    QLineEdit*     lookup_;
    QCheckBox*     colorChk_;
    QTextBrowser*  detail_;

    QString title_, body_;
    QVector<Token> tokens_;
    bool hintsOn_ = true;
};
