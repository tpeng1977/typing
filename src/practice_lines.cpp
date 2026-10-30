#include "practice.h"
#include "practice_internal.h"
#include "justifiedlabel.h"
#include <QFont>
#include <QFontMetrics>
#include <QFrame>

// ============================================================================
// 折行：把当前练习正文按可用宽度切成整齐的“练习行”。
// 中文按字符逐字折行（CJK 全角字天然齐边），英文按词折行（不在词中间断行）。
// 每段首行缩进两字符（缩进通过标签 padding 实现，不进入匹配文本，
// 且该行按“可用宽度-缩进”折行，保证右边缘与其他行对齐）。
// ============================================================================

// 文本像素宽：取 advance 与 boundingRect 较大值，避免 ;,. 等 right-bearing 被低估
static int textPixWidth(const QFontMetrics& fm, const QString& s)
{
    if (s.isEmpty()) return 0;
    return qMax(fm.horizontalAdvance(s), fm.boundingRect(s).width());
}

static int charPixWidth(const QFontMetrics& fm, QChar c)
{
    return textPixWidth(fm, QString(c));
}

// Word 文档式换行（中文）：整段按可用宽度自然折行，行满宽度、左右对齐，
// 仅段落最后一行允许不齐。
void PracticeView::buildLines()
{
    lines_.clear();
    lineStarts_.clear();
    indents_.clear();

    if (langMode_ == 1) {          // 英文按“词”折行，不在单词中间断行
        wrapEnglish();
        return;
    }

    QFont f = lineLabel_->font();
    f.setPixelSize(20);
    QFontMetrics fm(f);
    const int avail = textAvailWidth();
    const int indentW = 2 * charWidth();

    // 折行：从 begin 到 end（不含）按可用宽度切成若干行
    auto wrapPara = [&](int begin, int end) {
        int i = begin;
        bool firstLine = true;
        while (i < end) {
            const int start = i;
            const int limit = firstLine ? qMax(1, avail - indentW) : avail;
            int w = 0;
            while (i < end) {
                const QChar c = body_.at(i);
                const int cw = charPixWidth(fm, c);
                if (w + cw > limit && i > start) break;
                w += cw;
                ++i;
            }
            if (i == start) ++i;   // 极端窄窗：单字也超宽时至少放一个
            lines_.append(body_.mid(start, i - start));
            lineStarts_.append(start);
            indents_.append(firstLine);
            firstLine = false;
        }
    };

    // 按段落处理：以换行符分隔（当前内置文章为单段，兼容未来多段）
    const int n = body_.size();
    int paraBegin = 0;
    for (int i = 0; i <= n; ++i) {
        const bool atEnd = (i == n);
        const bool isNl = !atEnd && (body_.at(i) == QLatin1Char('\n') || body_.at(i) == QLatin1Char('\r'));
        if (atEnd || isNl) {
            if (i > paraBegin) wrapPara(paraBegin, i);
            paraBegin = i + 1;
        }
    }
    if (lines_.isEmpty()) {
        lines_.append(body_);
        lineStarts_.append(0);
        indents_.append(true);
    }
}

// 英文折行：以空格为词边界逐词放入行中；行尾不留空格；
// 首行按“可用宽度-缩进”折行，其余行按可用宽度，均不拆词。
void PracticeView::wrapEnglish()
{
    QFont f = lineLabel_->font();
    f.setPixelSize(20);
    QFontMetrics fm(f);
    const int avail = textAvailWidth();
    const int indentW = 2 * charWidth();
    const int spaceW = fm.horizontalAdvance(QLatin1Char(' '));
    const int n = body_.size();
    if (n == 0) {
        lines_.append(QString());
        lineStarts_.append(0);
        indents_.append(true);
        return;
    }

    int lineStart = 0;
    int i = 0;
    int lineW = 0;             // 当前行已累积宽度
    bool firstLine = true;
    while (i < n) {
        // 跳过行首空格
        if (i == lineStart && body_.at(i) == QLatin1Char(' ')) {
            ++i;
            lineStart = i;
            continue;
        }
        const int sp = body_.indexOf(QLatin1Char(' '), i);
        const int end = (sp < 0) ? n : sp;
        const QString word = body_.mid(i, end - i);
        const int wordW = textPixWidth(fm, word);
        const int addW = (i > lineStart) ? (spaceW + wordW) : wordW;
        const int limit = firstLine ? qMax(1, avail - indentW) : avail;
        if (lineW + addW > limit && lineW > 0) {
            // 折行：当前行内容为 [lineStart, i)，i 指向词首且 body_[i-1] 为空格
            QString line = body_.mid(lineStart, i - lineStart);
            if (line.endsWith(QLatin1Char(' '))) line.chop(1);
            lines_.append(line);
            lineStarts_.append(lineStart);
            indents_.append(firstLine);
            firstLine = false;
            lineStart = i;
            lineW = 0;
            continue;
        }
        lineW += addW;         // 该词放入当前行
        i = end + 1;
        if (sp < 0) break;     // 到文末
    }
    // 收尾剩余内容（含最后一行）
    if (lineStart < n) {
        QString line = body_.mid(lineStart, n - lineStart);
        if (line.endsWith(QLatin1Char(' '))) line.chop(1);
        if (!line.isEmpty()) {
            lines_.append(line);
            lineStarts_.append(lineStart);
            indents_.append(firstLine);
        }
    }
    if (lines_.isEmpty()) {
        lines_.append(body_);
        lineStarts_.append(0);
        indents_.append(true);
    }
}

int PracticeView::textAvailWidth() const
{
    // 返回「非缩进行」的文字最大可用宽。
    // 缩进行再在折行逻辑里减 indentW。
    // 扣除：右 padding + 边框余量 + 字形 overhang（分号等 right-bearing）。
    // 下限 120：无界面自检/未完成布局时避免逐字折行导致匹配错乱。
    const int rightW = rightCard_ ? rightCard_->width() : 0;
    const int gap = rightCard_ ? 10 : 0;
    int labelW = width() - rightW - gap - 20 - 36 - 2;
    // 窗口已合理布局时，改用实测宽度（更准，避免估宽偏大把行尾画出界）
    if (lineLabel_ && lineLabel_->width() > 200)
        labelW = lineLabel_->width();
    else if (lineCard_ && lineCard_->width() > 200)
        labelW = lineCard_->contentsRect().width();
    return qMax(120, labelW - kLinePadRight - kWrapSafety);
}

int PracticeView::charWidth() const
{
    QFont f = lineLabel_->font();
    f.setPixelSize(20);
    return QFontMetrics(f).horizontalAdvance(QStringLiteral("国"));
}
