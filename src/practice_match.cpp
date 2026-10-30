#include "practice.h"
#include "dataloader.h"
#include "bilingual.h"
#include "justifiedinput.h"
#include <QLineEdit>
#include <QInputMethodEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QTimer>

// ============================================================================
// 匹配核心与目标查询：
// 输入框内容与“当前行行首到文末的连续正文”做前缀匹配（0.1.9 红线），
// 支持跨行词组整词输入与单行内逐字输入；同时提供当前目标
// （字/词/英文单词）的文本、编码、拼音、释义与句子定位。
// ============================================================================

int PracticeView::absPos() const
{
    return lineStarts_[lineIdx_] + typedLen_;
}

int PracticeView::targetStartAbs() const
{
    if (langMode_ == 1) {              // 英文：当前输入位置所在的单词
        const int p = absPos();
        int b = p;
        while (b > 0 && !body_.at(b - 1).isSpace()) --b;
        return b;
    }
    const int p = absPos();
    if (wordMode_) {
        for (const Token& t : tokens_) {
            if (p >= t.start && p < t.start + t.len)
                return t.start;
        }
    }
    return p;
}

QString PracticeView::currentEnWord() const
{
    if (finished_ || lines_.isEmpty() || langMode_ != 1) return QString();
    const int p = absPos();
    if (p >= body_.size()) return QString();
    int b = p;
    while (b > 0 && !body_.at(b - 1).isSpace()) --b;
    int e = p;
    while (e < body_.size() && !body_.at(e).isSpace()) ++e;
    return body_.mid(b, e - b);
}

QString PracticeView::targetText() const
{
    if (finished_ || lineIdx_ >= lines_.size()) return QString();
    if (langMode_ == 1)
        return currentEnWord();
    const int p = typedLen_;
    const QString& line = lines_[lineIdx_];
    if (p >= line.size()) return QString();
    if (wordMode_) {
        const int abs = lineStarts_[lineIdx_] + p;
        for (const Token& t : tokens_) {
            if (abs >= t.start && abs < t.start + t.len) {
                if (t.isWord) return body_.mid(t.start, t.len);
                return body_.mid(abs, 1);
            }
        }
        return body_.mid(abs, 1);
    }
    return body_.mid(lineStarts_[lineIdx_] + p, 1);
}

int PracticeView::targetStartInLine() const
{
    if (finished_) return 0;
    return qMax(0, targetStartAbs() - lineStarts_[lineIdx_]);
}

int PracticeView::targetLenInLine() const
{
    if (finished_) return 0;
    const QString t = targetText();
    if (t.isEmpty()) return 0;
    // 目标词可能跨行：只高亮“目标区间 ∩ 当前行 [已输入位置, 行尾)”部分
    const int lineStart = lineStarts_[lineIdx_];
    const int lineEnd = lineStart + lines_[lineIdx_].size();
    const int hiStart = qMax(targetStartAbs(), lineStart + typedLen_);
    const int hiEnd = qMin(targetStartAbs() + t.size(), lineEnd);
    return qMax(0, hiEnd - hiStart);
}

QString PracticeView::targetCode() const
{
    if (langMode_ == 1) return QString();   // 英文练习无五笔编码
    const QString t = targetText();
    if (t.isEmpty()) return QString();
    if (wordMode_ && t.size() > 1)
        return theDict().wordCode(t);
    return theDict().wubiFull(t);
}

QString PracticeView::targetPinyin() const
{
    if (langMode_ == 1) return QString();   // 英文练习无拼音
    const QString t = targetText();
    if (t.isEmpty()) return QString();
    const Dict& d = theDict();
    QStringList parts;
    for (const QChar& c : t) {
        const QString p = d.pinyin(QString(c));
        if (!p.isEmpty()) parts << p;
    }
    return parts.join(" ");
}

QString PracticeView::targetMeaning() const
{
    return theBilingual().meaning(currentEnWord());
}

int PracticeView::currentZhSentence() const
{
    if (langMode_ != 0 || pairs_.isEmpty()) return -1;
    const int p = absPos();
    int best = -1;
    for (int i = 0; i < pairs_.size(); ++i) {
        if (pairs_[i].start <= p)
            best = i;
        else
            break;
    }
    return best;
}

int PracticeView::currentEnSentence() const
{
    if (langMode_ != 1 || enStarts_.isEmpty()) return -1;
    const int p = absPos();
    int best = -1;
    for (int i = 0; i < enStarts_.size(); ++i) {
        if (enStarts_[i] <= p)
            best = i;
        else
            break;
    }
    return best;
}

void PracticeView::advanceLine()
{
    ++lineIdx_;
    typedLen_ = 0;
    suppressEdit_ = true;
    input_->clear();
    suppressEdit_ = false;
    if (lineIdx_ >= lines_.size()) {
        finished_ = true;
        idleTimer_->stop();
        input_->setEnabled(false);
        samples_.append({clock_.elapsed(), activeMs_, correct_});  // 记录最终样本
    }
    refresh();
}

// 延迟到当前事件（IME 提交 / 按键）处理完毕后再匹配，避免组合态竞态
void PracticeView::scheduleProcessText()
{
    if (processQueued_ || finished_) return;
    processQueued_ = true;
    QTimer::singleShot(0, this, [this]() {
        processQueued_ = false;
        if (suppressEdit_ || finished_) return;
        processText();
    });
}

// 输入框内容变化（输入法提交或直接键入）：与正文连续匹配
void PracticeView::onTextEdited()
{
    if (suppressEdit_) return;        // 程序性改写输入框，不重入
    noteActivity();
    // 文本已变 ⇒ 视为提交结果。绝不能因 compositionActive_ 丢弃：
    // 若干 IME（ibus/fcitx/搜狗等）会先更新 text 再发清空 preedit 的事件。
    compositionActive_ = false;
    // textEdited 触发时 text() 已是最新，同步匹配即可（勿再被组合态门闩挡住）
    processText();
}

void PracticeView::processText()
{
    if (finished_) return;
    if (lines_.isEmpty() || lineIdx_ < 0 || lineIdx_ >= lines_.size()) return;
    const QString box = input_->text();
    const int boxLen = box.size();

    // 输入框为空：回退到上一行末尾（如果当前行已删完）
    if (boxLen == 0) {
        if (typedLen_ == 0 && lineIdx_ > 0) {
            // 回到上一行末尾
            --lineIdx_;
            typedLen_ = lines_[lineIdx_].size();
            suppressEdit_ = true;
            input_->setText(lines_[lineIdx_]);
            suppressEdit_ = false;
        } else {
            typedLen_ = 0;
        }
        correct_ = lineStarts_[lineIdx_] + typedLen_;
        error_ = 0;
        refresh();
        return;
    }

    // 跨行连续匹配：输入框语义为"从当前行行首开始累积的输入"（box 含已匹配前缀），
    // 与"当前行行首到文末的连续正文"做前缀匹配。
    // 英文折行会丢掉行尾空格，正文里行间仍有空格——匹配时自动跳过这些
    // “视觉行间隙空格”，避免必须多打一个看不见的空格才能进下一行。
    const int lineStart = lineStarts_[lineIdx_];
    const int bodyLen = body_.size();
    int bi = 0;   // 相对 lineStart 的正文偏移
    int qi = 0;   // 输入框已匹配长度
    while (qi < boxLen && lineStart + bi < bodyLen) {
        const QChar bc = body_.at(lineStart + bi);
        const QChar qc = box.at(qi);
        if (qc == bc) {
            ++qi;
            ++bi;
            continue;
        }
        if (bc.isSpace()) {
            // 是否正好落在某条视觉行的末尾（行文本之后、下一行 start 之前的间隙）
            bool atVisualLineEnd = false;
            for (int li = lineIdx_; li < lines_.size(); ++li) {
                const int visEnd = lineStarts_[li] + lines_[li].size();
                const int abs = lineStart + bi;
                if (abs == visEnd) {
                    atVisualLineEnd = true;
                    break;
                }
                if (lineStarts_[li] > abs)
                    break;
            }
            if (atVisualLineEnd) {
                ++bi;       // 跳过正文空格，不消耗输入
                continue;
            }
        }
        break;
    }

    // 定位匹配终点所在行
    int newAbs = lineStart + bi;
    // 若正好停在行末且后面只剩间隙空格，吞掉空格以便立刻进入下一行
    while (newAbs < bodyLen && body_.at(newAbs).isSpace()) {
        bool gap = false;
        for (int li = lineIdx_; li < lines_.size(); ++li) {
            if (lineStarts_[li] + lines_[li].size() == newAbs) {
                gap = true;
                break;
            }
            if (lineStarts_[li] > newAbs) break;
        }
        if (!gap) break;
        ++newAbs;
    }

    int newLineIdx = lineIdx_;
    for (int i = lineIdx_; i < lines_.size(); ++i) {
        if (lineStarts_[i] <= newAbs)
            newLineIdx = i;
        else
            break;
    }
    // 若 newAbs 已到达下一行起点
    if (newLineIdx + 1 < lines_.size() && newAbs >= lineStarts_[newLineIdx + 1])
        newLineIdx = newLineIdx; // already updated by loop
    int newTypedLen = newAbs - lineStarts_[newLineIdx];
    if (newTypedLen < 0) newTypedLen = 0;
    if (newTypedLen > lines_[newLineIdx].size())
        newTypedLen = lines_[newLineIdx].size();

    // 全文打完
    if (newAbs >= bodyLen) {
        finished_ = true;
        lineIdx_ = lines_.size() - 1;
        typedLen_ = lines_.last().size();
        correct_ = bodyLen;
        error_ = 0;
        suppressEdit_ = true;
        input_->setEnabled(false);
        input_->setText(QString());
        suppressEdit_ = false;
        refresh();
        return;
    }

    // 行切换时同步输入框（避免输入框残留旧行文本）
    if (newLineIdx != lineIdx_) {
        lineIdx_ = newLineIdx;
        typedLen_ = newTypedLen;
        const QString want = typedLen_ > 0 ? lines_[lineIdx_].left(typedLen_) : QString();
        suppressEdit_ = true;
        input_->setText(want);
        suppressEdit_ = false;
    } else {
        // 同一行：保留错误字符显示，不强制清除
        typedLen_ = newTypedLen;
    }

    correct_ = lineStarts_[lineIdx_] + typedLen_;
    // 错误字符数 = 输入框未匹配上的尾部
    error_ = qMax(0, boxLen - qi);

    refresh();
}
