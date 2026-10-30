#include "practice.h"
#include "practice_internal.h"
#include "bilingual.h"
#include "justifiedlabel.h"
#include "justifiedinput.h"
#include <QLabel>
#include <QLineEdit>

// ============================================================================
// 渲染：练习区各行标签、右侧句子对照面板与统计栏的刷新。
// 行标签统一使用自绘 JustifiedLabel（英文行自动两端对齐、最后一行左对齐，
// 当前行按已输入/目标/其余分段着色）。
// 同行内逐字输入时跳过周围行结构/stylesheet 重设，保证跟打流畅、不打断 IME。
// ============================================================================

void PracticeView::refresh()
{
    if (finished_) {
        hintLabel_->setText(QString());
        lineLabel_->setText(QStringLiteral("练习完成"));
        done2Line_->setText(QString()); done2In_->setText(QString());
        done1Line_->setText(QString()); done1In_->setText(QString());
        done2Line_->setVisible(false); done2In_->setVisible(false);
        done1Line_->setVisible(false); done1In_->setVisible(false);
        up1_->setText(QString()); up2_->setText(QString()); up3_->setText(QString());
        up4_->setText(QString()); up5_->setText(QString());
        up1_->setVisible(false); up2_->setVisible(false); up3_->setVisible(false);
        up4_->setVisible(false); up5_->setVisible(false);
        lastRenderLine_ = -1;
        updateRightPanel();
        updateStats();
        return;
    }
    if (lines_.isEmpty()) return;

    // ---- 界面上方：当前词注音 + 五笔编码（英文模式：单词中英双解） ----
    if (!hintsOn_) {
        hintLabel_->setText(QString());
    } else if (langMode_ == 1) {
        const QString w = currentEnWord();
        const QString m = targetMeaning();
        QString s = QStringLiteral("<b style='font-size:22px;color:#0f172a'>%1</b>")
                        .arg(esc(w));
        if (!m.isEmpty())
            s += QStringLiteral("　释义 <span style='color:#475569'>%1</span>").arg(esc(m));
        else
            s += QStringLiteral("　<span style='color:#94a3b8'>(未收录释义)</span>");
        hintLabel_->setText(s);
    } else {
        const QString t = targetText();
        const QString py = targetPinyin();
        const QString code = targetCode();
        QString s = QStringLiteral("<b style='font-size:22px;color:#0f172a'>%1</b>")
                        .arg(esc(t));
        if (!py.isEmpty())
            s += QStringLiteral("　注音 <span style='color:#475569'>%1</span>").arg(esc(py));
        if (!code.isEmpty())
            s += QStringLiteral("　五笔 <b style='font-family:Consolas,monospace;color:#1d4ed8'>%1</b>")
                     .arg(code.toUpper());
        hintLabel_->setText(s);
    }

    const int indentW = 2 * charWidth();
    // 左右 padding 必须对称留足：此前当前行 padding-right:2px 导致行尾 ; 被裁切
    auto rowStyle = [indentW](int basePad, bool indent) {
        return QStringLiteral("font-size:20px;padding-left:%1px;padding-right:%2px;")
            .arg(basePad + (indent ? indentW : 0))
            .arg(kLinePadRight);
    };
    const bool lineChanged = (lineIdx_ != lastRenderLine_);
    lastRenderLine_ = lineIdx_;

    // ---- 已完成行 / 后续行：仅换行时重建（同行内输入不必反复 setStyleSheet/setVisible） ----
    if (lineChanged) {
        const int d0 = lineIdx_ - 2;
        const int d1 = lineIdx_ - 1;
        const QString t2 = d0 >= 0 ? lines_[d0] : QString();
        const QString t1 = d1 >= 0 ? lines_[d1] : QString();
        done2Line_->setText(t2);
        done2In_->setText(t2);
        done1Line_->setText(t1);
        done1In_->setText(t1);
        done2Line_->setLastRow(d0 == lines_.size() - 1);
        done2In_->setLastRow(d0 == lines_.size() - 1);
        done1Line_->setLastRow(d1 == lines_.size() - 1);
        done1In_->setLastRow(d1 == lines_.size() - 1);
        done2Line_->setStyleSheet(rowStyle(18, d0 >= 0 && indents_[d0]));
        done2In_->setStyleSheet(rowStyle(18, d0 >= 0 && indents_[d0]));
        done1Line_->setStyleSheet(rowStyle(18, d1 >= 0 && indents_[d1]));
        done1In_->setStyleSheet(rowStyle(18, d1 >= 0 && indents_[d1]));
        done2Line_->setVisible(!t2.isEmpty());
        done2In_->setVisible(!t2.isEmpty());
        done1Line_->setVisible(!t1.isEmpty());
        done1In_->setVisible(!t1.isEmpty());

        auto upText = [&](int idx) -> QString {
            return (idx < lines_.size()) ? lines_[idx] : QString();
        };
        const int u0 = lineIdx_ + 1;
        const int u1 = lineIdx_ + 2;
        const int u2 = lineIdx_ + 3;
        const int u3 = lineIdx_ + 4;
        const int u4 = lineIdx_ + 5;
        up1_->setText(upText(u0));
        up2_->setText(upText(u1));
        up3_->setText(upText(u2));
        up4_->setText(upText(u3));
        up5_->setText(upText(u4));
        up1_->setLastRow(u0 == lines_.size() - 1);
        up2_->setLastRow(u1 == lines_.size() - 1);
        up3_->setLastRow(u2 == lines_.size() - 1);
        up4_->setLastRow(u3 == lines_.size() - 1);
        up5_->setLastRow(u4 == lines_.size() - 1);
        up1_->setStyleSheet(rowStyle(18, u0 < lines_.size() && indents_[u0]));
        up2_->setStyleSheet(rowStyle(18, u1 < lines_.size() && indents_[u1]));
        up3_->setStyleSheet(rowStyle(18, u2 < lines_.size() && indents_[u2]));
        up4_->setStyleSheet(rowStyle(18, u3 < lines_.size() && indents_[u3]));
        up5_->setStyleSheet(rowStyle(18, u4 < lines_.size() && indents_[u4]));
        up1_->setVisible(u0 < lines_.size());
        up2_->setVisible(u1 < lines_.size());
        up3_->setVisible(u2 < lines_.size());
        up4_->setVisible(u3 < lines_.size());
        up5_->setVisible(u4 < lines_.size());
    }

    // ---- 当前行卡片：原文高亮 + 输入框布局（每键更新） ----
    const bool curIndent = indents_[lineIdx_];
    const QString& cur = lines_[lineIdx_];
    const int hs = targetStartInLine();
    const int hl = targetLenInLine();
    const int hiStart = qMax(hs, typedLen_);
    if (lineChanged)
        lineLabel_->setText(cur);
    lineLabel_->setTarget(hiStart, hl);
    if (lineChanged) {
        lineLabel_->setLastRow(lineIdx_ == lines_.size() - 1);
        lineLabel_->setStyleSheet(QStringLiteral(
            "font-size:20px;padding-left:%1px;padding-right:%2px;")
                                     .arg(curIndent ? indentW : 0)
                                     .arg(kLinePadRight));
    }

    const int wantPad = curIndent ? indentW : 0;
    if (wantPad != lastInputIndent_) {
        lastInputIndent_ = wantPad;
        // 右侧留白与原文行一致，避免输入尾字符贴边被裁
        input_->setStyleSheet(QStringLiteral(
            "QLineEdit{background:transparent;border:none;"
            "font-size:20px;color:#1e40af;padding:6px %1px 4px 0px;}")
                                 .arg(kLinePadRight));
        input_->setLeftPad(wantPad);
    }
    input_->setLineLayout(cur, typedLen_, lineIdx_ != lines_.size() - 1);

    updateRightPanel();
    updateStats();
    // 已有焦点时切勿反复 setFocus：部分平台会重置 IME 组合态，表现为偶发无法输入
    if (!finished_ && input_->isEnabled() && !input_->hasFocus())
        input_->setFocus(Qt::OtherFocusReason);
}

// 右侧句子对照面板：练中文 → 高亮当前输入内容对应的英文句；
// 练英文 → 高亮当前输入英文句对应的中文句。附上一句与下一句作上下文。
void PracticeView::updateRightPanel()
{
    if (pairs_.isEmpty()) {
        rightTitle_->setText(QStringLiteral("句子对照"));
        rightLabel_->setText(QStringLiteral("该文章暂无英文译文，仅支持中文练习。"));
        return;
    }
    const int si = (langMode_ == 1) ? currentEnSentence() : currentZhSentence();
    if (si < 0) {
        rightLabel_->setText(QString());
        return;
    }
    if (langMode_ == 1) {
        rightTitle_->setText(QStringLiteral("中文对照 · 当前英文句"));
        QString html;
        if (si > 0)
            html += QStringLiteral("<div style='color:#94a3b8;margin-bottom:8px'>%1</div>")
                        .arg(esc(pairs_[si - 1].zh));
        html += QStringLiteral("<div style='background:#fde68a;color:#0f172a;font-weight:bold;"
                               "padding:6px;border-radius:6px;margin-bottom:8px'>%1</div>")
                    .arg(esc(pairs_[si].zh));
        if (si + 1 < pairs_.size())
            html += QStringLiteral("<div style='color:#94a3b8'>%1</div>")
                        .arg(esc(pairs_[si + 1].zh));
        rightLabel_->setText(html);
    } else {
        rightTitle_->setText(QStringLiteral("英文对照 · 当前中文句"));
        QString html;
        if (si > 0)
            html += QStringLiteral("<div style='color:#94a3b8;margin-bottom:8px'>%1</div>")
                        .arg(esc(pairs_[si - 1].en));
        html += QStringLiteral("<div style='background:#fde68a;color:#0f172a;font-weight:bold;"
                               "padding:6px;border-radius:6px;margin-bottom:8px'>%1</div>")
                    .arg(esc(pairs_[si].en));
        if (si + 1 < pairs_.size())
            html += QStringLiteral("<div style='color:#94a3b8'>%1</div>")
                        .arg(esc(pairs_[si + 1].en));
        rightLabel_->setText(html);
    }
}

void PracticeView::updateStats()
{
    // 全文速度 = 正确字数 / 累计活动时间
    qint64 fullSpeed = 0;
    if (activeMs_ > 0)
        fullSpeed = qint64(correct_) * 60000 / activeMs_;

    // 最近 5 分钟速度 = 窗口内正确增量 / 窗口内活动时间
    qint64 speed5 = 0;
    const qint64 now = clock_.elapsed();
    int idx = 0;
    while (idx < samples_.size() && samples_[idx].wall < now - kWindowMs)
        ++idx;
    if (samples_.size() - idx >= 2) {
        const Sample& first = samples_[idx];
        const Sample& last = samples_.last();
        const qint64 dActive = last.active - first.active;
        if (dActive > 0)
            speed5 = qint64(last.correct - first.correct) * 60000 / dActive;
    }

    const QString fullTxt = fullSpeed > 0 ? QString::number(fullSpeed, 'f', 1) : QStringLiteral("—");
    const QString fiveTxt = speed5 > 0 ? QString::number(speed5, 'f', 1) : QStringLiteral("—");

    // 用时：时分秒（HH:MM:SS）
    const qint64 secs = activeMs_ / 1000;
    const QString hms = QStringLiteral("%1:%2:%3")
        .arg(secs / 3600)
        .arg((secs % 3600) / 60, 2, 10, QLatin1Char('0'))
        .arg(secs % 60, 2, 10, QLatin1Char('0'));

    const QString unit = (langMode_ == 1) ? QStringLiteral("字符/分") : QStringLiteral("字/分");
    QString s = QStringLiteral("进度 %1/%2 · 正确 %3 · 错误 %4 · 跳过 %5 · 用时 %6"
                               " · 全文速度 <b style='color:#0f766e'>%7</b> %8"
                               " · 最近5分钟 <b style='color:#1d4ed8'>%9</b> %10")
                    .arg(absPos()).arg(body_.size()).arg(correct_).arg(error_).arg(skip_)
                    .arg(hms)
                    .arg(fullTxt, unit, fiveTxt, unit);
    if (paused_ && !finished_)
        s += QStringLiteral(" · <span style='color:#b45309'>（练习暂停中）</span>");
    statsLabel_->setText(s);
}
