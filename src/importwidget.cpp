#include "importwidget.h"
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>

ImportWidget::ImportWidget(ArticleStore* store, QWidget* parent)
    : QWidget(parent), store_(store)
{
    title_ = new QLineEdit(this);
    title_->setPlaceholderText(QStringLiteral("文章标题（留空将自动命名）"));

    content_ = new QPlainTextEdit(this);
    content_->setPlaceholderText(QStringLiteral("在此粘贴或导入文章正文内容…"));
    QFont f = content_->font();
    f.setPointSize(12);
    content_->setFont(f);

    fileBtn_ = new QPushButton(QStringLiteral("从文件导入…"), this);
    saveBtn_ = new QPushButton(QStringLiteral("保存到文章库"), this);
    saveBtn_->setStyleSheet("QPushButton{background:#1d4ed8;color:#fff;border:none;border-radius:6px;"
                            "padding:8px 16px;font-weight:bold;}QPushButton:hover{background:#1e40af;}");

    QHBoxLayout* btns = new QHBoxLayout;
    btns->addWidget(fileBtn_);
    btns->addStretch();
    btns->addWidget(saveBtn_);

    QGroupBox* inBox = new QGroupBox(QStringLiteral("新建 / 导入文章"), this);
    QVBoxLayout* inLay = new QVBoxLayout(inBox);
    inLay->addWidget(new QLabel(QStringLiteral("标题："), this));
    inLay->addWidget(title_);
    inLay->addWidget(new QLabel(QStringLiteral("正文："), this));
    inLay->addWidget(content_, 1);
    inLay->addLayout(btns);

    list_ = new QListWidget(this);
    delBtn_ = new QPushButton(QStringLiteral("删除所选"), this);
    openBtn_ = new QPushButton(QStringLiteral("打开所选"), this);
    QHBoxLayout* listBtns = new QHBoxLayout;
    listBtns->addStretch();
    listBtns->addWidget(openBtn_);
    listBtns->addWidget(delBtn_);

    QGroupBox* libBox = new QGroupBox(QStringLiteral("已导入的文章"), this);
    QVBoxLayout* libLay = new QVBoxLayout(libBox);
    libLay->addWidget(list_, 1);
    libLay->addLayout(listBtns);

    QHBoxLayout* main = new QHBoxLayout(this);
    main->addWidget(inBox, 3);
    main->addWidget(libBox, 2);

    connect(fileBtn_, &QPushButton::clicked, this, &ImportWidget::onFile);
    connect(saveBtn_, &QPushButton::clicked, this, &ImportWidget::onSave);
    connect(delBtn_, &QPushButton::clicked, this, &ImportWidget::onDelete);
    connect(openBtn_, &QPushButton::clicked, this, &ImportWidget::onOpen);
    connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*){ onOpen(); });

    refresh();
}

void ImportWidget::refresh()
{
    list_->clear();
    const int n = store_->count();
    for (int i = 0; i < n; ++i) {
        if (store_->isBuiltin(i)) continue;
        const Article a = store_->at(i);
        QString label = a.title + QStringLiteral("  （%1 字）").arg(a.body.size());
        list_->addItem(label);
    }
}

void ImportWidget::onFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择文本文件"), QString(),
        QStringLiteral("文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("导入失败"), QStringLiteral("无法读取该文件。"));
        return;
    }
    const QString text = QString::fromUtf8(f.readAll());
    content_->setPlainText(text.trimmed());
    // 以文件名作默认标题
    if (title_->text().trimmed().isEmpty()) {
        QFileInfo fi(path);
        title_->setText(fi.completeBaseName());
    }
}

void ImportWidget::onSave()
{
    const int idx = store_->addImported(title_->text(), content_->toPlainText());
    if (idx < 0) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QStringLiteral("正文内容不能为空，请先输入或导入文章正文。"));
        return;
    }
    QMessageBox::information(this, QStringLiteral("已保存"),
                             QStringLiteral("文章已加入文章库，可在「文章库」或左侧列表中打开。"));
    title_->clear();
    content_->clear();
    refresh();
}

void ImportWidget::onDelete()
{
    const int row = list_->currentRow();
    if (row < 0) return;
    int target = -1, seen = -1;
    const int n = store_->count();
    for (int i = 0; i < n; ++i) {
        if (store_->isBuiltin(i)) continue;
        ++seen;
        if (seen == row) { target = i; break; }
    }
    if (target < 0) return;
    if (QMessageBox::question(this, QStringLiteral("删除文章"),
                              QStringLiteral("确定删除这篇导入的文章吗？")) == QMessageBox::Yes) {
        store_->removeAt(target);
        refresh();
    }
}

void ImportWidget::onOpen()
{
    const int row = list_->currentRow();
    if (row < 0) return;
    int target = -1, seen = -1;
    const int n = store_->count();
    for (int i = 0; i < n; ++i) {
        if (store_->isBuiltin(i)) continue;
        ++seen;
        if (seen == row) { target = i; break; }
    }
    if (target >= 0) emit requestOpen(target);
}
