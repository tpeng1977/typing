#include "reader.h"
#include "dataloader.h"
#include <QMouseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextBrowser>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFrame>

// 词组分色配色
static const char* kSingle = "#333a45"; // 单字 / 标点
static const char* kWord2  = "#1d4ed8"; // 二字词
static const char* kWord3  = "#0f766e"; // 三字词
static const char* kWord4  = "#7c3aed"; // 四字及以上

static QString esc(const QString& s)
{
    QString out = s;
    out.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
    return out;
}

// ---------------- ReaderBrowser ----------------
ReaderBrowser::ReaderBrowser(QWidget* parent) : QTextBrowser(parent)
{
    viewport()->setMouseTracking(true);
    setOpenLinks(false);
    setOpenExternalLinks(false);
}

int ReaderBrowser::tokenAt(const QPoint& p) const
{
    const QString href = anchorAt(p);
    if (href.startsWith("tok:")) {
        bool ok = false;
        const int idx = href.mid(4).toInt(&ok);
        if (ok) return idx;
    }
    return -1;
}

void ReaderBrowser::mouseMoveEvent(QMouseEvent* e)
{
    const int idx = tokenAt(e->pos());
    if (idx >= 0) emit tokenHovered(idx);
    else setToolTip(QString());
    QTextBrowser::mouseMoveEvent(e);
}

void ReaderBrowser::mousePressEvent(QMouseEvent* e)
{
    const int idx = tokenAt(e->pos());
    if (e->button() == Qt::LeftButton && idx >= 0) {
        emit tokenActivated(idx);
        e->accept();
        return;
    }
    QTextBrowser::mousePressEvent(e);
}

// ---------------- ReaderView ----------------
ReaderView::ReaderView(QWidget* parent) : QWidget(parent)
{
    browser_ = new ReaderBrowser(this);
    browser_->setStyleSheet("QTextBrowser{border:1px solid #d8dee9;border-radius:8px;padding:8px;"
                            "background:#ffffff;}");
    browser_->setHtml(QStringLiteral("<p style='color:#8a8f98'>请从「文章库」选择一篇文章开始阅读。</p>"));

    detail_ = new QTextBrowser(this);
    detail_->setOpenLinks(false);
    detail_->setStyleSheet("QTextBrowser{border:1px solid #d8dee9;border-radius:8px;padding:10px;"
                           "background:#f7f8fa;}");
    detail_->setHtml(QStringLiteral("<p style='color:#8a8f98;font-size:13px'>"
                                    "把鼠标移到字或词上可查看<b>五笔编码与拼音</b>提示；"
                                    "点击某个字/词，在右侧查看详细信息。</p>"));

    QSplitter* split = new QSplitter(Qt::Horizontal, this);
    split->addWidget(browser_);
    split->addWidget(detail_);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 0);
    split->setSizes({760, 340});

    legend_ = new QLabel(this);
    legend_->setTextFormat(Qt::RichText);
    legend_->setText(QStringLiteral("<span style='color:%1;font-weight:bold'>■</span> 单字 "
                                    "<span style='color:%2;font-weight:bold'>■</span> 二字词 "
                                    "<span style='color:%3;font-weight:bold'>■</span> 三字词 "
                                    "<span style='color:%4;font-weight:bold'>■</span> 四字以上词")
                      .arg(QLatin1String(kSingle), QLatin1String(kWord2),
                           QLatin1String(kWord3), QLatin1String(kWord4)));

    colorChk_ = new QCheckBox(QStringLiteral("词组分色"), this);
    colorChk_->setChecked(true);

    lookup_ = new QLineEdit(this);
    lookup_->setPlaceholderText(QStringLiteral("输入单个字或词语，回车查询五笔编码与拼音"));
    lookup_->setClearButtonEnabled(true);

    QHBoxLayout* top = new QHBoxLayout;
    top->addWidget(legend_);
    top->addStretch();
    top->addWidget(colorChk_);
    top->addWidget(lookup_, 1);

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(10, 10, 10, 10);
    lay->addLayout(top);
    lay->addWidget(split, 1);

    connect(browser_, &ReaderBrowser::tokenHovered, this, &ReaderView::onHovered);
    connect(browser_, &ReaderBrowser::tokenActivated, this, &ReaderView::onActivated);
    connect(lookup_, &QLineEdit::returnPressed, this, &ReaderView::onLookup);
    connect(colorChk_, &QCheckBox::toggled, this, &ReaderView::rebuild);
}

void ReaderView::setArticle(const QString& title, const QString& body)
{
    title_ = title;
    body_ = body;
    tokens_ = segment(body_, theDict());
    buildHtml();
}

void ReaderView::setHintsEnabled(bool on)
{
    hintsOn_ = on;
    const QString msg = on
        ? QStringLiteral("把鼠标移到字或词上可查看<b>五笔编码与拼音</b>提示；"
                         "点击某个字/词，在右侧查看详细信息。")
        : QStringLiteral("编码提示已关闭：悬停与点击不再显示五笔/拼音。"
                         "可在顶部勾选「显示编码提示」重新开启，或使用上方查询框主动查询。");
    detail_->setHtml(QStringLiteral("<p style='color:#8a8f98;font-size:13px'>%1</p>").arg(msg));
}

void ReaderView::rebuild()
{
    buildHtml();
}

void ReaderView::buildHtml()
{
    QString html;
    html.reserve(body_.size() * 3 + 512);
    html += QStringLiteral("<html><head><style>"
                           "body{font-family:'Microsoft YaHei','PingFang SC','Noto Sans CJK SC',"
                           "sans-serif;font-size:17px;line-height:2.0;color:#333a45;}"
                           "h1{font-size:24px;color:#0f172a;margin-bottom:14px;}"
                           "a{text-decoration:none;color:inherit;}"
                           "</style></head><body>");
    html += "<h1>" + esc(title_) + "</h1><p>";
    const bool color = colorChk_->isChecked();
    for (int i = 0; i < tokens_.size(); ++i) {
        const Token& t = tokens_[i];
        const QString cls = t.isWord ? (t.len >= 4 ? "w4" : t.len == 3 ? "w3" : "w2") : "w1";
        QString style;
        if (color) {
            if (t.len >= 2) {
                const char* c = t.len >= 4 ? kWord4 : t.len == 3 ? kWord3 : kWord2;
                style = QStringLiteral("color:%1;").arg(QLatin1String(c));
            }
        }
        const QString ttext = esc(body_.mid(t.start, t.len));
        html += QStringLiteral("<a href='tok:%1' style='%2'>%3</a>")
                    .arg(i).arg(style).arg(ttext);
    }
    html += "</p></body></html>";
    browser_->setHtml(html);
}

void ReaderView::onHovered(int idx)
{
    if (!hintsOn_) {           // 提示已关闭：不显示悬停提示
        browser_->setToolTip(QString());
        return;
    }
    if (idx < 0 || idx >= tokens_.size()) return;
    const Token& t = tokens_[idx];
    const QString text = body_.mid(t.start, t.len);
    QString tip;
    if (t.isWord) {
        const Dict& d = theDict();
        const QString code = d.wordCode(text);
        QString py;
        for (int k = 0; k < text.size(); ++k) {
            if (k) py += " ";
            py += d.pinyin(QString(text[k]));
        }
        tip = QStringLiteral("%1  词语编码：%2\n拼音：%3").arg(text, code.toUpper(), py);
    } else {
        const Dict& d = theDict();
        tip = QStringLiteral("%1  %2\n五笔：%3（全码）")
                  .arg(text, d.pinyin(text), d.wubiFull(text).toUpper());
        const QStringList b = d.wubiBriefs(text);
        if (!b.isEmpty()) tip += "\n简码：" + b.join(" / ").toUpper();
    }
    browser_->setToolTip(tip);
}

void ReaderView::onActivated(int idx)
{
    if (!hintsOn_) return;     // 提示已关闭：不响应点击详情
    if (idx < 0 || idx >= tokens_.size()) return;
    const Token& t = tokens_[idx];
    const QString text = body_.mid(t.start, t.len);
    detail_->setHtml(t.isWord ? wordDetailHtml(text) : charDetailHtml(text));
}

QString ReaderView::charDetailHtml(const QString& ch) const
{
    const Dict& d = theDict();
    const QString full = d.wubiFull(ch);
    const QStringList briefs = d.wubiBriefs(ch);
    QString html = QStringLiteral("<h3 style='margin:0 0 6px;font-size:20px'>%1</h3>").arg(esc(ch));
    html += QStringLiteral("<p style='font-size:14px'>拼音：<b>%1</b></p>")
                .arg(esc(d.pinyins(ch).join(" / ")));
    html += QStringLiteral("<p style='font-size:14px'>五笔全码：<b style='font-family:Consolas,monospace'>%1</b></p>")
                .arg(full.toUpper());
    if (!briefs.isEmpty()) {
        QStringList lbl;
        for (const QString& b : briefs)
            lbl << QStringLiteral("%1（%2级简码）")
                       .arg(b.toUpper(), b.size() == 1 ? "一" : b.size() == 2 ? "二" : "三");
        html += QStringLiteral("<p style='font-size:14px'>简码：%1</p>")
                    .arg(esc(lbl.join("，")));
    }
    if (!d.pinyins(ch).isEmpty())
        html += QStringLiteral("<p style='font-size:13px;color:#6b7280'>读音：%1</p>")
                    .arg(esc(d.pinyins(ch).join("，")));
    return html;
}

QString ReaderView::wordDetailHtml(const QString& word) const
{
    const Dict& d = theDict();
    const QString code = d.wordCode(word);
    QString rule;
    if (word.size() == 2) rule = "二字词：取第1字前2码 + 第2字前2码";
    else if (word.size() == 3) rule = "三字词：取第1字首码 + 第2字首码 + 第3字前2码";
    else rule = "四字及以上：取前3字首码 + 末字首码";

    QString html = QStringLiteral("<h3 style='margin:0 0 6px;font-size:20px'>%1</h3>").arg(esc(word));
    html += QStringLiteral("<p style='font-size:14px'>词语编码：<b style='font-family:Consolas,monospace'>%1</b></p>")
                .arg(code.toUpper());
    html += QStringLiteral("<p style='font-size:13px;color:#6b7280'>%1</p>").arg(rule);
    html += QStringLiteral("<table style='border-collapse:collapse;margin-top:8px;font-size:14px'>"
                           "<tr style='background:#eef2f7'><th style='padding:4px 10px;border:1px solid #d8dee9'>字</th>"
                           "<th style='padding:4px 10px;border:1px solid #d8dee9'>拼音</th>"
                           "<th style='padding:4px 10px;border:1px solid #d8dee9'>五笔</th></tr>");
    for (const QChar& c : word) {
        const QString s = QString(c);
        html += QStringLiteral("<tr><td style='padding:3px 10px;border:1px solid #d8dee9'>%1</td>"
                               "<td style='padding:3px 10px;border:1px solid #d8dee9'>%2</td>"
                               "<td style='padding:3px 10px;border:1px solid #d8dee9'>%3</td></tr>")
                    .arg(esc(s), esc(d.pinyin(s)), d.wubiFull(s).toUpper());
    }
    html += "</table>";
    return html;
}

QString ReaderView::lookupResultHtml(const QString& text) const
{
    if (text.isEmpty()) return QStringLiteral("<p style='color:#8a8f98'>请输入要查询的内容。</p>");
    if (text.size() == 1) return charDetailHtml(text);
    return wordDetailHtml(text);
}

void ReaderView::onLookup()
{
    const QString t = lookup_->text().trimmed();
    if (t.isEmpty()) return;
    detail_->setHtml(lookupResultHtml(t));
}
