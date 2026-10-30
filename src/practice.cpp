#include "practice.h"
#include "practice_internal.h"
#include "dataloader.h"
#include "bilingual.h"
#include "justifiedlabel.h"
#include "justifiedinput.h"
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QTimer>
#include <QEvent>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QResizeEvent>
#include <QStandardItemModel>

// 练习行标签（自绘、支持两端对齐与当前行分段着色）
static JustifiedLabel* makeRowLabel(const QString& color, int height)
{
    JustifiedLabel* l = new JustifiedLabel(color, height);
    l->setStyleSheet(QStringLiteral("font-size:20px;padding-left:18px;padding-right:18px;"));
    return l;
}

PracticeView::PracticeView(QWidget* parent) : QWidget(parent)
{
    articleLabel_ = new QLabel(QStringLiteral("尚未选择文章"), this);
    articleLabel_->setStyleSheet("font-size:15px;font-weight:bold;color:#0f172a;");

    mode_ = new QComboBox(this);
    mode_->addItem(QStringLiteral("单字练习"));
    mode_->addItem(QStringLiteral("词语练习"));
    mode_->setCurrentIndex(1);

    // 中英双语：练习语言切换
    langCmb_ = new QComboBox(this);
    langCmb_->addItem(QStringLiteral("中文练习"));
    langCmb_->addItem(QStringLiteral("英文练习"));
    langCmb_->setCurrentIndex(0);

    restartBtn_ = new QPushButton(QStringLiteral("重新开始"), this);
    skipBtn_    = new QPushButton(QStringLiteral("跳过"), this);

    QHBoxLayout* bar = new QHBoxLayout;
    bar->addWidget(articleLabel_, 1);
    bar->addWidget(langCmb_);
    bar->addWidget(mode_);
    bar->addWidget(restartBtn_);
    bar->addWidget(skipBtn_);

    // 界面上方：当前词注音 + 五笔编码提示（英文模式为单词中英双解）
    hintLabel_ = new QLabel(this);
    hintLabel_->setAlignment(Qt::AlignCenter);
    hintLabel_->setTextFormat(Qt::RichText);
    hintLabel_->setStyleSheet("font-size:18px;color:#334155;");
    hintLabel_->setFixedHeight(30);

    // 统计信息
    statsLabel_ = new QLabel(this);
    statsLabel_->setStyleSheet("color:#475569;font-size:13px;");
    statsLabel_->setFixedHeight(22);

    // 已完成行（原文+输入成对，最多展示最近 2 行）
    done2Line_ = makeRowLabel("#94a3b8", 24);
    done2In_   = makeRowLabel("#a5b4fc", 24);
    done1Line_ = makeRowLabel("#94a3b8", 24);
    done1In_   = makeRowLabel("#a5b4fc", 24);

    // “当前行”卡片：原文在上、输入框在下
    lineCard_ = new QFrame(this);
    lineCard_->setObjectName("lineCard");
    lineCard_->setStyleSheet(
        "#lineCard{background:#ffffff;border:1px solid #e2e8f0;border-radius:14px;}");

    lineLabel_ = new JustifiedLabel("#0f172a", 34, this);
    lineLabel_->setStyleSheet("font-size:20px;");

    input_ = new JustifiedInputLine(this);
    input_->setPlaceholderText(QStringLiteral("用系统输入法输入，与上方原文逐字匹配"));
    input_->setStyleSheet(
        "QLineEdit{background:transparent;border:none;"
        "font-size:20px;color:#1e40af;padding:6px 2px 4px 2px;}");
    // 明确允许 IME / 任意字符（中英、标点、符号），勿设 validator / 输入掩码
    input_->setAttribute(Qt::WA_InputMethodEnabled, true);
    input_->setInputMethodHints(Qt::ImhNone);
    input_->setMaxLength(32767);
    input_->setFrame(false);
    input_->installEventFilter(this);

    QVBoxLayout* cardLay = new QVBoxLayout(lineCard_);
    cardLay->setContentsMargins(18, 12, 18, 14);
    cardLay->setSpacing(8);
    cardLay->addWidget(lineLabel_);
    cardLay->addWidget(input_);

    // 后续行
    up1_ = makeRowLabel("#64748b", 24);
    up2_ = makeRowLabel("#64748b", 24);
    up3_ = makeRowLabel("#64748b", 24);
    up4_ = makeRowLabel("#64748b", 24);
    up5_ = makeRowLabel("#64748b", 24);

    // 左侧练习列
    QVBoxLayout* left = new QVBoxLayout;
    left->setSpacing(4);
    left->addLayout(bar);
    left->addWidget(hintLabel_);            // 注音+五笔提示在界面上方
    left->addWidget(statsLabel_);
    left->addWidget(done2Line_);
    left->addWidget(done2In_);
    left->addWidget(done1Line_);
    left->addWidget(done1In_);
    left->addWidget(lineCard_);
    left->addWidget(up1_);
    left->addWidget(up2_);
    left->addWidget(up3_);
    left->addWidget(up4_);
    left->addWidget(up5_);
    left->addStretch(1);

    // 右侧句子对照面板
    rightCard_ = new QFrame(this);
    rightCard_->setObjectName("rightCard");
    rightCard_->setStyleSheet(
        "#rightCard{background:#f8fafc;border:1px solid #e2e8f0;border-radius:12px;}");
    rightCard_->setFixedWidth(330);
    rightTitle_ = new QLabel(QStringLiteral("句子对照"), rightCard_);
    rightTitle_->setStyleSheet("font-size:15px;font-weight:bold;color:#334155;");
    rightTitle_->setWordWrap(true);
    rightLabel_ = new QLabel(rightCard_);
    rightLabel_->setTextFormat(Qt::RichText);
    rightLabel_->setWordWrap(true);
    rightLabel_->setStyleSheet("font-size:16px;color:#475569;");
    QVBoxLayout* rl = new QVBoxLayout(rightCard_);
    rl->setContentsMargins(14, 12, 14, 12);
    rl->setSpacing(8);
    rl->addWidget(rightTitle_);
    rl->addWidget(rightLabel_, 1);

    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);
    root->addLayout(left, 1);
    root->addWidget(rightCard_);

    idleTimer_ = new QTimer(this);
    idleTimer_->setInterval(1000);
    connect(idleTimer_, &QTimer::timeout, this, &PracticeView::onTick);

    connect(input_, &QLineEdit::textEdited, this, &PracticeView::onTextEdited);
    connect(restartBtn_, &QPushButton::clicked, this, &PracticeView::onRestart);
    connect(skipBtn_, &QPushButton::clicked, this, &PracticeView::onSkip);
    connect(mode_, &QComboBox::currentIndexChanged, this, &PracticeView::onModeChanged);
    connect(langCmb_, &QComboBox::currentIndexChanged, this, &PracticeView::onLangChanged);

    prepare();
}

bool PracticeView::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj != input_)
        return QWidget::eventFilter(obj, ev);

    switch (ev->type()) {
    case QEvent::KeyPress: {
        noteActivity();
        QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
        // Escape / 切换输入法时常残留组合态，主动清除以免挡住后续匹配
        if (ke->key() == Qt::Key_Escape)
            compositionActive_ = false;
        // Backspace 且输入框为空：手动回退到上一行末尾（跨行使删除）
        if (ke->key() == Qt::Key_Backspace && input_->text().isEmpty() && lineIdx_ > 0) {
            --lineIdx_;
            typedLen_ = lines_[lineIdx_].size();
            correct_ = lineStarts_[lineIdx_] + typedLen_;
            suppressEdit_ = true;
            input_->setText(lines_[lineIdx_]);
            suppressEdit_ = false;
            refresh();
            return true;  // 事件已处理
        }
        break;
    }
    case QEvent::InputMethod: {
        noteActivity();
        QInputMethodEvent* ime = static_cast<QInputMethodEvent*>(ev);
        // 有提交串 ⇒ 组合结束。必须在 QLineEdit 吃完本事件后再匹配：
        // 部分 IME 会先改 text / 发 textEdited，后清 preedit；若在 textEdited 里
        // 因 compositionActive_ 直接 return，就会出现“打了字却不前进/像卡死”。
        if (!ime->commitString().isEmpty()) {
            compositionActive_ = false;
            scheduleProcessText();
        } else {
            compositionActive_ = !ime->preeditString().isEmpty();
        }
        break;
    }
    case QEvent::FocusIn:
        compositionActive_ = false;
        noteActivity();
        break;
    case QEvent::FocusOut:
        // 失焦时结束组合态，避免回来后 textEdited 仍被旧门闩挡住
        compositionActive_ = false;
        break;
    case QEvent::MouseButtonPress:
        noteActivity();
        if (!input_->hasFocus())
            input_->setFocus(Qt::MouseFocusReason);
        break;
    default:
        break;
    }
    return QWidget::eventFilter(obj, ev);
}

void PracticeView::setArticle(const QString& title, const QString& body)
{
    title_ = title;
    zhBody_ = body;
    tokens_ = segment(body, theDict());

    // 双语数据：英文译文全文 + 逐句对照（按标题匹配）
    pairs_ = theBilingual().sentences(title);
    enText_.clear();
    enStarts_.clear();
    if (!pairs_.isEmpty()) {
        for (const SentencePair& p : pairs_) {
            enStarts_.append(enText_.size());
            if (!enText_.isEmpty())
                enText_ += QLatin1Char(' ');
            enText_ += p.en;
        }
    }
    // 无译文的文章（如用户导入）禁用“英文练习”选项
    const bool hasEn = !pairs_.isEmpty();
    if (QStandardItemModel* m = qobject_cast<QStandardItemModel*>(langCmb_->model())) {
        if (QStandardItem* it = m->item(1))
            it->setEnabled(hasEn);
    }
    if (!hasEn && langMode_ == 1) {
        langCmb_->setCurrentIndex(0);
        langMode_ = 0;
    }
    body_ = (langMode_ == 1) ? enText_ : zhBody_;
    if (langMode_ == 0)
        tokens_ = segment(body_, theDict());

    articleLabel_->setText(QStringLiteral("《%1》").arg(title));
    prepare();
}

void PracticeView::setHintsEnabled(bool on)
{
    hintsOn_ = on;
    if (!on) hintLabel_->setText(QString());
    refresh();
}

void PracticeView::setLangMode(int mode)
{
    langCmb_->setCurrentIndex(mode);
    onLangChanged();
}

void PracticeView::onModeChanged()
{
    wordMode_ = (mode_->currentIndex() == 1);
    prepare();
}

void PracticeView::onLangChanged()
{
    const int m = langCmb_->currentIndex();
    if (m == langMode_) return;
    if (m == 1 && enText_.isEmpty()) {   // 无译文时禁止切换（下拉项已禁用，双保险）
        langCmb_->setCurrentIndex(0);
        return;
    }
    langMode_ = m;
    mode_->setEnabled(m == 0);           // 单字/词语仅中文模式有效
    if (langMode_ == 1) {
        body_ = enText_;
        tokens_.clear();
        input_->setPlaceholderText(QStringLiteral("用键盘直接输入英文译文，与上方英文逐词匹配"));
    } else {
        body_ = zhBody_;
        tokens_ = segment(body_, theDict());
        input_->setPlaceholderText(QStringLiteral("用系统输入法输入，与上方原文逐字匹配"));
    }
    prepare();
}

void PracticeView::onRestart()
{
    prepare();
}

void PracticeView::onSkip()
{
    noteActivity();
    ++skip_;
    advanceLine();
}

void PracticeView::prepare()
{
    lineIdx_ = 0;
    typedLen_ = 0;
    correct_ = error_ = skip_ = 0;
    finished_ = false;
    paused_ = false;
    compositionActive_ = false;
    processQueued_ = false;
    lastInputIndent_ = -1;
    lastRenderLine_ = -1;
    buildLines();
    lastBuildWidth_ = lineCard_->width();
    clock_.start();
    lastActivityMs_ = 0;
    activeMs_ = 0;
    samples_.clear();
    suppressEdit_ = true;
    input_->clear();
    suppressEdit_ = false;
    input_->setEnabled(true);
    input_->setFocus(Qt::OtherFocusReason);
    idleTimer_->start();
    refresh();
}

void PracticeView::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    // 输入法预编辑中不要重排，否则会打断组合并表现为“突然不能输入”
    if (compositionActive_)
        return;
    const int w = lineCard_->width();
    if (w <= 50 || qAbs(w - lastBuildWidth_) <= 8)
        return;
    const int abs = absPos();
    const bool wasFinished = finished_;
    buildLines();
    lastBuildWidth_ = w;
    lastRenderLine_ = -1;   // 折行变了，强制整页重绘
    lastInputIndent_ = -1;
    if (wasFinished || lines_.isEmpty()) {
        refresh();
        return;
    }
    // 按绝对位置恢复到对应行，保留进度
    lineIdx_ = 0;
    typedLen_ = 0;
    for (int i = 0; i < lines_.size(); ++i) {
        if (abs >= lineStarts_[i]) {
            lineIdx_ = i;
            typedLen_ = qMin(lines_[i].size(), abs - lineStarts_[i]);
        } else {
            break;
        }
    }
    if (typedLen_ >= lines_[lineIdx_].size()) {
        ++lineIdx_;
        typedLen_ = 0;
        if (lineIdx_ >= lines_.size()) {
            finished_ = true;
            input_->setEnabled(false);
        }
    }
    suppressEdit_ = true;
    input_->setText(typedLen_ > 0 ? lines_[lineIdx_].left(typedLen_) : QString());
    suppressEdit_ = false;
    refresh();
}

// 计时与暂停：任何输入活动更新活动时间；连续 kPauseMs 无输入置暂停
void PracticeView::noteActivity()
{
    const qint64 now = clock_.elapsed();
    if (paused_) {
        // 从暂停恢复：跳过整个空闲段，不计入活动时间
        paused_ = false;
        lastActivityMs_ = now;
    } else {
        activeMs_ += now - lastActivityMs_;
        lastActivityMs_ = now;
    }
}

// 1 秒心跳：暂停检测 + 速度采样
void PracticeView::onTick()
{
    const qint64 now = clock_.elapsed();
    if (!paused_ && !finished_ && now - lastActivityMs_ > kPauseMs)
        paused_ = true;                      // 连续 5 秒无输入 → 暂停
    samples_.append({now, activeMs_, correct_});
    // 只保留速度窗口内样本，避免长时间练习 samples_ 无限膨胀拖慢刷新
    const qint64 cut = now - kWindowMs - 2000;
    int drop = 0;
    while (drop < samples_.size() && samples_[drop].wall < cut)
        ++drop;
    if (drop > 0)
        samples_.remove(0, drop);
    updateStats();
}
