#include "justifiedinput.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QInputMethodEvent>
#include <QFocusEvent>
#include <QSizePolicy>
#include <QVector>
#include <QPair>

JustifiedInputLine::JustifiedInputLine(QWidget* parent) : QLineEdit(parent)
{
    // 不按已输入全文计算最小宽，避免练习中途窗口无法横向缩小
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumWidth(0);
}

void JustifiedInputLine::setLineLayout(const QString& fullLine, int typedLen, bool justified)
{
    fullLine_ = fullLine;
    typedLen_ = typedLen;
    justified_ = justified;
    update();
}

void JustifiedInputLine::inputMethodEvent(QInputMethodEvent* e)
{
    // 记录预编辑串（汉字上屏前的拼音/五笔等编码字母），再交给基类处理提交
    preedit_ = e->preeditString();
    QLineEdit::inputMethodEvent(e);
    if (e->preeditString().isEmpty())
        preedit_.clear();
    update();
}

void JustifiedInputLine::focusOutEvent(QFocusEvent* e)
{
    preedit_.clear();
    QLineEdit::focusOutEvent(e);
    update();
}

QVariant JustifiedInputLine::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (query == Qt::ImCursorRectangle) {
        // 候选窗跟自绘光标对齐（含预编辑串宽度）
        QFontMetrics fm(font());
        const int h = contentsRect().height();
        return QRect(lastCursorX_, contentsRect().top(), 1, h > 0 ? h : fm.height());
    }
    return QLineEdit::inputMethodQuery(query);
}

void JustifiedInputLine::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setFont(font());
    const QRect r = contentsRect();
    if (r.width() <= 0) return;
    QFontMetrics fm(font());
    const QString userText = text();
    const int baseline = r.top() + (r.height() - fm.height()) / 2 + fm.ascent();

    // 绘制底部边框线（与样式表一致）
    p.setPen(QColor(hasFocus() ? "#2563eb" : "#dbeafe"));
    p.drawLine(r.left(), rect().bottom() - 1, rect().right(), rect().bottom() - 1);

    auto drawCursor = [&](int cx) {
        lastCursorX_ = cx;
        if (!hasFocus()) return;
        p.setPen(QPen(QColor("#2563eb"), 2));
        p.drawLine(cx, r.top(), cx, r.bottom());
    };

    // 在光标处绘制 IME 预编辑编码（灰色 + 下划线），上屏前必须可见
    auto drawPreedit = [&](int& cursorX) {
        if (preedit_.isEmpty()) return;
        const int w = fm.horizontalAdvance(preedit_);
        p.setPen(QColor("#64748b"));
        p.drawText(cursorX, baseline, preedit_);
        p.setPen(QPen(QColor("#94a3b8"), 1));
        const int ulY = baseline + 2;
        p.drawLine(cursorX, ulY, cursorX + w, ulY);
        cursorX += w;
    };

    // 尚无布局原文，或尚未正确匹配任何字符
    if (fullLine_.isEmpty() || typedLen_ <= 0) {
        int cursorX = r.left() + leftPad_;
        if (!userText.isEmpty()) {
            p.setPen(QColor("#dc2626"));
            p.drawText(cursorX, baseline, userText);
            cursorX += fm.horizontalAdvance(userText);
        } else if (!hasFocus() && preedit_.isEmpty() && !placeholderText().isEmpty()) {
            p.setPen(QColor("#94a3b8"));
            p.drawText(r.left(), baseline, placeholderText());
            lastCursorX_ = r.left() + leftPad_;
            return;
        }
        drawPreedit(cursorX);
        drawCursor(cursorX);
        return;
    }

    // 按空格收集词边界（与 JustifiedLabel 完全相同）
    QVector<QPair<int, int>> words;
    int i = 0;
    const int n = fullLine_.size();
    while (i < n) {
        while (i < n && fullLine_.at(i) == QLatin1Char(' ')) ++i;
        if (i >= n) break;
        const int s = i;
        while (i < n && fullLine_.at(i) != QLatin1Char(' ')) ++i;
        words.append(qMakePair(s, i));
    }
    if (words.isEmpty()) {
        int cursorX = r.left() + leftPad_;
        if (typedLen_ > 0) {
            p.setPen(QColor("#1e40af"));
            const QString ok = fullLine_.left(qMin(typedLen_, fullLine_.size()));
            p.drawText(cursorX, baseline, ok);
            cursorX += fm.horizontalAdvance(ok);
        }
        if (userText.size() > typedLen_) {
            p.setPen(QColor("#dc2626"));
            const QString wrong = userText.mid(typedLen_);
            p.drawText(cursorX, baseline, wrong);
            cursorX += fm.horizontalAdvance(wrong);
        }
        drawPreedit(cursorX);
        drawCursor(cursorX);
        return;
    }

    const int baseSpace = fm.horizontalAdvance(QLatin1Char(' '));
    // 与 JustifiedLabel 一致：右缘内缩，末字符（;,. 等）不被裁切
    constexpr int kEdgeInset = 2;
    int wordSum = 0;
    for (const auto& w : words)
        wordSum += fm.horizontalAdvance(fullLine_.mid(w.first, w.second - w.first));
    const int gaps = words.size() - 1;
    const int extra = r.width() - leftPad_ - kEdgeInset - wordSum - gaps * baseSpace;
    const bool just = justified_ && words.size() > 1 && extra > 0;

    QVector<int> wordX(words.size());
    int x = r.left() + leftPad_;
    const int addBase = just ? extra / gaps : 0;
    const int addRem = just ? extra % gaps : 0;
    for (int k = 0; k < words.size(); ++k) {
        wordX[k] = x;
        const int ww = fm.horizontalAdvance(fullLine_.mid(words[k].first, words[k].second - words[k].first));
        if (k < gaps)
            x += ww + baseSpace + addBase + (k < addRem ? 1 : 0);
    }

    p.setPen(QColor("#1e40af"));
    int cursorX = r.left() + leftPad_;

    for (int k = 0; k < words.size(); ++k) {
        const int ws = words[k].first;
        const int we = words[k].second;

        const int drawStart = qMax(ws, 0);
        const int drawEnd = qMin(we, typedLen_);
        if (drawStart < drawEnd) {
            const int offsetW = fm.horizontalAdvance(fullLine_.mid(ws, drawStart - ws));
            const QString part = fullLine_.mid(drawStart, drawEnd - drawStart);
            p.drawText(wordX[k] + offsetW, baseline, part);
        }

        if (typedLen_ >= ws && typedLen_ <= we) {
            cursorX = wordX[k] + fm.horizontalAdvance(fullLine_.mid(ws, typedLen_ - ws));
        } else if (typedLen_ == we && k < gaps) {
            const int ww = fm.horizontalAdvance(fullLine_.mid(ws, we - ws));
            cursorX = wordX[k] + ww + baseSpace + addBase + (k < addRem ? 1 : 0);
        }
    }

    if (userText.size() > typedLen_) {
        const QString wrongPart = userText.mid(typedLen_);
        p.setPen(QColor("#dc2626"));
        p.drawText(cursorX, baseline, wrongPart);
        cursorX += fm.horizontalAdvance(wrongPart);
    }

    drawPreedit(cursorX);
    drawCursor(cursorX);
}
