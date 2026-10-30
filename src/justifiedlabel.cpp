#include "justifiedlabel.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QVector>
#include <QPair>

JustifiedLabel::JustifiedLabel(const QString& color, int height, QWidget* parent)
    : QLabel(parent), color_(color)
{
    setFixedHeight(height);
    setTextFormat(Qt::PlainText);
}

void JustifiedLabel::setTarget(int start, int len)
{
    targetStart_ = start;
    targetLen_ = len;
    update();
}

void JustifiedLabel::setLastRow(bool last)
{
    if (lastRow_ != last) {
        lastRow_ = last;
        update();
    }
}

void JustifiedLabel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setFont(font());
    const QString text = this->text();
    if (text.isEmpty()) return;
    const QRect r = contentsRect();
    if (r.width() <= 0) return;
    QFontMetrics fm(font());

    // 按空格收集词边界（保留行内原始偏移）
    QVector<QPair<int, int>> words;
    int i = 0;
    const int n = text.size();
    while (i < n) {
        while (i < n && text.at(i) == QLatin1Char(' ')) ++i;
        if (i >= n) break;
        const int s = i;
        while (i < n && text.at(i) != QLatin1Char(' ')) ++i;
        words.append(qMakePair(s, i));
    }
    if (words.isEmpty()) return;

    const int baseline = r.top() + (r.height() - fm.height()) / 2 + fm.ascent();
    const int baseSpace = fm.horizontalAdvance(QLatin1Char(' '));
    int wordSum = 0;
    for (const auto& w : words)
        wordSum += fm.horizontalAdvance(text.mid(w.first, w.second - w.first));
    const int gaps = words.size() - 1;
    const int extra = r.width() - wordSum - gaps * baseSpace;
    const bool justify = !lastRow_ && words.size() > 1 && extra > 0;

    int x = r.left();
    if (justify) {
        // 两端对齐：多余宽度均匀分配到词间空格（余数逐格 +1px）
        const int addBase = extra / gaps;
        const int addRem = extra % gaps;
        for (int k = 0; k < words.size(); ++k) {
            const int ww = fm.horizontalAdvance(
                text.mid(words[k].first, words[k].second - words[k].first));
            drawWord(p, r, baseline, x, text, words[k].first, words[k].second);
            if (k < gaps) x += ww + baseSpace + addBase + (k < addRem ? 1 : 0);
        }
    } else {
        // 左对齐：词间单空格
        for (int k = 0; k < words.size(); ++k) {
            const int ww = fm.horizontalAdvance(
                text.mid(words[k].first, words[k].second - words[k].first));
            drawWord(p, r, baseline, x, text, words[k].first, words[k].second);
            if (k < gaps) x += ww + baseSpace;
        }
    }
}

void JustifiedLabel::drawWord(QPainter& p, const QRect& r, int baseline, int x,
                              const QString& text, int ws, int we)
{
    if (ws >= we) return;
    QFontMetrics fm(font());
    struct Seg { int from, to; QColor color; bool bold; bool bg; };
    QVector<Seg> segs;
    if (targetStart_ < 0) {
        segs.append({ ws, we, QColor(color_), false, false });
    } else {
        int cur = ws;
        if (cur < targetStart_) {
            const int e = qMin(we, targetStart_);
            segs.append({ cur, e, QColor("#2563eb"), false, false });
            cur = e;
        }
        if (cur < targetStart_ + targetLen_) {
            const int e = qMin(we, targetStart_ + targetLen_);
            segs.append({ cur, e, QColor("#0f172a"), true, true });
            cur = e;
        }
        if (cur < we)
            segs.append({ cur, we, QColor("#0f172a"), false, false });
    }
    QFont boldF = font();
    boldF.setBold(true);
    int sx = x;
    for (const Seg& s : segs) {
        if (s.from >= s.to) continue;
        const QString part = text.mid(s.from, s.to - s.from);
        const int w = fm.horizontalAdvance(part);
        if (s.bg)
            p.fillRect(QRect(sx, r.top(), w, r.height()), QColor("#fde68a"));
        p.setPen(s.color);
        p.setFont(s.bold ? boldF : font());
        p.drawText(sx, baseline, part);
        sx += w;
    }
    p.setFont(font());
}
